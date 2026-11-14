#include "generated_version.h"
#include "ui.h"
#include <commdlg.h>
#include <shellapi.h>
#pragma comment(lib,"shell32.lib")
#include <thread>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <cwchar>

namespace cloud {
enum {ID_PROVIDER=101,ID_MODEL,ID_KEY,ID_PROMPT,ID_QUALITY,ID_COMPLEXITY,ID_MODE,ID_GENERATE,ID_APPLY,ID_SAVE,ID_CANCEL_RUN,ID_SOURCE,ID_RESULT,ID_STATUS,ID_CLEAR_KEY,ID_CONTEXT,ID_REF_ADD,ID_REF_CLEAR,ID_REF_NAME,ID_GUIDE,ID_BASE,ID_PREVIOUS,ID_KEY_NOTE,ID_SOURCE_INFO,ID_RESULT_INFO,ID_SCOPE,ID_QUALITY_LABEL,ID_REF_PREVIEW,ID_LAYER_NOTE,ID_CONNECT,ID_REDO,ID_CLOSE,ID_SECURITY,ID_MODE_NOTE,ID_LANGUAGE};
namespace color {constexpr COLORREF background=RGB(30,34,40),panel=RGB(38,43,51),input=RGB(25,29,35),text=RGB(234,240,246),muted=RGB(155,169,188),accent=RGB(106,155,255),border=RGB(62,73,89),disabled=RGB(92,101,115),danger=RGB(185,28,28),dangerPressed=RGB(153,27,27);}
struct Editor {
    HWND hwnd{};SessionState& session;const ReadInput& readInput;bool alphaLocked;Tile& output;std::wstring logPath;
    Tile input;Bytes positionHint;bool drawing=false;int lastX=0,lastY=0;WNDPROC sourceProc{},promptProc{};CandidateHistory history;Image sourcePreview,referencePreview,requestImage;
    const Candidate& candidate()const{return history.current();}
    Cancel cancel;std::thread worker;bool busy=false,showGuide=true,connecting=false;std::wstring error;Reply reply;Settings settings;int http=0;std::vector<std::string> fetchedModels;
    std::chrono::steady_clock::time_point start;double seconds=0;std::string runId;
    HBRUSH background=CreateSolidBrush(color::background),inputBrush=CreateSolidBrush(color::input),panel=CreateSolidBrush(color::panel);
    HFONT bodyFont{},headingFont{};
    ~Editor(){cancel.stop();if(worker.joinable())worker.join();DeleteObject(background);DeleteObject(inputBrush);DeleteObject(panel);if(bodyFont)DeleteObject(bodyFont);if(headingFont)DeleteObject(headingFont);}
};
static std::wstring getText(HWND h,int id){HWND e=GetDlgItem(h,id);int n=GetWindowTextLengthW(e);std::wstring s(n+1,0);GetWindowTextW(e,s.data(),n+1);s.resize(n);return s;}
static int selection(HWND h,int id){int n=(int)SendDlgItemMessageW(h,id,CB_GETCURSEL,0,0);return n<0?0:n;}
static void status(Editor* e,const std::wstring& text){SetDlgItemTextW(e->hwnd,ID_STATUS,text.c_str());}
static void populate(HWND h,int id,std::initializer_list<const wchar_t*> values,int chosen){SendDlgItemMessageW(h,id,CB_RESETCONTENT,0,0);for(auto s:values)SendDlgItemMessageW(h,id,CB_ADDSTRING,0,(LPARAM)s);SendDlgItemMessageW(h,id,CB_SETCURSEL,chosen,0);}
static bool sameRect(Rect a,Rect b){return a.left==b.left&&a.top==b.top&&a.right==b.right&&a.bottom==b.bottom;}
static bool canContinue(Editor* e){return !e->candidate().empty()&&sameRect(e->input.rect,e->candidate().tile.rect);}
static const Image& baseImage(Editor* e){return selection(e->hwnd,ID_BASE)==1&&canContinue(e)?e->candidate().tile.image:e->input.image;}
static bool recognized(Editor* e){auto key=getText(e->hwnd,ID_KEY);return !key.empty()&&key==e->session.connectedKeys[e->session.provider];}
static bool connected(Editor* e){return recognized(e)&&!e->session.availableModels[e->session.provider].empty();}
static void keyNote(Editor* e){bool present=GetWindowTextLengthW(GetDlgItem(e->hwnd,ID_KEY))>0;
    SetDlgItemTextW(e->hwnd,ID_KEY_NOTE,connected(e)?cloud::tr(L"認識済み。この接続先は再入力不要です。"):present?cloud::tr(L"「送信・モデル取得」で接続を確認します。"):cloud::tr(L"この接続先のキーを入力して送信してください。"));}
static void refreshPreview(Editor* e){const auto& im=baseImage(e);e->sourcePreview=!e->positionHint.empty()?annotateImage(im,selectionMarkup(im,e->positionHint)):im;InvalidateRect(GetDlgItem(e->hwnd,ID_SOURCE),nullptr,TRUE);}
static void refreshControls(Editor* e){HWND h=e->hwnd;bool idle=!e->busy;
    for(int id:{ID_PROVIDER,ID_KEY,ID_PROMPT,ID_MODE,ID_REF_ADD,ID_CLOSE,ID_LANGUAGE})EnableWindow(GetDlgItem(h,id),idle);
    EnableWindow(GetDlgItem(h,ID_GENERATE),idle&&connected(e));EnableWindow(GetDlgItem(h,ID_MODEL),idle&&connected(e));
    bool hasKey=GetWindowTextLengthW(GetDlgItem(h,ID_KEY))>0;EnableWindow(GetDlgItem(h,ID_CONNECT),idle&&hasKey&&!recognized(e));EnableWindow(GetDlgItem(h,ID_CLEAR_KEY),idle&&hasKey);
    SetDlgItemTextW(h,ID_CONNECT,e->connecting?cloud::tr(L"認識中…"):recognized(e)?cloud::tr(L"認識済み"):cloud::tr(L"送信・モデル取得"));
    EnableWindow(GetDlgItem(h,ID_REF_CLEAR),idle&&!e->session.reference.empty());
    EnableWindow(GetDlgItem(h,ID_APPLY),idle&&!e->candidate().empty());EnableWindow(GetDlgItem(h,ID_SAVE),idle&&!e->candidate().empty());
    EnableWindow(GetDlgItem(h,ID_PREVIOUS),idle&&e->history.canUndo());EnableWindow(GetDlgItem(h,ID_REDO),idle&&e->history.canRedo());
    if(!canContinue(e))SendDlgItemMessageW(h,ID_BASE,CB_SETCURSEL,0,0);EnableWindow(GetDlgItem(h,ID_BASE),idle&&canContinue(e));
    ShowWindow(GetDlgItem(h,ID_CANCEL_RUN),emergencyStopVisible(connected(e),e->busy)?SW_SHOW:SW_HIDE);ShowWindow(GetDlgItem(h,ID_SAVE),SW_SHOW);EnableWindow(GetDlgItem(h,ID_CANCEL_RUN),emergencyStopEnabled(e->busy,e->cancel.cancelled));
    SetDlgItemTextW(h,ID_APPLY,e->input.selected&&e->candidate().natural?cloud::tr(L"選択解除後に適用…"):cloud::tr(L"適用"));
    SetDlgItemTextW(h,ID_SCOPE,e->input.selected?cloud::tr(L"クリスタの選択が残っています。全体への適用時は選択解除が必要です。"):cloud::tr(L"形・背丈・背景とのつながりは自然に。位置指定は任意です。"));
    SetDlgItemTextW(h,ID_MODE_NOTE,cloud::tr(L"元画像をクリック・なぞって「だいたいこの辺」を指定できます。"));EnableWindow(GetDlgItem(h,ID_GUIDE),idle&&!e->positionHint.empty());InvalidateRect(GetDlgItem(h,ID_PROMPT),nullptr,TRUE);
    SetDlgItemTextW(h,ID_LAYER_NOTE,e->alphaLocked?cloud::tr(L"現在のレイヤーに反映（透明度ロック中）。適用後はクリスタのCtrl+Zで取消。"):cloud::tr(L"現在のレイヤーに反映。元を残すなら先に複製。適用後はCtrl+Zで取消。"));
    SetDlgItemTextW(h,ID_SOURCE_INFO,(std::to_wstring(e->input.image.w)+L" × "+std::to_wstring(e->input.image.h)+cloud::tr(L" px ／ 常にレイヤー全体をAIへ送信")).c_str());
    std::wstring result=e->candidate().empty()?cloud::tr(L"元の画像（まだ適用しません）"):wide(e->candidate().model)+L" ／ "+std::to_wstring((int)(e->candidate().seconds+.5))+cloud::tr(L"秒");
    if(!e->candidate().empty())result+=e->candidate().provider?L" / Gemini":cloud::tr(L"\n画質 ")+wide(e->candidate().quality)+cloud::tr(L" ／ 複雑さ ")+(e->candidate().complexity.empty()?cloud::tr(L"モデル既定"):wide(e->candidate().complexity));SetDlgItemTextW(h,ID_RESULT_INFO,result.c_str());SetDlgItemTextW(h,ID_REF_NAME,e->session.reference.empty()?cloud::tr(L"未追加：文章だけでも生成できます。"):e->session.referenceName.c_str());
    SetDlgItemTextW(h,ID_QUALITY_LABEL,e->session.provider?cloud::tr(L"最高解像度 4K（旧2.5はモデル規定値）"):cloud::tr(L"最高画質 max ／ 構成の複雑さ high"));
}
static void models(Editor* e){auto& s=e->session;HWND h=e->hwnd;
    SendDlgItemMessageW(h,ID_MODEL,CB_RESETCONTENT,0,0);const auto& list=s.availableModels[s.provider];for(const auto& id:list){auto label=wide(id);SendDlgItemMessageW(h,ID_MODEL,CB_ADDSTRING,0,(LPARAM)label.c_str());}if(list.empty())SendDlgItemMessageW(h,ID_MODEL,CB_ADDSTRING,0,(LPARAM)cloud::tr(L"キー送信後に自動取得"));s.model[s.provider]=std::max(0,std::min(s.model[s.provider],(int)list.size()-1));SendDlgItemMessageW(h,ID_MODEL,CB_SETCURSEL,s.model[s.provider],0);
    SetDlgItemTextW(h,ID_KEY,s.keys[s.provider].c_str());keyNote(e);refreshControls(e);
}
static void applyLanguage(Editor* e){HWND h=e->hwnd;int base=selection(h,ID_BASE);
#ifdef AIB_OFFICIAL_SDK
    constexpr auto variant=L"Official SDK";
#else
    constexpr auto variant=L"Unofficial SDK";
#endif
    SetWindowTextW(h,(std::wstring(AIB_TITLE_WIDE L" | ")+variant).c_str());
    SetDlgItemTextW(h,136,uiLanguage?L"Manuals":L"マニュアル");
    SetDlgItemTextW(h,201,tr(L"AI Image Bridge"));
    SetDlgItemTextW(h,202,tr(L"選ぶ。指示する。絵に戻す。"));
    SetDlgItemTextW(h,203,(std::wstring(AIB_VERSION_WIDE L" / ")+variant).c_str());
    SetDlgItemTextW(h,301,tr(L"APIキー"));
    SetDlgItemTextW(h,130,tr(L"送信・モデル取得"));
    SetDlgItemTextW(h,115,tr(L"消去"));
    SetDlgItemTextW(h,133,tr(L"キーは接続先ごとにメモリ内で保持。\n切替後も再入力不要。PCへ保存しません。\nクリスタ終了時に破棄（次回は再入力）。"));
    SetDlgItemTextW(h,302,tr(L"接続先（キー送信時に自動判別）"));
    SetDlgItemTextW(h,303,tr(L"モデル（OpenAIはSunburstを優先）"));
    SetDlgItemTextW(h,304,tr(L"未対応設定のみ互換設定に切替。結果に明示。"));
    SetDlgItemTextW(h,305,tr(L"見た目の参考画像（任意）"));
    SetDlgItemTextW(h,306,tr(L"服装・顔・小物・画風の見本を1枚追加。\n例：この服を着た女の子を左に追加。"));
    SetDlgItemTextW(h,117,tr(L"見本画像を追加…"));
    SetDlgItemTextW(h,118,tr(L"解除"));
    SetDlgItemTextW(h,119,tr(L"未追加：文章だけでも生成できます。"));
    SetDlgItemTextW(h,307,tr(L"元画像（レイヤー全体）"));
    SetDlgItemTextW(h,308,tr(L"生成結果 / 取り消し後の状態"));
    SetDlgItemTextW(h,120,tr(L"位置指定を解除"));
    SetDlgItemTextW(h,309,tr(L"ピンクは「この辺」の目安。\n形や大きさを縛る枠ではありません。"));
    SetDlgItemTextW(h,310,tr(L"編集内容"));
    SetDlgItemTextW(h,311,tr(L"変更したいこと"));
    SetDlgItemTextW(h,122,tr(L"取り消し"));
    SetDlgItemTextW(h,131,tr(L"やり直し"));
    SetDlgItemTextW(h,132,tr(L"閉じる（適用しない）"));
    SetDlgItemTextW(h,110,tr(L"PNG保存"));
    SetDlgItemTextW(h,111,tr(L"緊急停止"));
    SetDlgItemTextW(h,109,tr(L"適用"));
    SetDlgItemTextW(h,108,tr(L"生成する（API課金）"));

    populate(h,ID_MODE,{tr(L"追加"),tr(L"置き換え"),tr(L"消去")},e->session.mode);
    populate(h,ID_BASE,{tr(L"元レイヤーから編集"),tr(L"表示中の候補から続ける")},base);
    populate(h,135,{L"日本語",L"English"},uiLanguage);
    keyNote(e);refreshControls(e);InvalidateRect(h,nullptr,TRUE);
}
static void saveFields(Editor* e){auto& s=e->session;HWND h=e->hwnd;s.keys[s.provider]=getText(h,ID_KEY);s.model[s.provider]=selection(h,ID_MODEL);s.prompt=getText(h,ID_PROMPT);s.mode=selection(h,ID_MODE);}
static void appendLog(Editor* e,const char* event){try{SYSTEMTIME t{};GetSystemTime(&t);char date[40];sprintf_s(date,"%04u-%02u-%02uT%02u:%02u:%02uZ",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
    bool applied=std::string(event)=="apply";const auto& tile=applied?e->candidate().tile:e->input;
    Json j={{"utc",date},{"version",AIB_VERSION_TEXT},{"runId",applied?e->candidate().runId:e->runId},{"event",event},{"provider",(applied?e->candidate().provider:e->settings.provider)==0?"openai":"gemini"},{"model",applied?e->candidate().model:e->settings.model},{"http",applied?200:e->http},{"elapsedSeconds",applied?e->candidate().seconds:e->seconds},{"inputWidth",tile.image.w},{"inputHeight",tile.image.h},{"left",tile.rect.left},{"top",tile.rect.top},{"selected",tile.selected}};
    j["naturalPlacement"]=applied?e->candidate().natural:e->settings.natural;j["quality"]=applied?e->candidate().quality:e->settings.quality;j["complexity"]=applied?e->candidate().complexity:e->settings.complexity;
    if(!applied){j["operation"]=e->settings.operation;j["positionHintPixels"]=std::count_if(e->positionHint.begin(),e->positionHint.end(),[](unsigned char a){return a!=0;});}
    if(std::string(event)=="success"){j["fallbackCount"]=e->reply.fallbackCount;j["effectiveQuality"]=e->reply.quality;j["effectiveComplexity"]=e->reply.complexity;j["usage"]=e->reply.usage;j["requestId"]=e->reply.requestId;j["outputWidth"]=e->reply.image.w;j["outputHeight"]=e->reply.image.h;}
    std::ofstream f(e->logPath,std::ios::app);f<<j.dump()<<"\n";
}catch(...){}}
static void fill(HDC dc,RECT r,COLORREF c){HBRUSH b=CreateSolidBrush(c);FillRect(dc,&r,b);DeleteObject(b);}
static void paintImage(DRAWITEMSTRUCT* d,const Image& im,const wchar_t* empty){fill(d->hDC,d->rcItem,color::input);HBRUSH frame=CreateSolidBrush(color::muted);FrameRect(d->hDC,&d->rcItem,frame);DeleteObject(frame);SetTextColor(d->hDC,color::muted);SetBkMode(d->hDC,TRANSPARENT);
    if(im.empty()){RECT text=d->rcItem;InflateRect(&text,-12,-12);DrawTextW(d->hDC,empty,-1,&text,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);return;}
    const auto rect=previewRect(d->rcItem.right-d->rcItem.left,d->rcItem.bottom-d->rcItem.top,im.w,im.h);
    const int x=d->rcItem.left+rect.left,y=d->rcItem.top+rect.top,w=rect.width,h=rect.height;
    if(w<1||h<1)return;
    Bytes bgra=im.rgba;for(size_t p=0;p<bgra.size()/4;p++){int px=(int)(p%im.w),py=(int)(p/im.w);int bg=((px/16+py/16)%2)?57:68;int a=im.rgba[p*4+3];for(int c=0;c<3;c++)bgra[p*4+c]=(unsigned char)((im.rgba[p*4+2-c]*a+bg*(255-a)+127)/255);bgra[p*4+3]=255;}
    BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=im.w;bi.bmiHeader.biHeight=-im.h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;SetStretchBltMode(d->hDC,HALFTONE);StretchDIBits(d->hDC,x,y,w,h,0,0,im.w,im.h,bgra.data(),&bi,DIB_RGB_COLORS,SRCCOPY);
}
static void paintControl(Editor* e,DRAWITEMSTRUCT* d){
    if(d->CtlType==ODT_COMBOBOX){bool disabled=!IsWindowEnabled(d->hwndItem);fill(d->hDC,d->rcItem,(d->itemState&ODS_SELECTED)?color::border:color::input);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,disabled?color::disabled:color::text);if(d->itemID!=(UINT)-1){int n=(int)SendMessageW(d->hwndItem,CB_GETLBTEXTLEN,d->itemID,0);if(n>=0){std::wstring text(n+1,0);SendMessageW(d->hwndItem,CB_GETLBTEXT,d->itemID,(LPARAM)text.data());RECT r=d->rcItem;r.left+=9;DrawTextW(d->hDC,text.c_str(),n,&r,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);}}if(d->itemState&ODS_FOCUS){RECT r=d->rcItem;InflateRect(&r,-2,-2);DrawFocusRect(d->hDC,&r);}return;}
    if(d->CtlType==ODT_BUTTON){bool disabled=d->itemState&ODS_DISABLED,primary=d->CtlID==ID_GENERATE||d->CtlID==ID_CONNECT,danger=d->CtlID==ID_CANCEL_RUN;COLORREF bg=danger&&disabled?RGB(127,29,29):disabled?color::panel:danger?((d->itemState&ODS_SELECTED)?color::dangerPressed:color::danger):primary?color::accent:(d->itemState&ODS_SELECTED)?color::border:color::panel;HBRUSH brush=CreateSolidBrush(bg);HPEN pen=CreatePen(PS_SOLID,1,danger&&!disabled?color::danger:primary&&!disabled?color::accent:color::border);auto oldB=SelectObject(d->hDC,brush),oldP=SelectObject(d->hDC,pen);RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,9,9);SelectObject(d->hDC,oldB);SelectObject(d->hDC,oldP);DeleteObject(brush);DeleteObject(pen);SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,danger?RGB(255,255,255):disabled?color::disabled:danger?RGB(255,255,255):primary?color::input:color::text);RECT r=d->rcItem;auto text=getText(e->hwnd,d->CtlID);DrawTextW(d->hDC,text.c_str(),-1,&r,DT_SINGLELINE|DT_CENTER|DT_VCENTER);if(d->itemState&ODS_FOCUS){InflateRect(&r,-4,-4);DrawFocusRect(d->hDC,&r);}return;}
    if(d->CtlID==ID_SOURCE)paintImage(d,e->sourcePreview,cloud::tr(L"入力画像"));else if(d->CtlID==ID_RESULT)paintImage(d,e->candidate().empty()?e->input.image:e->candidate().tile.image,cloud::tr(L"生成結果をここで確認"));else if(d->CtlID==ID_REF_PREVIEW)paintImage(d,e->referencePreview,L"+");
}
static void fitWindow(Editor* e){HWND h=e->hwnd;RECT wr{};GetWindowRect(h,&wr);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&mi);RECT cr{};GetClientRect(h,&cr);int frameW=(wr.right-wr.left)-cr.right,frameH=(wr.bottom-wr.top)-cr.bottom;double factor=fitClientScale(cr.right,cr.bottom,mi.rcWork.right-mi.rcWork.left,mi.rcWork.bottom-mi.rcWork.top,frameW,frameH);
    LOGFONTW lf{};GetObjectW((HFONT)SendMessageW(h,WM_GETFONT,0,0),sizeof(lf),&lf);lf.lfHeight=(LONG)(lf.lfHeight*factor);e->bodyFont=CreateFontIndirectW(&lf);
    struct Layout{HWND dialog;double factor;HFONT font;} layout{h,factor,e->bodyFont};
    EnumChildWindows(h,[](HWND child,LPARAM data)->BOOL{auto l=(Layout*)data;RECT r;GetWindowRect(child,&r);MapWindowPoints(nullptr,l->dialog,(POINT*)&r,2);SetWindowPos(child,nullptr,(int)(r.left*l->factor),(int)(r.top*l->factor),(int)((r.right-r.left)*l->factor),(int)((r.bottom-r.top)*l->factor),SWP_NOZORDER|SWP_NOACTIVATE);SendMessageW(child,WM_SETFONT,(WPARAM)l->font,TRUE);return TRUE;},(LPARAM)&layout);
    int w=(int)(cr.right*factor)+frameW,height=(int)(cr.bottom*factor)+frameH;SetWindowPos(h,nullptr,mi.rcWork.left+(mi.rcWork.right-mi.rcWork.left-w)/2,mi.rcWork.top+(mi.rcWork.bottom-mi.rcWork.top-height)/2,w,height,SWP_NOZORDER|SWP_NOACTIVATE);
    lf.lfHeight=(LONG)(lf.lfHeight*1.8);lf.lfWeight=FW_SEMIBOLD;wcscpy_s(lf.lfFaceName,L"Segoe UI");e->headingFont=CreateFontIndirectW(&lf);SendDlgItemMessageW(h,201,WM_SETFONT,(WPARAM)e->headingFont,TRUE);
    HDC dc=GetDC(h);auto old=SelectObject(dc,e->bodyFont);TEXTMETRICW tm{};GetTextMetricsW(dc,&tm);SelectObject(dc,old);ReleaseDC(h,dc);for(int id:{ID_PROVIDER,ID_MODEL,ID_MODE,ID_BASE,ID_LANGUAGE}){SendDlgItemMessageW(h,id,CB_SETITEMHEIGHT,0,tm.tmHeight+8);SendDlgItemMessageW(h,id,CB_SETITEMHEIGHT,(WPARAM)-1,tm.tmHeight+6);}
}
static void connectModels(Editor* e){if(e->busy||recognized(e))return;auto draft=getText(e->hwnd,ID_KEY);int provider=detectProvider(utf8(draft),e->session.provider);auto previousKey=e->session.connectedKeys[e->session.provider];saveFields(e);auto& s=e->session;if(provider!=s.provider){s.keys[s.provider]=std::move(previousKey);s.provider=provider;s.keys[provider]=draft;SendDlgItemMessageW(e->hwnd,ID_PROVIDER,CB_SETCURSEL,provider,0);models(e);}
    e->settings={};e->settings.provider=provider;e->settings.model="";e->settings.key=utf8(draft);e->http=0;e->error.clear();e->fetchedModels.clear();e->cancel.reset();e->connecting=true;e->busy=true;e->start=std::chrono::steady_clock::now();e->seconds=0;GUID id{};CoCreateGuid(&id);wchar_t g[40];StringFromGUID2(id,g,40);e->runId=utf8(g);appendLog(e,"models_start");refreshControls(e);status(e,cloud::tr(L"接続先を自動判別し、画像モデルを取得しています。画像生成は行いません。"));SetTimer(e->hwnd,1,1000,nullptr);
    e->worker=std::thread([e,provider,key=e->settings.key]{try{e->fetchedModels=listImageModels(provider,key,e->cancel);e->http=200;}catch(const ApiError& ex){e->http=ex.status;e->error=localizeError(wide(ex.what()));}catch(const std::exception& ex){e->error=localizeError(wide(ex.what()));}catch(...){e->error=cloud::tr(L"モデル一覧の取得に失敗しました。");}PostMessageW(e->hwnd,WM_APP+2,0,0);});
}
static void start(Editor* e){if(e->busy)return;if(!connected(e))throw std::runtime_error("先にAPIキーを入力し「送信・モデル取得」を押してください。");saveFields(e);auto& s=e->session;Settings cfg;cfg.provider=s.provider;cfg.model=s.availableModels[s.provider].at(s.model[s.provider]);cfg.prompt=utf8(s.prompt);cfg.key=utf8(s.keys[s.provider]);configureOperation(cfg,s.mode);
    makeRequest(cfg,cfg.edit?Bytes{1}:Bytes{},!e->positionHint.empty()?Bytes{1}:Bytes{});
    e->requestImage=baseImage(e);e->settings=cfg;e->reply={};e->http=0;e->error.clear();e->cancel.reset();e->start=std::chrono::steady_clock::now();GUID id{};CoCreateGuid(&id);wchar_t g[40];StringFromGUID2(id,g,40);e->runId=utf8(g);e->seconds=0;appendLog(e,"start");e->busy=true;refreshControls(e);status(e,cloud::tr(L"生成中… 0秒。APIへ送信しています。"));SetTimer(e->hwnd,1,1000,nullptr);
    e->worker=std::thread([e,cfg]{try{e->reply=generate(cfg,e->requestImage,e->cancel,e->positionHint,e->session.reference);e->http=e->reply.http;}catch(const ApiError& ex){e->http=ex.status;e->error=localizeError(wide(ex.what()));}catch(const std::exception& ex){e->error=localizeError(wide(ex.what()));}catch(...){e->error=cloud::tr(L"生成中に予期しないエラーが発生しました。");}PostMessageW(e->hwnd,WM_APP+1,0,0);});
}
// Native multiline EDIT does not support EM_SETCUEBANNER; paint the cue without adding it to the input value.
static LRESULT CALLBACK promptWindow(HWND h,UINT msg,WPARAM wp,LPARAM lp){auto e=(Editor*)GetWindowLongPtrW(h,GWLP_USERDATA);auto original=e->promptProc;
    if(msg==WM_KEYDOWN&&wp=='A'&&(GetKeyState(VK_CONTROL)&0x8000)){SendMessageW(h,EM_SETSEL,0,-1);return 0;}
    if(msg==WM_NCDESTROY){SetWindowLongPtrW(h,GWLP_WNDPROC,(LONG_PTR)original);SetWindowLongPtrW(h,GWLP_USERDATA,0);return CallWindowProcW(original,h,msg,wp,lp);}
    auto result=CallWindowProcW(original,h,msg,wp,lp);
    if(msg==WM_PAINT&&GetWindowTextLengthW(h)==0){HDC dc=GetDC(h);auto old=SelectObject(dc,(HFONT)SendMessageW(h,WM_GETFONT,0,0));SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(120,136,155));RECT r{};SendMessageW(h,EM_GETRECT,0,(LPARAM)&r);r.left+=3;r.top+=2;DrawTextW(dc,promptPlaceholder(e->session.mode),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);ReleaseDC(h,dc);}return result;
}
static LRESULT CALLBACK sourceWindow(HWND h,UINT msg,WPARAM wp,LPARAM lp){auto e=(Editor*)GetWindowLongPtrW(h,GWLP_USERDATA);auto original=e->sourceProc;
    if(msg==WM_NCDESTROY){SetWindowLongPtrW(h,GWLP_WNDPROC,(LONG_PTR)original);SetWindowLongPtrW(h,GWLP_USERDATA,0);return CallWindowProcW(original,h,msg,wp,lp);}
    if(msg==WM_SETCURSOR&&!e->busy){SetCursor(LoadCursorW(nullptr,IDC_CROSS));return TRUE;}
    if(msg==WM_CAPTURECHANGED){e->drawing=false;return 0;}
    if(msg==WM_LBUTTONUP){e->drawing=false;if(GetCapture()==h)ReleaseCapture();return 0;}
    if(!e->busy&&(msg==WM_LBUTTONDOWN||(msg==WM_MOUSEMOVE&&e->drawing))){try{
        RECT r{};GetClientRect(h,&r);int x=0,y=0;int mx=(short)LOWORD(lp),my=(short)HIWORD(lp);auto& im=e->input.image;
        if(!previewPoint(r.right,r.bottom,im.w,im.h,mx,my,x,y))return 0;
        if(msg==WM_LBUTTONDOWN){e->positionHint.assign((size_t)im.w*im.h,0);e->drawing=true;e->lastX=x;e->lastY=y;SetCapture(h);}
        markPosition(e->positionHint,im.w,im.h,e->lastX,e->lastY,x,y,std::max(1,std::min(im.w,im.h)/14));e->lastX=x;e->lastY=y;
        refreshPreview(e);refreshControls(e);status(e,cloud::tr(L"ピンクは位置の目安です。人物・物の形や大きさはこの線に合わせません。"));return 0;
    }catch(const std::exception& ex){e->drawing=false;if(GetCapture()==h)ReleaseCapture();status(e,localizeError(wide(ex.what())));return 0;}}
    return CallWindowProcW(original,h,msg,wp,lp);
}
static INT_PTR CALLBACK dialogImpl(HWND h,UINT msg,WPARAM wp,LPARAM lp){Editor* e=(Editor*)GetWindowLongPtrW(h,DWLP_USER);
    if(msg==WM_MEASUREITEM){auto m=(MEASUREITEMSTRUCT*)lp;if(m->CtlType==ODT_COMBOBOX){m->itemHeight=22;return TRUE;}}
    if(msg==WM_INITDIALOG){e=(Editor*)lp;e->hwnd=h;SetWindowLongPtrW(h,DWLP_USER,(LONG_PTR)e);auto& s=e->session;
        populate(h,ID_PROVIDER,{L"OpenAI",L"Google Gemini"},s.provider);populate(h,ID_MODE,{cloud::tr(L"追加"),cloud::tr(L"置き換え"),cloud::tr(L"消去")},s.mode);populate(h,ID_BASE,{cloud::tr(L"元レイヤーから編集"),cloud::tr(L"表示中の候補から続ける")},0);models(e);SetDlgItemTextW(h,ID_PROMPT,s.prompt.c_str());HWND source=GetDlgItem(h,ID_SOURCE),prompt=GetDlgItem(h,ID_PROMPT);SetWindowLongPtrW(source,GWLP_USERDATA,(LONG_PTR)e);SetWindowLongPtrW(prompt,GWLP_USERDATA,(LONG_PTR)e);e->sourceProc=(WNDPROC)SetWindowLongPtrW(source,GWLP_WNDPROC,(LONG_PTR)sourceWindow);e->promptProc=(WNDPROC)SetWindowLongPtrW(prompt,GWLP_WNDPROC,(LONG_PTR)promptWindow);SendDlgItemMessageW(h,ID_KEY,EM_SETLIMITTEXT,4096,0);SendDlgItemMessageW(h,ID_PROMPT,EM_SETLIMITTEXT,16000,0);SendMessageW(h,DM_SETDEFID,ID_CONNECT,0);applyLanguage(e);fitWindow(e);refreshPreview(e);status(e,!e->history.current().empty()?cloud::tr(L"保持中の候補を復元しました。再生成せず、このまま適用できます。"):connected(e)?cloud::tr(L"接続済み。変更したいことを入力し、右下の「生成する」で開始。"):cloud::tr(L"APIキーを入力して送信。接続後、右下の「生成する」で開始。"));return TRUE;
    }
    if(!e)return FALSE;
    if(msg==WM_CTLCOLORDLG)return (INT_PTR)e->background;
    if(msg==WM_CTLCOLORSTATIC||msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX||msg==WM_CTLCOLORBTN){HDC dc=(HDC)wp;HWND control=(HWND)lp;int id=GetDlgCtrlID(control);bool entry=msg==WM_CTLCOLOREDIT||msg==WM_CTLCOLORLISTBOX||id==ID_KEY||id==ID_PROMPT;SetBkColor(dc,entry?color::input:color::background);SetTextColor(dc,IsWindowEnabled(control)?(id==201?color::text:id==ID_STATUS?color::accent:entry?color::text:color::muted):color::disabled);return (INT_PTR)(entry?e->inputBrush:e->background);}
    if(msg==WM_DRAWITEM){paintControl(e,(DRAWITEMSTRUCT*)lp);return TRUE;}
    if(msg==WM_TIMER){int secs=(int)std::chrono::duration<double>(std::chrono::steady_clock::now()-e->start).count();status(e,e->cancel.cancelled?cloud::tr(L"中止処理中…"):(e->connecting?cloud::tr(L"モデル取得中… "):cloud::tr(L"生成中… "))+std::to_wstring(secs)+cloud::tr(L"秒。中止ボタンで通信を止められます。"));return TRUE;}
    if(msg==WM_APP+2){if(e->worker.joinable())e->worker.join();KillTimer(h,1);e->seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-e->start).count();e->busy=false;e->connecting=false;auto& s=e->session;
        if(e->cancel.cancelled){status(e,cloud::tr(L"モデル取得を中止しました。入力したキーと既存候補は保持しています。"));appendLog(e,"models_cancelled");}
        else if(!e->error.empty()){status(e,e->error+cloud::tr(L" 入力したキーは保持しています。"));appendLog(e,"models_failed");}
        else{s.availableModels[s.provider]=std::move(e->fetchedModels);s.connectedKeys[s.provider]=s.keys[s.provider];s.model[s.provider]=0;models(e);status(e,s.availableModels[s.provider].empty()?cloud::tr(L"接続成功。このAPIには対応画像モデルが見つかりませんでした。"):cloud::tr(L"接続成功。画像モデル ")+std::to_wstring(s.availableModels[s.provider].size())+cloud::tr(L"件を取得しました。最新の対応モデルを選択しています。"));appendLog(e,"models_success");}
        if(!e->settings.key.empty())SecureZeroMemory(e->settings.key.data(),e->settings.key.size());e->settings.key.clear();keyNote(e);refreshControls(e);return TRUE;
    }
    if(msg==WM_APP+1){if(e->worker.joinable())e->worker.join();KillTimer(h,1);e->seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-e->start).count();
        if(e->cancel.cancelled){status(e,cloud::tr(L"中止しました。送信済みの生成は課金される場合があります。"));appendLog(e,"cancelled");}
        else if(!e->error.empty()){status(e,e->error+(e->candidate().empty()?L"":cloud::tr(L" 前の候補は保持しています。")));appendLog(e,"failed");}
        else{try{Candidate next;next.natural=e->settings.natural;next.tile=placeResult(e->input,e->reply.image,next.natural,e->alphaLocked);next.runId=e->runId;next.model=e->settings.model;next.provider=e->settings.provider;next.seconds=e->seconds;next.quality=e->reply.quality;next.complexity=e->reply.complexity;e->history.push(std::move(next));status(e,e->reply.fallbackCount?cloud::tr(L"生成完了。APIの設定未対応のため互換設定を使用しました。結果の画質表示を確認してください。"):cloud::tr(L"生成完了。取り消し／やり直しで比較できます。最大8候補を保持。"));appendLog(e,"success");InvalidateRect(GetDlgItem(h,ID_RESULT),nullptr,TRUE);}catch(const std::exception& ex){status(e,localizeError(wide(ex.what())));appendLog(e,"image_processing_failed");}}
        e->busy=false;refreshControls(e);refreshPreview(e);if(!e->settings.key.empty())SecureZeroMemory(e->settings.key.data(),e->settings.key.size());e->settings.key.clear();return TRUE;
    }
    if(msg==WM_CLOSE){if(e->busy){e->cancel.stop();refreshControls(e);return TRUE;}saveFields(e);EndDialog(h,IDCANCEL);return TRUE;}
    if(msg==WM_COMMAND){int id=LOWORD(wp),notification=HIWORD(wp);
        if(id==ID_PROMPT&&notification==EN_CHANGE){InvalidateRect(GetDlgItem(h,ID_PROMPT),nullptr,TRUE);return TRUE;}
        if(id==ID_KEY&&notification==EN_CHANGE){keyNote(e);refreshControls(e);return TRUE;}
        if(id==135&&notification==CBN_SELCHANGE&&!e->busy){saveFields(e);uiLanguage=selection(h,135);applyLanguage(e);models(e);status(e,uiLanguage?L"Language changed. Your prompt, connections and candidates are retained.":L"表示言語を変更しました。文章・接続・生成候補は保持しています。");return TRUE;}
        if(id==136){
            HMENU menu=CreatePopupMenu();
            const wchar_t* namesJa[]={L"プラグインの使い方",L"OpenAI APIの準備",L"Gemini APIの準備"};
            const wchar_t* namesEn[]={L"Plugin manual",L"OpenAI API setup",L"Gemini API setup"};
            const wchar_t* files[]={L"AI-Image-Bridge-Manual.pdf",L"OpenAI-API-Setup.pdf",L"Gemini-API-Setup.pdf"};
            for(int i=0;i<3;i++)AppendMenuW(menu,MF_STRING,701+i,uiLanguage?namesEn[i]:namesJa[i]);
            RECT rect{};GetWindowRect(GetDlgItem(h,136),&rect);
            int choice=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_LEFTALIGN|TPM_TOPALIGN,rect.left,rect.bottom,0,h,nullptr);DestroyMenu(menu);
            if(choice>=701&&choice<=703){auto path=e->logPath.substr(0,e->logPath.find_last_of(L"\\/"))+L"\\manuals\\"+files[choice-701];
                if(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)status(e,uiLanguage?L"Manual PDF not found. Reinstall the complete package, including the manuals folder.":L"PDFが見つかりません。manualsフォルダーを含む配布一式を再インストールしてください。");
                else if((INT_PTR)ShellExecuteW(h,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL)<=32)status(e,uiLanguage?L"Could not open the PDF. Choose a default PDF viewer in Windows.":L"PDFを開けませんでした。Windowsの既定のPDF閲覧アプリを確認してください。");
            }return TRUE;
        }
        if(id==ID_CONNECT){connectModels(e);return TRUE;}
        if(id==ID_MODEL&&notification==CBN_SELCHANGE){saveFields(e);refreshControls(e);return TRUE;}
        if(id==ID_PROVIDER&&notification==CBN_SELCHANGE){saveFields(e);e->session.provider=selection(h,ID_PROVIDER);models(e);status(e,cloud::tr(L"接続先を切り替えました。入力済みキーは接続先ごとに保持します。"));return TRUE;}
        if(id==ID_MODE&&notification==CBN_SELCHANGE){saveFields(e);refreshControls(e);refreshPreview(e);status(e,cloud::tr(L"入力欄の例文を切り替えました。入力済みの文章はそのまま残ります。"));return TRUE;}
        if(id==ID_BASE&&notification==CBN_SELCHANGE){refreshPreview(e);return TRUE;}
        if(id==ID_GUIDE&&!e->busy){e->positionHint.clear();refreshPreview(e);refreshControls(e);status(e,cloud::tr(L"位置指定を解除しました。文章だけでも生成できます。"));return TRUE;}
        if((id==ID_PREVIOUS||id==ID_REDO)&&!e->busy){if(id==ID_PREVIOUS)e->history.undo();else e->history.redo();refreshControls(e);refreshPreview(e);InvalidateRect(GetDlgItem(h,ID_RESULT),nullptr,TRUE);status(e,e->candidate().empty()?cloud::tr(L"生成前の元画像に戻しました。「やり直し」で候補を復元できます。"):cloud::tr(L"生成履歴 ")+std::to_wstring(e->history.position())+L" / "+std::to_wstring(e->history.count())+cloud::tr(L"。表示中の結果が適用されます。"));return TRUE;}
        if(id==ID_GENERATE){start(e);return TRUE;}
        if(id==ID_CANCEL_RUN){if(!emergencyStopEnabled(e->busy,e->cancel.cancelled))return TRUE;e->cancel.stop();refreshControls(e);status(e,cloud::tr(L"中止処理中…"));return TRUE;}
        if(id==ID_CLEAR_KEY&&!e->busy){if(GetWindowTextLengthW(GetDlgItem(h,ID_KEY))==0){status(e,cloud::tr(L"消去するキーはありません。"));return TRUE;}if(MessageBoxW(h,cloud::tr(L"この接続先のAPIキーを消去しますか？\n送信する場合は「いいえ」で戻り、「送信・モデル取得」を押してください。"),cloud::tr(L"APIキーの消去"),MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return TRUE;for(auto* k:{&e->session.keys[e->session.provider],&e->session.connectedKeys[e->session.provider]}){if(!k->empty())SecureZeroMemory(k->data(),k->size()*sizeof(wchar_t));k->clear();}e->session.availableModels[e->session.provider].clear();e->session.model[e->session.provider]=0;models(e);status(e,cloud::tr(L"選択中の接続先のキーを消去しました。"));return TRUE;}
        if(id==ID_REF_CLEAR){e->session.reference.clear();e->session.referenceName.clear();e->referencePreview={};refreshControls(e);InvalidateRect(GetDlgItem(h,ID_REF_PREVIEW),nullptr,TRUE);return TRUE;}
        if(id==ID_REF_ADD&&!e->busy){wchar_t name[32768]{};OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=h;o.lpstrFilter=cloud::tr(L"画像 (PNG / JPEG / WebP / BMP)\0*.png;*.jpg;*.jpeg;*.webp;*.bmp\0すべて\0*.*\0");o.lpstrFile=name;o.nMaxFile=32768;o.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(GetOpenFileNameW(&o)){auto image=readImageFile(name);auto png=encodePng(image);std::wstring path=name;e->session.referenceName=path.substr(path.find_last_of(L"\\/")+1);e->session.reference=std::move(png);e->referencePreview=std::move(image);refreshControls(e);InvalidateRect(GetDlgItem(h,ID_REF_PREVIEW),nullptr,TRUE);status(e,cloud::tr(L"見本を追加しました。例：「この服を着た女の子を左に追加」と指示します。"));}return TRUE;}
        if(id==ID_APPLY&&!e->busy&&!e->candidate().empty()){saveFields(e);
            if(e->input.selected&&e->candidate().natural){e->session.pending=e->candidate();e->session.pendingInput=e->input;MessageBoxW(h,cloud::tr(L"候補をメモリに保持して、この画面を閉じます。\n\n1. クリスタで Ctrl+D を押して選択を解除\n2. フィルター → AI Image Bridge をもう一度開く\n3. 復元された候補を「適用」\n\n再生成・追加課金はありません。元画像と同じレイヤーで操作してください。\n選択範囲の外へ自然にはみ出す部分も、そのまま反映できます。"),cloud::tr(L"自然な配置を適用するには"),MB_OK|MB_ICONINFORMATION);appendLog(e,"pending_apply");EndDialog(h,IDCANCEL);return TRUE;}
            e->output=e->candidate().tile;e->session.pending={};e->session.pendingInput={};appendLog(e,"apply");EndDialog(h,IDOK);return TRUE;}
        if(id==IDCANCEL||id==ID_CLOSE){SendMessageW(h,WM_CLOSE,0,0);return TRUE;}
        if(id==ID_SAVE&&!e->candidate().empty()&&!e->busy){wchar_t name[32768]=L"ai-image-bridge.png";OPENFILENAMEW o{};o.lStructSize=sizeof(o);o.hwndOwner=h;o.lpstrFilter=cloud::tr(L"PNG画像\0*.png\0");o.lpstrFile=name;o.nMaxFile=32768;o.lpstrDefExt=L"png";o.Flags=OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(GetSaveFileNameW(&o)){writeBytes(name,encodePng(e->candidate().tile.image));status(e,cloud::tr(L"表示中の候補をPNG保存しました。レイヤーへの適用はまだ行いません。"));}return TRUE;}
    }return FALSE;
}
static INT_PTR CALLBACK dialog(HWND h,UINT msg,WPARAM wp,LPARAM lp){try{return dialogImpl(h,msg,wp,lp);}catch(const std::exception& ex){auto e=(Editor*)GetWindowLongPtrW(h,DWLP_USER);if(e){status(e,localizeError(wide(ex.what())));if(!e->worker.joinable()){e->busy=false;KillTimer(h,1);refreshControls(e);}}return TRUE;}catch(...){return FALSE;}}
bool showEditor(HINSTANCE instance,HWND owner,SessionState& session,const ReadInput& reader,bool alphaLocked,Tile& output,const std::wstring& path){Editor e{nullptr,session,reader,alphaLocked,output,path};e.input=reader(2);if(e.input.selected)e.positionHint=e.input.mask;if(!session.pending.empty()&&sameInput(e.input,session.pendingInput))e.history.push(session.pending);if(!session.reference.empty())e.referencePreview=decodeImage(session.reference);INT_PTR result=DialogBoxParamW(instance,MAKEINTRESOURCEW(200),owner,dialog,(LPARAM)&e);if(result==-1)throw std::runtime_error("生成画面を開けませんでした。");return result==IDOK;}
}

