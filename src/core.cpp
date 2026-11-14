#include "generated_version.h"
#include "core.h"
#include <wincodec.h>
#include <wrl/client.h>
#include <wincrypt.h>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <memory>

namespace cloud {
using Microsoft::WRL::ComPtr;
static void check(HRESULT r) {if(FAILED(r)) throw std::runtime_error("画像の変換に失敗しました。");}
struct ComInit { HRESULT hr=CoInitializeEx(nullptr,COINIT_MULTITHREADED); ~ComInit(){if(SUCCEEDED(hr))CoUninitialize();} };
static ComPtr<IWICImagingFactory> factory(){ComPtr<IWICImagingFactory> f;check(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&f)));return f;}
std::string utf8(const std::wstring& s){if(s.empty())return {};int n=WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0,nullptr,nullptr);std::string r(n,0);WideCharToMultiByte(CP_UTF8,0,s.data(),(int)s.size(),r.data(),n,nullptr,nullptr);return r;}
std::wstring wide(const std::string& s){if(s.empty())return {};int n=MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),nullptr,0);std::wstring r(n,0);MultiByteToWideChar(CP_UTF8,0,s.data(),(int)s.size(),r.data(),n);return r;}
std::string base64(const Bytes& b){DWORD n=0;if(!CryptBinaryToStringA(b.data(),(DWORD)b.size(),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,nullptr,&n))throw std::runtime_error("Base64 encode failed");std::string s(n,0);CryptBinaryToStringA(b.data(),(DWORD)b.size(),CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,s.data(),&n);s.resize(n);return s;}
Bytes unbase64(const std::string& s){if(s.empty()||s.size()>90*1024*1024)throw std::runtime_error("画像データが空、または上限を超えています。");DWORD n=0;DWORD flags=CRYPT_STRING_BASE64|CRYPT_STRING_STRICT;if(!CryptStringToBinaryA(s.data(),(DWORD)s.size(),flags,nullptr,&n,nullptr,nullptr)||n==0)throw std::runtime_error("画像のBase64データが不正です。");Bytes b(n);if(!CryptStringToBinaryA(s.data(),(DWORD)s.size(),flags,b.data(),&n,nullptr,nullptr))throw std::runtime_error("画像のBase64データが不正です。");b.resize(n);return b;}
static void validateImage(const Image& im){if(im.w<1||im.h<1||im.w>16384||im.h>16384||(uint64_t)im.w*im.h>33554432||im.rgba.size()!=(size_t)im.w*im.h*4)throw std::runtime_error("画像サイズが不正です。範囲を小さくして試してください。");}
Bytes encodePng(const Image& im){
    validateImage(im);ComInit init;auto f=factory();ComPtr<IStream> stream;check(CreateStreamOnHGlobal(nullptr,TRUE,&stream));
    ComPtr<IWICBitmapEncoder> enc;check(f->CreateEncoder(GUID_ContainerFormatPng,nullptr,&enc));check(enc->Initialize(stream.Get(),WICBitmapEncoderNoCache));
    ComPtr<IWICBitmapFrameEncode> frame;check(enc->CreateNewFrame(&frame,nullptr));check(frame->Initialize(nullptr));check(frame->SetSize(im.w,im.h));
    WICPixelFormatGUID fmt=GUID_WICPixelFormat32bppBGRA;check(frame->SetPixelFormat(&fmt));if(fmt!=GUID_WICPixelFormat32bppBGRA)throw std::runtime_error("PNGのピクセル形式が未対応です。");
    Bytes bgra=im.rgba;for(size_t p=0;p<bgra.size();p+=4)std::swap(bgra[p],bgra[p+2]);
    check(frame->WritePixels(im.h,im.w*4,(UINT)bgra.size(),bgra.data()));check(frame->Commit());check(enc->Commit());
    STATSTG stat{};check(stream->Stat(&stat,STATFLAG_NONAME));if(stat.cbSize.QuadPart>64*1024*1024)throw std::runtime_error("入力画像が64MBを超えています。");
    LARGE_INTEGER zero{};check(stream->Seek(zero,STREAM_SEEK_SET,nullptr));Bytes out((size_t)stat.cbSize.QuadPart);ULONG read=0;check(stream->Read(out.data(),(ULONG)out.size(),&read));if(read!=out.size())throw std::runtime_error("PNG読み取りに失敗しました。");return out;
}
Image decodeImage(const Bytes& b){
    if(b.empty()||b.size()>64*1024*1024)throw std::runtime_error("返された画像のサイズが不正です。");ComInit init;auto f=factory();ComPtr<IWICStream> stream;check(f->CreateStream(&stream));check(stream->InitializeFromMemory(const_cast<BYTE*>(b.data()),(DWORD)b.size()));
    ComPtr<IWICBitmapDecoder> dec;check(f->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnDemand,&dec));ComPtr<IWICBitmapFrameDecode> frame;check(dec->GetFrame(0,&frame));UINT w,h;check(frame->GetSize(&w,&h));
    if(w<1||h<1||w>16384||h>16384||(uint64_t)w*h>33554432)throw std::runtime_error("返された画像が画素数の上限を超えています。");
    ComPtr<IWICFormatConverter> conv;check(f->CreateFormatConverter(&conv));check(conv->Initialize(frame.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
    Image out{(int)w,(int)h,Bytes((size_t)w*h*4)};check(conv->CopyPixels(nullptr,w*4,(UINT)out.rgba.size(),out.rgba.data()));return out;
}
Image resizeImage(const Image& im,int w,int h){
    validateImage(im);if(w<1||h<1||w>16384||h>16384||(uint64_t)w*h>33554432)throw std::runtime_error("配置先の画像サイズが不正です。");
    if(im.w==w&&im.h==h)return im;Image out{w,h,Bytes((size_t)w*h*4)};ComInit init;auto f=factory();ComPtr<IWICBitmap> bm;check(f->CreateBitmapFromMemory(im.w,im.h,GUID_WICPixelFormat32bppRGBA,im.w*4,(UINT)im.rgba.size(),const_cast<BYTE*>(im.rgba.data()),&bm));
    // WIC's scaler may change the channel order. Scale premultiplied pixels,
    // then explicitly convert back to our straight RGBA representation.
    ComPtr<IWICFormatConverter> premult;check(f->CreateFormatConverter(&premult));check(premult->Initialize(bm.Get(),GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
    ComPtr<IWICBitmapScaler> sc;check(f->CreateBitmapScaler(&sc));check(sc->Initialize(premult.Get(),w,h,WICBitmapInterpolationModeFant));
    ComPtr<IWICFormatConverter> rgba;check(f->CreateFormatConverter(&rgba));check(rgba->Initialize(sc.Get(),GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom));
    check(rgba->CopyPixels(nullptr,w*4,(UINT)out.rgba.size(),out.rgba.data()));return out;
}
Image blendSelection(const Image& original,const Image& generated,const Bytes& mask,bool alphaLocked){
    validateImage(original);validateImage(generated);if(original.w!=generated.w||original.h!=generated.h||mask.size()!=(size_t)original.w*original.h)throw std::runtime_error("選択範囲と画像の寸法が一致しません。");
    Image out=original;for(size_t p=0;p<mask.size();p++){
        unsigned m=mask[p];if(!m)continue;
        if(alphaLocked){if(original.rgba[p*4+3])for(int c=0;c<3;c++)out.rgba[p*4+c]=(unsigned char)((original.rgba[p*4+c]*(255-m)+generated.rgba[p*4+c]*m+127)/255);continue;}
        if(m==255){std::copy_n(&generated.rgba[p*4],4,&out.rgba[p*4]);continue;}
        unsigned a0=original.rgba[p*4+3],a1=generated.rgba[p*4+3];unsigned denom=a0*(255-m)+a1*m;
        for(int c=0;c<3;c++)out.rgba[p*4+c]=denom?(unsigned char)((original.rgba[p*4+c]*a0*(255-m)+generated.rgba[p*4+c]*a1*m+denom/2)/denom):0;
        out.rgba[p*4+3]=(unsigned char)((denom+127)/255);
    }return out;
}
void Cancel::close(){auto r=request.exchange(nullptr);if(r)WinHttpCloseHandle(r);}
Rect contextRect(Rect selection,Rect bounds,int mode){
    if(mode<0||mode>2||bounds.right<=bounds.left||bounds.bottom<=bounds.top||selection.right<=selection.left||selection.bottom<=selection.top)throw std::runtime_error("参照範囲が不正です。");
    if(mode==2)return bounds;long long pad=mode==1?128:0;
    Rect r{(int)std::max<long long>(bounds.left,(long long)selection.left-pad),(int)std::max<long long>(bounds.top,(long long)selection.top-pad),(int)std::min<long long>(bounds.right,(long long)selection.right+pad),(int)std::min<long long>(bounds.bottom,(long long)selection.bottom+pad)};
    if(r.right<=r.left||r.bottom<=r.top)throw std::runtime_error("選択範囲がレイヤー領域の外です。");return r;
}
Image selectionMarkup(const Image& im,const Bytes& mask){
    validateImage(im);if(mask.size()!=(size_t)im.w*im.h)throw std::runtime_error("選択ガイドの寸法が一致しません。");
    Image out{im.w,im.h,Bytes(im.rgba.size(),0)};int radius=std::max(1,std::min(im.w,im.h)/256);
    auto selected=[&](int x,int y){return x>=0&&y>=0&&x<im.w&&y<im.h&&mask[(size_t)y*im.w+x]>0;};
    for(int y=0;y<im.h;y++)for(int x=0;x<im.w;x++){size_t p=(size_t)y*im.w+x;if(!mask[p])continue;
        bool edge=!selected(x-radius,y)||!selected(x+radius,y)||!selected(x,y-radius)||!selected(x,y+radius);
        out.rgba[p*4]=255;out.rgba[p*4+1]=68;out.rgba[p*4+2]=104;out.rgba[p*4+3]=edge?230:(unsigned char)(mask[p]*26/255);
    }return out;
}
Image annotateImage(const Image& im,const Image& markup){
    validateImage(im);validateImage(markup);if(im.w!=markup.w||im.h!=markup.h)throw std::runtime_error("ガイドの寸法が一致しません。");Image out=im;
    for(size_t p=0;p<im.rgba.size();p+=4){unsigned a=markup.rgba[p+3];for(int c=0;c<3;c++)out.rgba[p+c]=(unsigned char)((im.rgba[p+c]*(255-a)+markup.rgba[p+c]*a+127)/255);out.rgba[p+3]=(unsigned char)(a+(im.rgba[p+3]*(255-a)+127)/255);}return out;
}
Image readImageFile(const std::wstring& path){
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw std::runtime_error("参考画像を開けません。");auto n=f.tellg();if(n<=0||n>64*1024*1024)throw std::runtime_error("参考画像は64MB以下の画像を選んでください。");f.seekg(0);Bytes b((size_t)n);if(!f.read((char*)b.data(),n))throw std::runtime_error("参考画像を読み取れません。");return decodeImage(b);
}
void Cancel::stop(){cancelled=true;close();}
void Cancel::reset(){close();cancelled=false;}
static void add(Bytes& body,const std::string& s){body.insert(body.end(),s.begin(),s.end());}
static bool in(const std::string& value,std::initializer_list<const char*> choices){for(auto c:choices)if(value==c)return true;return false;}
static void validateKey(const std::string& key){if(key.empty())throw std::runtime_error("APIキーを入力してください。");for(unsigned char c:key)if(c<=32||c>=127)throw std::runtime_error("APIキーに空白または不正な文字が含まれています。");}
int detectProvider(const std::string& key,int fallback){validateKey(key);if(key.rfind("sk-",0)==0)return 0;if(key.rfind("AIza",0)==0)return 1;if(fallback<0||fallback>1)throw std::runtime_error("接続先を選択してください。");return fallback;}
bool supportedImageModel(int provider,const std::string& id){
    if(id.empty()||id.size()>128||id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._")!=std::string::npos)return false;
    if(provider==0)return id.rfind("gpt-image-2.6",0)==0&&(id.size()==13||id[13]=='-');
    return provider==1&&id.rfind("gemini-",0)==0&&(id.find("image")!=std::string::npos||id.find("nano-banana")!=std::string::npos);
}
Request modelListRequest(int provider,const std::string& key,const std::string& pageToken){
    validateKey(key);Request r;r.method=L"GET";
    if(provider==0){if(!pageToken.empty())throw std::runtime_error("OpenAIモデル一覧のページ指定が不正です。");r.host=L"api.openai.com";r.path=L"/v1/models";r.headers=L"Authorization: Bearer "+wide(key)+L"\r\n";}
    else if(provider==1){r.host=L"generativelanguage.googleapis.com";r.path=L"/v1beta/models?pageSize=1000";r.headers=L"x-goog-api-key: "+wide(key)+L"\r\n";if(!pageToken.empty()){if(pageToken.size()>4096)throw std::runtime_error("モデル一覧のページ情報が大きすぎます。");std::string escaped;const char hex[]="0123456789ABCDEF";for(unsigned char c:pageToken){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')escaped+=c;else{escaped+='%';escaped+=hex[c>>4];escaped+=hex[c&15];}}r.path+=L"&pageToken="+wide(escaped);}}
    else throw std::runtime_error("接続先が不正です。");return r;
}
std::vector<std::string> parseModelList(int provider,const Json& data){
    const char* field=provider==0?"data":"models";if(provider<0||provider>1||!data.contains(field)||!data[field].is_array())throw std::runtime_error("APIのモデル一覧形式を読み取れません。");std::vector<std::string> result;
    for(const auto& m:data[field]){if(!m.is_object())continue;std::string id=m.value(provider==0?"id":"name",std::string{});if(provider==1){if(id.rfind("models/",0)!=0)continue;id.erase(0,7);auto methods=m.find("supportedGenerationMethods");if(methods==m.end()||!methods->is_array()||std::find(methods->begin(),methods->end(),Json("generateContent"))==methods->end())continue;}
        if(supportedImageModel(provider,id)&&std::find(result.begin(),result.end(),id)==result.end())result.push_back(id);
    }return result;
}
void orderImageModels(int provider,std::vector<std::string>& models){
    // Current official image families: Nano Banana supersedes the earlier Gemini image family.
    // Compare numeric releases naturally (2.10 > 2.9), keeping the current OpenAI aliases ahead of dated snapshots.
    auto newer=[](const std::string& a,const std::string& b){size_t i=0,j=0;while(i<a.size()&&j<b.size()){bool an=a[i]>='0'&&a[i]<='9',bn=b[j]>='0'&&b[j]<='9';if(an&&bn){size_t ai=i,bj=j;while(i<a.size()&&a[i]>='0'&&a[i]<='9')i++;while(j<b.size()&&b[j]>='0'&&b[j]<='9')j++;auto av=a.substr(ai,i-ai),bv=b.substr(bj,j-bj);if(av.size()!=bv.size())return av.size()>bv.size();if(av!=bv)return av>bv;}else{if(a[i]!=b[j])return a[i]>b[j];i++;j++;}}return a.size()>b.size();};
    std::stable_sort(models.begin(),models.end(),[&](const auto& a,const auto& b){if(provider==0){auto alias=[](const auto& id){return id=="gpt-image-2.6-flare"||id=="gpt-image-2.6-sunburst";};if(alias(a)!=alias(b))return alias(a);if(alias(a)&&alias(b))return a=="gpt-image-2.6-sunburst"&&b!=a;}else{bool na=a.rfind("gemini-nano-banana-",0)==0,nb=b.rfind("gemini-nano-banana-",0)==0;if(na!=nb)return na;}return newer(a,b);});
}
Request makeRequest(const Settings& s,const Bytes& png,const Bytes& guide,const Bytes& reference){
    if(s.prompt.find_first_not_of(" \r\n\t")==std::string::npos&&!s.remove)throw std::runtime_error("生成・編集の指示を入力してください。");
    if(s.key.empty())throw std::runtime_error("選択したプロバイダーのAPIキーを入力してください。");
    for(unsigned char c:s.key)if(c<=32||c>=127)throw std::runtime_error("APIキーに空白または不正な文字が含まれています。");
    if(s.edit&&png.empty())throw std::runtime_error("編集用の画像がありません。");
    if(!s.edit&&(s.remove||!guide.empty()||!reference.empty()))throw std::runtime_error("選択ガイドと参考画像は編集モードで使用してください。");
    if(s.remove&&guide.empty())throw std::runtime_error("消去する対象をクリスタで選択してください。");
    if(s.natural&&(!s.edit||s.remove))throw std::runtime_error("自然な配置には元レイヤーの画像が必要です。");
    std::string prompt=s.prompt;
    if(s.edit){
        prompt="Edit the FIRST image as the base canvas. Preserve its framing, camera position, aspect ratio, and all unrelated subjects. Return only the finished unmarked image, at the same composition.\n"+prompt;
        if(!guide.empty())prompt+=(s.provider==0?"\nThe attached transparent markup marks the requested location in pink/red.":"\nThe SECOND image is the same canvas with a pink/red location guide, NOT an appearance reference.");
        if(s.natural){
            prompt+="\nPerform the requested edit naturally in this scene. The marked region, when supplied, is a rough POSITION hint, NOT a silhouette, cutout, or bounding box to fill. Use natural anatomy, body proportions, pose, and height appropriate to the other subjects and scene perspective. A natural pose may extend beyond the guide. Ground subjects naturally in the scene. Do not stretch, compress, crop, or shape a subject to match the guide. Preserve unrelated people, background and composition.";
            if(s.operation==0)prompt+="\nAdd the requested subject or object near the indicated location, keeping existing subjects unless the user asks otherwise.";
            if(s.operation==1)prompt+="\nReplace or change the indicated existing subject/object as requested. Do not keep a duplicate of the replaced subject. Preserve its scene role and placement unless the user asks otherwise.";
            if(s.operation==2)prompt+="\nRemove the indicated subject/object and its associated shadow, reconstructing a continuous natural background. Do not add a replacement subject. Use the guide only to identify the target; remove the complete target even if it extends outside the guide.";
        }
        else if(!guide.empty())prompt+="\nEdit only the marked region; preserve the rest.";
        if(s.remove)prompt+="\nRemove the subject/object inside the marked area, including its shadow when marked, and reconstruct a continuous plausible background from its surroundings. Do not add a replacement subject.";
        if(!guide.empty())prompt+="\nDo not include any colored guide, annotation or outline in the result.";
        if(!reference.empty())prompt+="\nThe LAST additional image is an appearance/reference image. Use it only as requested in the user instructions; keep the base canvas composition.";
    }
    Request r;
    if(s.provider==0){
        if(!supportedImageModel(0,s.model)||!in(s.quality,{"low","medium","high","xhigh","max"})||!in(s.complexity,{"","low","medium","high"}))throw std::runtime_error("OpenAIの設定が不正です。");
        r.host=L"api.openai.com";r.path=s.edit?L"/v1/images/edits":L"/v1/images/generations";
        r.headers=L"Authorization: Bearer "+wide(s.key)+L"\r\n";
        Json fields={{"model",s.model},{"prompt",prompt},{"size","auto"},{"quality",s.quality},{"complexity",s.complexity},{"output_format","png"}};
        if(s.complexity.empty())fields.erase("complexity");
        if(s.edit){
            GUID id{};CoCreateGuid(&id);wchar_t uuid[40]{};StringFromGUID2(id,uuid,40);std::string boundary="CloudImage"+utf8(uuid);
            r.headers+=L"Content-Type: multipart/form-data; boundary="+wide(boundary)+L"\r\n";
            for(auto it=fields.begin();it!=fields.end();++it)add(r.body,"--"+boundary+"\r\nContent-Disposition: form-data; name=\""+it.key()+"\"\r\n\r\n"+it.value().get<std::string>()+"\r\n");
            auto file=[&](const char* field,const char* name,const Bytes& bytes){add(r.body,"--"+boundary+"\r\nContent-Disposition: form-data; name=\""+field+"\"; filename=\""+name+"\"\r\nContent-Type: image/png\r\n\r\n");r.body.insert(r.body.end(),bytes.begin(),bytes.end());add(r.body,"\r\n");};
            file("image[]","canvas.png",png);if(!reference.empty())file("image[]","reference.png",reference);if(!guide.empty())file("markup","selection-guide.png",guide);add(r.body,"--"+boundary+"--\r\n");
        }else{r.headers+=L"Content-Type: application/json\r\n";add(r.body,fields.dump());}
    }else if(s.provider==1){
        if(!supportedImageModel(1,s.model)||!in(s.imageSize,{"1K","2K","4K"}))throw std::runtime_error("Geminiのモデル・解像度設定が不正です。");
        r.host=L"generativelanguage.googleapis.com";r.path=L"/v1beta/models/"+wide(s.model)+L":generateContent";r.headers=L"Content-Type: application/json\r\nx-goog-api-key: "+wide(s.key)+L"\r\n";
        Json parts=Json::array({{{"text",prompt}}});for(const auto* bytes:{&png,&guide,&reference})if(s.edit&&!bytes->empty())parts.push_back({{"inlineData",{{"mimeType","image/png"},{"data",base64(*bytes)}}}});
        Json body={{"contents",Json::array({{{"role","user"},{"parts",parts}}})},{"generationConfig",{{"responseModalities",Json::array({"TEXT","IMAGE"})},{"imageConfig",{{"imageSize",s.imageSize}}}}}};if(s.model.rfind("gemini-2.5-",0)==0)body["generationConfig"].erase("imageConfig");add(r.body,body.dump());
    }else throw std::runtime_error("プロバイダーが不正です。");
    return r;
}
Bytes parseImageResponse(int provider,const Json& j){
    if(provider==0){if(j.contains("data")&&j["data"].is_array())for(const auto& item:j["data"])if(item.contains("b64_json")&&item["b64_json"].is_string()&&!item["b64_json"].get_ref<const std::string&>().empty())return unbase64(item["b64_json"].get<std::string>());}
    else if(provider==1&&j.contains("candidates")&&j["candidates"].is_array()){
        for(const auto& candidate:j["candidates"]){if(!candidate.contains("content")||!candidate["content"].contains("parts")||!candidate["content"]["parts"].is_array())continue;
            for(const auto& part:candidate["content"]["parts"]){if(part.value("thought",false))continue;const char* key=part.contains("inlineData")?"inlineData":"inline_data";
                if(!part.contains(key)||!part[key].is_object())continue;const auto& data=part[key];std::string mime=data.value("mimeType",data.value("mime_type",std::string{}));
                if(mime.rfind("image/",0)==0&&data.contains("data")&&data["data"].is_string())return unbase64(data["data"].get<std::string>());
            }
        }
    }
    throw std::runtime_error("APIは画像を返しませんでした。入力やプロバイダーの制限を確認してください。自動再試行はしていません。");
}
struct InternetHandle{HINTERNET h;explicit InternetHandle(HINTERNET v):h(v){if(!v)throw ApiError(0,"通信の初期化に失敗しました。");}~InternetHandle(){if(h)WinHttpCloseHandle(h);}InternetHandle(const InternetHandle&)=delete;};
static void internetCheck(BOOL ok,Cancel& cancel){if(!ok){DWORD error=GetLastError();if(cancel.cancelled)throw ApiError(0,"中止しました。");throw ApiError(0,"通信に失敗しました (Windows "+std::to_string(error)+")。自動再試行はしていません。");}}
struct HttpReply{Json body;int status;std::string requestId;};
std::string unsupportedParameter(int status,const Json& data){
    if(status!=400||!data.is_object()||!data.contains("error")||!data["error"].is_object())return {};
    const auto& e=data["error"];auto code=e.value("code",Json());auto param=e.value("param",Json());
    if(!code.is_string()||!param.is_string())return {};
    if(code!="unsupported_value"&&code!="unsupported_parameter")return {};
    const auto p=param.get<std::string>();return p=="quality"||p=="complexity"?p:std::string{};
}
bool capabilityFallback(Settings& s,const ApiError& error){
    if(error.status!=400||s.provider!=0)return false;
    if(error.unsupportedParameter=="quality"&&s.quality=="max"){s.quality="high";return true;}
    if(error.unsupportedParameter=="complexity"&&s.complexity=="high"){s.complexity.clear();return true;}
    return false;
}
static HttpReply sendHttp(const Request& r,Cancel& cancel){
    if(r.body.size()>128*1024*1024)throw std::runtime_error("送信データが128MBを超えています。参照範囲や参考画像を小さくしてください。");if(cancel.cancelled)throw ApiError(0,"中止しました。");
    InternetHandle session(WinHttpOpen(L"CloudImageFilter/" AIB_VERSION_WIDE,WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,WINHTTP_NO_PROXY_NAME,WINHTTP_NO_PROXY_BYPASS,0));
    WinHttpSetTimeouts(session.h,15000,15000,60000,300000);InternetHandle conn(WinHttpConnect(session.h,r.host.c_str(),INTERNET_DEFAULT_HTTPS_PORT,0));
    HINTERNET req=WinHttpOpenRequest(conn.h,r.method.c_str(),r.path.c_str(),nullptr,WINHTTP_NO_REFERER,WINHTTP_DEFAULT_ACCEPT_TYPES,WINHTTP_FLAG_SECURE);if(!req)throw ApiError(0,"通信要求を作成できませんでした。");
    cancel.request=req;struct Cleanup{Cancel& c;~Cleanup(){c.close();}} cleanup{cancel};if(cancel.cancelled){cancel.close();throw ApiError(0,"中止しました。");}
    DWORD redirect=WINHTTP_OPTION_REDIRECT_POLICY_NEVER;WinHttpSetOption(req,WINHTTP_OPTION_REDIRECT_POLICY,&redirect,sizeof(redirect));
    internetCheck(WinHttpSendRequest(req,r.headers.c_str(),(DWORD)-1,r.body.empty()?nullptr:(void*)r.body.data(),(DWORD)r.body.size(),(DWORD)r.body.size(),0),cancel);
    internetCheck(WinHttpReceiveResponse(req,nullptr),cancel);DWORD code=0,size=sizeof(code);internetCheck(WinHttpQueryHeaders(req,WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,nullptr,&code,&size,nullptr),cancel);
    Bytes response;DWORD got=0;unsigned char buf[65536];while(true){internetCheck(WinHttpReadData(req,buf,sizeof(buf),&got),cancel);if(!got)break;if(response.size()+got>64*1024*1024)throw ApiError(code,"API応答が64MBの上限を超えました。");response.insert(response.end(),buf,buf+got);}
    if(code<200||code>=300){std::string detail=code==401?"APIキーを確認してください。":code==403?"モデルの利用権限や組織認証を確認してください。":code==429?"利用上限・残高・混雑を確認してください。":code==400?"APIキー・画像・モデル・生成設定を確認してください。":"プロバイダーが要求を完了できませんでした。";throw ApiError(code,"HTTP "+std::to_string(code)+"："+detail+" 自動再試行はしていません。",unsupportedParameter(code,Json::parse(response.begin(),response.end(),nullptr,false)));}
    if(cancel.cancelled)throw ApiError(0,"中止しました。");Json j=Json::parse(response.begin(),response.end(),nullptr,false);if(j.is_discarded())throw ApiError(code,"APIの応答がJSONではありません。");
    HttpReply out{std::move(j),(int)code,{}};
    wchar_t rid[256]{};DWORD ridSize=sizeof(rid);if(WinHttpQueryHeaders(req,WINHTTP_QUERY_CUSTOM,L"x-request-id",rid,&ridSize,nullptr))out.requestId=utf8(rid);
    return out;
}
std::vector<std::string> listImageModels(int provider,const std::string& key,Cancel& cancel){
    std::vector<std::string> result,tokens;std::string next;
    for(int page=0;page<20;page++){auto response=sendHttp(modelListRequest(provider,key,next),cancel);auto models=parseModelList(provider,response.body);for(auto& id:models)if(std::find(result.begin(),result.end(),id)==result.end())result.push_back(id);
        next=provider==1?response.body.value("nextPageToken",std::string{}):std::string{};if(next.empty()){orderImageModels(provider,result);return result;}
        if(std::find(tokens.begin(),tokens.end(),next)!=tokens.end())throw std::runtime_error("モデル一覧のページ情報が循環しています。");tokens.push_back(next);
    }throw std::runtime_error("モデル一覧のページ数が上限を超えました。取得を完了できませんでした。");
}
Reply generate(const Settings& s,const Image& input,Cancel& cancel,const Bytes& mask,const Bytes& reference){
    if(cancel.cancelled)throw ApiError(0,"中止しました。");Bytes png=s.edit?encodePng(input):Bytes{},guide;
    if(s.edit&&!mask.empty()){auto markup=selectionMarkup(input,mask);guide=encodePng(s.provider==0?markup:annotateImage(input,markup));}
    Settings effective=s;HttpReply response;int fallbacks=0;
    for(;;){try{response=sendHttp(makeRequest(effective,png,guide,s.edit?reference:Bytes{}),cancel);break;}catch(const ApiError& error){if(cancel.cancelled||fallbacks>=2||!capabilityFallback(effective,error))throw;++fallbacks;}}
    Reply out;out.http=response.status;out.image=decodeImage(parseImageResponse(s.provider,response.body));out.usage=response.body.value(s.provider==0?"usage":"usageMetadata",Json::object());out.requestId=response.requestId;out.quality=effective.quality;out.complexity=effective.complexity;out.imageSize=effective.imageSize;out.fallbackCount=fallbacks;return out;
}
void writeBytes(const std::wstring& path,const Bytes& b){std::ofstream f(path,std::ios::binary);if(!f||!f.write((const char*)b.data(),b.size()))throw std::runtime_error("ファイルを保存できませんでした。");}
}


