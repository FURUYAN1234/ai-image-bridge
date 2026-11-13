#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#include <atomic>
#include <string>
#include <vector>
#include <stdexcept>
#include "../vendor/json.hpp"

namespace cloud {
using Bytes = std::vector<unsigned char>;
using Json = nlohmann::json;
struct Image { int w=0,h=0; Bytes rgba; bool empty() const {return rgba.empty();} };
struct Rect { int left=0,top=0,right=0,bottom=0; };
Rect contextRect(Rect selection,Rect bounds,int mode);
Image selectionMarkup(const Image& image,const Bytes& mask);
Image annotateImage(const Image& image,const Image& markup);
struct Settings {
    int provider=0;
    int operation=-1; // 0 add, 1 replace/edit, 2 remove; UI always uses natural placement.
    std::string model="gpt-image-2.6-sunburst", quality="max", complexity="high", prompt, key;
    bool edit=true,remove=false,natural=false;
    std::string imageSize="4K";
};
struct Request {std::wstring host,path,headers,method=L"POST"; Bytes body;};
int detectProvider(const std::string& key,int fallback);
bool supportedImageModel(int provider,const std::string& id);
Request modelListRequest(int provider,const std::string& key,const std::string& pageToken={});
std::vector<std::string> parseModelList(int provider,const Json& data);
void orderImageModels(int provider,std::vector<std::string>& models);
struct Reply {Image image; Json usage; int http=0; std::string requestId,quality,complexity,imageSize;int fallbackCount=0;};
struct Cancel {
    std::atomic<bool> cancelled{false};
    std::atomic<HINTERNET> request{nullptr};
    void stop();
    void close();
    void reset();
};
struct ApiError : std::runtime_error {
    int status;
    std::string unsupportedParameter;
    ApiError(int s,const std::string& m,std::string p={}):std::runtime_error(m),status(s),unsupportedParameter(std::move(p)){}
};
std::string unsupportedParameter(int status,const Json& error);
bool capabilityFallback(Settings& settings,const ApiError& error);
std::vector<std::string> listImageModels(int provider,const std::string& key,Cancel& cancel);
std::string utf8(const std::wstring& s);
std::wstring wide(const std::string& s);
std::string base64(const Bytes& b);
Bytes unbase64(const std::string& s);
Bytes encodePng(const Image& image);
Image decodeImage(const Bytes& bytes);
Image resizeImage(const Image& image,int width,int height);
Image blendSelection(const Image& original,const Image& generated,const Bytes& mask,bool alphaLocked=false);
Request makeRequest(const Settings& s,const Bytes& png,const Bytes& guide={},const Bytes& reference={});
Bytes parseImageResponse(int provider,const Json& response);
Reply generate(const Settings& s,const Image& input,Cancel& cancel,const Bytes& mask={},const Bytes& reference={});
Image readImageFile(const std::wstring& path);
void writeBytes(const std::wstring& path,const Bytes& bytes);
}
