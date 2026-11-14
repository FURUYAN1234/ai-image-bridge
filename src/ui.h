#pragma once
#include "core.h"
#include "i18n.h"
#include <functional>
#include <algorithm>
#include <cmath>
namespace cloud {
// Only the client area scales; title bar and frame remain native DPI dimensions.
inline double fitClientScale(int cw,int ch,int workW,int workH,int frameW,int frameH){
    if(cw<=0||ch<=0)return 1.0;
    return std::max(0.01,std::min({1.0,(double)std::max(1,workW-40-frameW)/cw,(double)std::max(1,workH-40-frameH)/ch}));
}

inline const wchar_t* promptPlaceholder(int operation){
    if(operation==1)return cloud::tr(L"例：この辺の男性を、手を振る女の子に置き換えて。\r\n残したい服装・ポーズ・背景も指定できます。");
    if(operation==2)return cloud::tr(L"例：この辺の人物と影を消して、背景を自然につないで。\r\n位置を指定しない場合は、消す対象を文章で示してください。");
    return cloud::tr(L"例：この辺に、立って手を振る女の子を追加して。\r\n元画像をクリック・なぞって位置の目安を指定できます。");
}
inline void configureOperation(Settings& settings,int operation){if(operation<0||operation>2)throw std::runtime_error("編集内容が不正です。");settings.edit=true;settings.natural=true;settings.remove=false;settings.operation=operation;}
struct PreviewRect {int left=0,top=0,width=0,height=0;};
inline PreviewRect previewRect(int cw,int ch,int iw,int ih){
    if(cw<1||ch<1||iw<1||ih<1)return {};
    const int pad=std::min(6,std::max(0,(std::min(cw,ch)-1)/2));
    const int availableW=cw-2*pad,availableH=ch-2*pad;
    const double scale=std::min((double)availableW/iw,(double)availableH/ih);
    const int w=std::min(availableW,std::max(1,(int)(iw*scale))),h=std::min(availableH,std::max(1,(int)(ih*scale)));
    return {(cw-w)/2,(ch-h)/2,w,h};
}
inline bool previewPoint(int cw,int ch,int iw,int ih,int mx,int my,int& x,int& y){
    const auto r=previewRect(cw,ch,iw,ih);
    if(r.width<1||r.height<1||mx<r.left||mx>=r.left+r.width||my<r.top||my>=r.top+r.height)return false;
    x=std::min(iw-1,(int)((double)(mx-r.left)*iw/r.width));y=std::min(ih-1,(int)((double)(my-r.top)*ih/r.height));return true;
}
inline void markPosition(Bytes& mask,int w,int h,int fromX,int fromY,int toX,int toY,int radius){
    if(w<1||h<1||mask.size()!=(size_t)w*h||radius<1||radius>std::max(w,h)||fromX<0||fromX>=w||toX<0||toX>=w||fromY<0||fromY>=h||toY<0||toY>=h)throw std::runtime_error("位置指定が不正です。");
    const int steps=std::max(abs(toX-fromX),abs(toY-fromY));
    for(int i=0;i<=steps;i++){double t=steps?(double)i/steps:0;int cx=(int)std::lround(fromX+(toX-fromX)*t),cy=(int)std::lround(fromY+(toY-fromY)*t);
        for(int y=std::max(0,cy-radius);y<std::min(h,cy+radius+1);y++)for(int x=std::max(0,cx-radius);x<std::min(w,cx+radius+1);x++)if((x-cx)*(x-cx)+(y-cy)*(y-cy)<=radius*radius)mask[(size_t)y*w+x]=255;
    }
}
struct Tile { Image image; Bytes mask; Rect rect; bool selected=false; };
inline Tile placeResult(const Tile& input,const Image& result,bool natural,bool alphaLocked){Tile output=input;auto placed=resizeImage(result,input.image.w,input.image.h);if(natural){output.mask.assign((size_t)placed.w*placed.h,255);output.selected=false;}output.image=blendSelection(input.image,placed,output.mask,alphaLocked);return output;}
struct Candidate {Tile tile;std::string runId,model,quality,complexity;int provider=0;double seconds=0;bool natural=false;bool empty()const{return tile.image.empty();}};
class CandidateHistory {
    std::vector<Candidate> states;size_t cursor=0;Candidate original;
public:
    const Candidate& current()const{return cursor?states[cursor-1]:original;}
    bool canUndo()const{return cursor>0;}
    bool canRedo()const{return cursor<states.size();}
    void undo(){if(canUndo())--cursor;}
    void redo(){if(canRedo())++cursor;}
    size_t position()const{return cursor;}
    size_t count()const{return states.size();}
    void push(Candidate next){
        states.erase(states.begin()+cursor,states.end());states.push_back(std::move(next));cursor=states.size();
        auto bytes=[&]{size_t n=0;for(const auto& c:states)n+=c.tile.image.rgba.size()+c.tile.mask.size();return n;};
        while(states.size()>1&&(states.size()>8||bytes()>256*1024*1024)){states.erase(states.begin());--cursor;}
    }
};
inline bool sameInput(const Tile& a,const Tile& b){return a.rect.left==b.rect.left&&a.rect.top==b.rect.top&&a.rect.right==b.rect.right&&a.rect.bottom==b.rect.bottom&&a.image.w==b.image.w&&a.image.h==b.image.h&&a.image.rgba==b.image.rgba;}
struct SessionState {
    std::wstring keys[2],connectedKeys[2],prompt,referenceName;
    std::vector<std::string> availableModels[2];
    Bytes reference;
    int provider=0,model[2]={0,0},mode=0;
    Candidate pending;Tile pendingInput;
};
using ReadInput=std::function<Tile(int)>;
bool showEditor(HINSTANCE instance,HWND owner,SessionState& session,const ReadInput& readInput,bool alphaLocked,Tile& output,const std::wstring& logPath);
}

namespace cloud { inline bool emergencyStopVisible(bool workReady,bool busy){return workReady||busy;}
inline bool emergencyStopEnabled(bool busy, bool cancelled) { return busy && !cancelled; }
inline COLORREF emergencyStopBackground(bool disabled,bool pressed) { return disabled?RGB(82,82,82):pressed?RGB(153,27,27):RGB(185,28,28); }
inline COLORREF emergencyStopBorder(bool disabled) { return disabled?RGB(163,163,163):RGB(185,28,28); }
inline std::wstring emergencyStopWaitHint() { return tr(L"秒。「緊急停止」で通信を止められます。"); }
 }
