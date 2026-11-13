// Shared image workflow. Host ABI is isolated in the selected adapter.
#include "ui.h"
#include "host.h"
#include <algorithm>

static HINSTANCE instance;
static cloud::SessionState session;
static std::wstring moduleDir;
static void checked(AibHostInt value,const char* operation){if(value!=kAibHostAPIResultSuccess)throw std::runtime_error(std::string("クリスタとの画像処理に失敗しました。\n")+operation+" ("+std::to_string(value)+")");}
#define ok(call) checked((call),#call)
static AibHostStringObject str(AibHostStringService* ss,const wchar_t* text){AibHostStringObject s{};ok(ss->createWithUnicodeStringProc(&s,(const AibHostUniChar*)text,(AibHostInt)wcslen(text)));return s;}
struct Block{
    AibHostRect rect{};AibHostPtr ptr{};AibHostInt row=0,pixel=0;int originX=0,originY=0;
    unsigned char* at(int x,int y){if(!ptr||x<rect.left||x>=rect.right||y<rect.top||y>=rect.bottom)return nullptr;return (unsigned char*)ptr+(y-originY)*row+(x-originX)*pixel;}
};
static Block block(AibHostOffscreenService* ss,AibHostOffscreenObject obj,int x,int y,int kind){
    Block b;if(!obj)return b;b.originX=x;b.originY=y;AibHostPoint p{x,y};if(kind==0)ok(ss->getBlockImageProc(&b.ptr,&b.row,&b.pixel,&b.rect,obj,&p));else if(kind==1)ok(ss->getBlockAlphaProc(&b.ptr,&b.row,&b.pixel,&b.rect,obj,&p));else ok(ss->getBlockSelectAreaProc(&b.ptr,&b.row,&b.pixel,&b.rect,obj,&p));return b;
}
static std::vector<AibHostRect> rectangles(AibHostOffscreenService* ss,AibHostOffscreenObject obj,AibHostRect bounds){AibHostInt count=0;ok(ss->getBlockRectCountProc(&count,obj,&bounds));if(count<0||count>1048576)throw std::runtime_error("画像ブロック数が不正です。");std::vector<AibHostRect> r(count);for(int i=0;i<count;i++)ok(ss->getBlockRectProc(&r[i],i,obj,&bounds));return r;}
static void run(AibHostInt* result,AibHostServer* server){
    auto rec=&server->recordSuite;auto host=server->hostObject;auto ss=server->serviceSuite.offscreenService;if(!ss)throw std::runtime_error("画像サービスが使用できません。");
    AibHostInt progress=0;bool started=false;
    try{
        AibHostOffscreenObject src{},dst{},sel{};AibHostRect area{};
        AibHostBool hasSelection=false,maskSelected=false,alphaLocked=false;
        ok(AibHostFilterRunIsLayerMaskSelected(rec,&maskSelected,host));
        if(maskSelected)throw std::runtime_error("レイヤーマスクではなく、編集するラスターレイヤーの画像サムネイルを選択してください。");
        ok(AibHostFilterRunIsAlphaLocked(rec,&alphaLocked,host));
        ok(AibHostFilterRunGetSourceOffscreen(rec,&src,host));ok(AibHostFilterRunGetDestinationOffscreen(rec,&dst,host));ok(AibHostFilterRunGetSelectAreaRect(rec,&area,host));
        ok(AibHostFilterRunHasSelectAreaOffscreen(rec,&hasSelection,host));
        if(hasSelection)ok(AibHostFilterRunGetSelectAreaOffscreen(rec,&sel,host));
        AibHostRect full{};ok(ss->getRectProc(&full,src));
        AibHostInt r=0,g=1,b=2;ok(ss->getRGBChannelIndexProc(&r,&g,&b,src));
        cloud::ReadInput reader=[&](int mode){
            auto bounds=cloud::contextRect({area.left,area.top,area.right,area.bottom},{full.left,full.top,full.right,full.bottom},hasSelection?mode:2);
            int w=bounds.right-bounds.left,h=bounds.bottom-bounds.top;if(w<1||h<1||w>16384||h>16384||(uint64_t)w*h>33554432)throw std::runtime_error("レイヤーが大きすぎます。3,355万画素以下・各辺16,384px以下の画像で試してください。");
            cloud::Tile tile;tile.rect=bounds;tile.selected=hasSelection!=0;tile.image={w,h,cloud::Bytes((size_t)w*h*4,0)};tile.mask=cloud::Bytes((size_t)w*h,sel?0:255);
            AibHostRect regionBounds{bounds.left,bounds.top,bounds.right,bounds.bottom};
            for(const auto& region:rectangles(ss,src,regionBounds)){auto color=block(ss,src,region.left,region.top,0),alpha=block(ss,src,region.left,region.top,1);
                for(int y=std::max<int>(region.top,bounds.top);y<std::min<int>(region.bottom,bounds.bottom);y++)for(int x=std::max<int>(region.left,bounds.left);x<std::min<int>(region.right,bounds.right);x++){
                    size_t p=(size_t)(y-bounds.top)*w+x-bounds.left;auto c=color.at(x,y),a=alpha.at(x,y);if(c&&a){tile.image.rgba[p*4]=c[r];tile.image.rgba[p*4+1]=c[g];tile.image.rgba[p*4+2]=c[b];tile.image.rgba[p*4+3]=*a;}
                }
            }
            if(sel){AibHostRect maskBounds{std::max<int>(bounds.left,area.left),std::max<int>(bounds.top,area.top),std::min<int>(bounds.right,area.right),std::min<int>(bounds.bottom,area.bottom)};
                for(const auto& region:rectangles(ss,sel,maskBounds)){auto selection=block(ss,sel,std::max<int>(region.left,maskBounds.left),std::max<int>(region.top,maskBounds.top),2);
                    for(int y=std::max<int>(region.top,maskBounds.top);y<std::min<int>(region.bottom,maskBounds.bottom);y++)for(int x=std::max<int>(region.left,maskBounds.left);x<std::min<int>(region.right,maskBounds.right);x++){auto s=selection.at(x,y);if(s)tile.mask[(size_t)(y-bounds.top)*w+x-bounds.left]=*s;}
                }
            }
            if(std::none_of(tile.mask.begin(),tile.mask.end(),[](unsigned char m){return m!=0;}))throw std::runtime_error("編集できる選択範囲がありません。");return tile;
        };
        cloud::Tile output;HWND owner=GetActiveWindow();
        if(!cloud::showEditor(instance,owner,session,reader,alphaLocked!=0,output,moduleDir+L"\\diagnostics.jsonl")){*result=kAibHostCallResultFailed;return;}
        int w=output.image.w;AibHostRect writeArea{std::max<int>(area.left,output.rect.left),std::max<int>(area.top,output.rect.top),std::min<int>(area.right,output.rect.right),std::min<int>(area.bottom,output.rect.bottom)};
        ok(AibHostFilterRunProcess(rec,&progress,host,kAibHostFilterRunProcessStateStart));started=true;if(progress==kAibHostFilterRunProcessResultExit){*result=kAibHostCallResultFailed;return;}
        ok(ss->getRGBChannelIndexProc(&r,&g,&b,dst));auto destRegions=rectangles(ss,dst,writeArea);ok(AibHostFilterRunSetProgressTotal(rec,host,(int)destRegions.size()));int done=0;
        for(const auto& region:destRegions){auto color=block(ss,dst,region.left,region.top,0),alpha=block(ss,dst,region.left,region.top,1);
            for(int y=std::max<int>(region.top,writeArea.top);y<std::min<int>(region.bottom,writeArea.bottom);y++)for(int x=std::max<int>(region.left,writeArea.left);x<std::min<int>(region.right,writeArea.right);x++){
                size_t p=(size_t)(y-output.rect.top)*w+x-output.rect.left;if(!output.mask[p])continue;auto c=color.at(x,y),a=alpha.at(x,y);if(!c||!a)throw std::runtime_error("書き込み先の画像ブロックが取得できません。");c[r]=output.image.rgba[p*4];c[g]=output.image.rgba[p*4+1];c[b]=output.image.rgba[p*4+2];*a=output.image.rgba[p*4+3];
            }
            AibHostRect update{std::max<int>(region.left,writeArea.left),std::max<int>(region.top,writeArea.top),std::min<int>(region.right,writeArea.right),std::min<int>(region.bottom,writeArea.bottom)};ok(AibHostFilterRunUpdateDestinationOffscreenRect(rec,host,&update));ok(AibHostFilterRunSetProgressDone(rec,host,++done));
            ok(AibHostFilterRunProcess(rec,&progress,host,kAibHostFilterRunProcessStateContinue));if(progress==kAibHostFilterRunProcessResultExit){*result=kAibHostCallResultSuccess;return;}
        }
        ok(AibHostFilterRunProcess(rec,&progress,host,kAibHostFilterRunProcessStateEnd));*result=kAibHostCallResultSuccess;
    }catch(...){if(started)AibHostFilterRunProcess(rec,&progress,host,kAibHostFilterRunProcessStateAbort);throw;}
}
void AIB_HOST_CALLBACK TriglavPluginCall(AibHostInt* result,AibHostPtr* data,AibHostInt selector,AibHostServer* server,AibHostPtr){
    *result=kAibHostCallResultFailed;if(!server)return;
    try{
        auto ss=server->serviceSuite.stringService;auto rec=&server->recordSuite;auto host=server->hostObject;
        if(selector==kAibHostSelectorModuleInitialize){auto init=rec->moduleInitializeRecord;AibHostInt version=0;ok(init->getHostVersionProc(&version,host));if(version<kAibHostNeedHostVersion)return;
            auto id=str(ss,L"09DA77C4-49D2-4BA5-B4F5-E398F53F3A90");ok(init->setModuleIDProc(host,id));ss->releaseProc(id);ok(init->setModuleKindProc(host,kAibHostModuleSwitchKindFilter));*data=nullptr;
        }else if(selector==kAibHostSelectorFilterInitialize){auto category=str(ss,L"AI Image Bridge (" AIB_HOST_VARIANT_W L")"),name=str(ss,cloud::uiLanguage?L"AI image editor...":L"AI画像編集...");ok(AibHostFilterInitializeSetFilterCategoryName(rec,host,category,'c'));ok(AibHostFilterInitializeSetFilterName(rec,host,name,'g'));ss->releaseProc(category);ss->releaseProc(name);ok(AibHostFilterInitializeSetCanPreview(rec,host,false));ok(AibHostFilterInitializeSetUseBlankImage(rec,host,true));AibHostInt target=kAibHostFilterTargetKindRasterLayerRGBAlpha;ok(AibHostFilterInitializeSetTargetKinds(rec,host,&target,1));
        }else if(selector==kAibHostSelectorFilterRun){run(result,server);return;}
        else if(selector==kAibHostSelectorModuleTerminate){for(int p=0;p<2;p++)for(auto* key:{&session.keys[p],&session.connectedKeys[p]}){if(!key->empty())SecureZeroMemory(key->data(),key->size()*sizeof(wchar_t));key->clear();}}
        *result=kAibHostCallResultSuccess;
    }catch(const std::exception& ex){MessageBoxW(GetActiveWindow(),cloud::localizeError(cloud::wide(ex.what())).c_str(),L"AI Image Bridge",MB_OK|MB_ICONERROR);}catch(...){MessageBoxW(GetActiveWindow(),cloud::tr(L"予期しないエラーで処理を中止しました。"),L"AI Image Bridge",MB_OK|MB_ICONERROR);}
}
BOOL WINAPI DllMain(HINSTANCE inst,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){instance=inst;DisableThreadLibraryCalls(inst);wchar_t path[32768];DWORD n=GetModuleFileNameW(inst,path,32768);moduleDir=std::wstring(path,n);moduleDir=moduleDir.substr(0,moduleDir.find_last_of(L"\\/"));}return TRUE;}
