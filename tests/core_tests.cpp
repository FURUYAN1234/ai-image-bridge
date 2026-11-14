#include "../src/ui.h"
#include "../src/ui.h"
#include <iostream>
#include <functional>
#include <thread>
#include <chrono>
using namespace cloud;
static int total=0;
void require(bool x,const char* message){++total;if(!x)throw std::runtime_error(message);}
void rejects(std::function<void()> fn,const char* message){bool rejected=false;try{fn();}catch(const std::exception&){rejected=true;}require(rejected,message);}
int main(int argc,char** argv){try{
    uiLanguage=1;require(std::wstring(promptPlaceholder(0)).find(L"Example:")==0,"English placeholder is localized");
    require(localizeError(L"APIキーを確認してください。 自動再試行はしていません。")==L"Check your API key. No automatic retry was made.","composed API errors are localized");
    uiLanguage=0;require(std::wstring(promptPlaceholder(0)).find(L"例：")==0,"Japanese placeholder restored");

    Settings defaults;require(defaults.model=="gpt-image-2.6-sunburst"&&defaults.quality=="max"&&defaults.complexity=="high","highest quality defaults must be Sunburst max high");
    defaults.key="test-placeholder";defaults.prompt="選択範囲の中で女の子が立って手を振っている";defaults.natural=true;
    auto naturalRequest=makeRequest(defaults,{1,2,3},{4,5,6});std::string naturalBody(naturalRequest.body.begin(),naturalRequest.body.end());
    require(naturalRequest.path==L"/v1/images/edits"&&naturalBody.find("filename=\"canvas.png\"")!=std::string::npos&&naturalBody.find("name=\"markup\"")!=std::string::npos,"new subject uses actual canvas and position guide, never text-only generation");
    require(naturalBody.find("rough POSITION hint")!=std::string::npos&&naturalBody.find("natural anatomy")!=std::string::npos&&naturalBody.find("Edit only the marked region")==std::string::npos,"natural placement does not constrain body to cutout");
    require(naturalBody.find("Create the requested new subject")==std::string::npos,"natural edit must not force addition for replacements and removals");
    defaults.edit=false;rejects([&]{makeRequest(defaults,{});},"natural placement cannot silently drop scene");defaults.edit=true;
    for(int operation=0;operation<3;operation++){Settings edit;edit.key="unit-test-placeholder";edit.prompt="change the indicated subject";configureOperation(edit,operation);auto request=makeRequest(edit,{1},{2});std::string body(request.body.begin(),request.body.end());require(edit.natural&&!edit.remove&&body.find("rough POSITION hint")!=std::string::npos,"all three operations use natural full-image editing");if(operation==1)require(body.find("Do not keep a duplicate")!=std::string::npos,"replacement removes original target");if(operation==2)require(body.find("remove the complete target even if it extends outside")!=std::string::npos,"removal is not clipped to hint");}
    require(std::wstring(promptPlaceholder(0))!=promptPlaceholder(1)&&std::wstring(promptPlaceholder(1))!=promptPlaceholder(2),"operation cue text is distinct");
    Settings emptyEdit;emptyEdit.key="test-placeholder";configureOperation(emptyEdit,2);rejects([&]{makeRequest(emptyEdit,{1});},"placeholder is not submitted as user instructions");rejects([&]{configureOperation(emptyEdit,9);},"invalid operation rejected");
    for(auto control:{std::pair<int,int>{498,343},{249,343},{234,171},{117,85},{44,44},{1,1}}){
        for(auto image:{std::pair<int,int>{2240,3078},{3078,2240},{1000,1000}}){
            auto r=previewRect(control.first,control.second,image.first,image.second);
            require(r.width>0&&r.height>0&&r.left>=0&&r.top>=0&&r.left+r.width<=control.first&&r.top+r.height<=control.second,"contain rectangle stays inside scaled control");
            if(std::min(control.first,control.second)>=13)require(r.left>=6&&r.top>=6&&r.left+r.width<=control.first-6&&r.top+r.height<=control.second-6,"six pixel image gutter on all sides");
            int x=-1,y=-1;require(previewPoint(control.first,control.second,image.first,image.second,r.left,r.top,x,y)&&x==0&&y==0,"top left source maps consistently");
            require(previewPoint(control.first,control.second,image.first,image.second,r.left+r.width-1,r.top+r.height-1,x,y)&&x<image.first&&y<image.second,"bottom right preview maps inside full source");
            require(!previewPoint(control.first,control.second,image.first,image.second,r.left+r.width,r.top,x,y),"right edge gutter cannot paint a hint");
            if(r.top>0)require(!previewPoint(control.first,control.second,image.first,image.second,r.left,r.top-1,x,y),"upper gutter cannot paint a hint");
            if(r.left>0)require(!previewPoint(control.first,control.second,image.first,image.second,r.left-1,r.top,x,y),"left gutter cannot paint a hint");
        }
    }
    int px=0,py=0;require(previewPoint(400,400,1000,500,200,200,px,py)&&px==500&&py==250,"preview click maps through letterboxing to actual image");require(!previewPoint(400,400,1000,500,200,50,px,py)&&!previewPoint(400,400,1000,500,400,200,px,py),"letterbox and right boundary cannot create misplaced hints");
    Bytes rough(100,0);markPosition(rough,10,10,0,0,9,9,1);require(rough[0]==255&&rough[99]==255&&rough[55]==255&&rough[9]==0,"open stroke is continuous and clipped safely at canvas edges");rejects([&]{markPosition(rough,10,10,-1,0,9,9,1);},"invalid stroke point rejected");rejects([&]{markPosition(rough,11,10,0,0,9,9,1);},"wrong-sized hint cannot corrupt memory");
    Json unsupported={{"error",{{"code","unsupported_value"},{"param","quality"}}}};
    require(unsupportedParameter(400,unsupported)=="quality"&&unsupportedParameter(429,unsupported).empty(),"only explicit capability refusal qualifies for fallback");
    require(unsupportedParameter(400,{{"error",{{"code","content_policy_violation"},{"param","quality"}}}}).empty()&&unsupportedParameter(400,{{"error","malformed"}}).empty(),"safety and malformed errors do not trigger fallback");
    require(!capabilityFallback(defaults,ApiError(401,"auth","quality"))&&!capabilityFallback(defaults,ApiError(400,"generic")),"auth or generic failures do not reduce quality or retry");
    require(capabilityFallback(defaults,ApiError(400,"unsupported","quality"))&&defaults.quality=="high"&&!capabilityFallback(defaults,ApiError(400,"unsupported","quality")),"quality compatibility fallback is bounded to one transition");
    require(capabilityFallback(defaults,ApiError(400,"unsupported","complexity"))&&!capabilityFallback(defaults,ApiError(400,"unsupported","complexity")),"complexity capability fallback is bounded");
    naturalRequest=makeRequest(defaults,{1,2,3},{4,5,6});naturalBody.assign(naturalRequest.body.begin(),naturalRequest.body.end());require(naturalBody.find("name=\"complexity\"")==std::string::npos,"unsupported complexity omitted on compatibility request");
    CandidateHistory history;Candidate first;first.model="first";first.tile.image={1,1,{1,2,3,255}};first.tile.mask={255};history.push(first);history.undo();require(history.current().empty()&&!history.canUndo()&&history.canRedo(),"first generated candidate can be undone to source");history.redo();require(history.current().model=="first","redo restores same candidate without API");
    Candidate second=first;second.model="second";history.push(second);history.undo();require(history.current().model=="first","multi-step history goes back one generation");history.push(first);require(!history.canRedo(),"new generation after Undo discards redo branch");for(int i=0;i<10;i++)history.push(first);require(history.count()==8&&history.canUndo(),"history bounded to newest eight candidates");
    Tile original;original.rect={0,0,2,1};original.image={2,1,{10,10,10,255,20,20,20,255}};original.mask={255,0};original.selected=true;Image newSubject{2,1,{30,30,30,255,40,40,40,255}};
    auto naturalPlacement=placeResult(original,newSubject,true,false),strictPlacement=placeResult(original,newSubject,false,false);require(naturalPlacement.image.rgba==newSubject.rgba&&naturalPlacement.mask==Bytes({255,255})&&!naturalPlacement.selected,"natural result keeps body outside rough selection intact");require(strictPlacement.image.rgba[4]==20,"ordinary edit still preserves hard selection exterior");
    Tile reopened=original;reopened.selected=false;reopened.mask={255,255};require(sameInput(original,reopened),"pending candidate can restore after selection cleared");reopened.image.rgba[0]++;require(!sameInput(original,reopened),"changed source cannot receive stale pending candidate");
    Image src{2,2,{255,0,0,255, 0,255,0,128, 0,0,255,255, 0,0,0,0}};auto png=encodePng(src);auto decoded=decodeImage(png);require(decoded.rgba==src.rgba,"PNG must preserve straight RGBA and alpha");
    require(unbase64(base64(png))==png,"Base64 binary round trip");rejects([]{unbase64("!not-base64!");},"invalid base64 must fail");rejects([]{decodeImage({1,2,3});},"nonimage must fail");
    Image gen{2,2,{0,0,255,255, 255,255,255,255, 255,0,0,0, 255,255,255,255}};auto blend=blendSelection(src,gen,{0,255,128,0});require(std::equal(blend.rgba.begin(),blend.rgba.begin()+4,src.rgba.begin()),"outside mask unchanged");require(blend.rgba[4]==255&&blend.rgba[7]==255,"inside mask replaced");require(blend.rgba[10]==255&&blend.rgba[11]==127,"feather blends in premultiplied alpha without dark fringe");require(blend.rgba[15]==0,"transparent outside preserved");rejects([&]{blendSelection(src,gen,{255});},"mask length rejected");
    auto locked=blendSelection(src,gen,{255,255,128,255},true);require(locked.rgba[7]==128&&locked.rgba[11]==255,"alpha lock preserves existing alpha");require(std::equal(locked.rgba.begin()+12,locked.rgba.end(),src.rgba.begin()+12),"alpha lock protects transparent pixel colors");require(locked.rgba[4]==255&&locked.rgba[8]==128,"alpha lock still edits colors with selection feather");
    auto resized=resizeImage(src,16,16);require(resized.w==16&&resized.h==16&&resized.rgba.size()==1024,"resize exact target dimensions");
    Image solid{2,2,{200,60,20,255,200,60,20,255,200,60,20,255,200,60,20,255}};
    auto scaledSolid=resizeImage(solid,1,1);require(scaledSolid.rgba==Bytes({200,60,20,255}),"resize preserves RGBA channel order");
    auto edge=resizeImage(Image{2,1,{255,0,0,255,0,0,255,0}},1,1);require(edge.rgba[0]>=253&&edge.rgba[1]==0&&edge.rgba[2]==0&&edge.rgba[3]>=127&&edge.rgba[3]<=128,"transparent blue does not contaminate resized red edge");
    rejects([&]{resizeImage(src,-1,10);},"negative target dimensions rejected before allocation");
    auto crop=contextRect({20,30,70,80},{0,0,512,512},0);require(crop.left==20&&crop.top==30&&crop.right==70&&crop.bottom==80,"selection-only context");
    crop=contextRect({20,30,70,80},{-40,-50,512,512},1);require(crop.left==-40&&crop.top==-50&&crop.right==198&&crop.bottom==208,"padded context clipped, negative origin preserved");
    crop=contextRect({20,30,70,80},{-40,-50,512,512},2);require(crop.left==-40&&crop.bottom==512,"full layer context");
    rejects([]{contextRect({600,600,700,700},{0,0,512,512},0);},"nonintersecting selection rejected");rejects([]{contextRect({0,0,4,4},{0,0,10,10},3);},"unknown context rejected");
    Image maskBase{9,9,Bytes(9*9*4,255)};Bytes shape(81,0);for(int y=1;y<8;y++)for(int x=1;x<8;x++)shape[y*9+x]=255;shape[4*9+4]=0;shape[1*9+1]=128;
    auto guide=selectionMarkup(maskBase,shape);require(guide.w==9&&guide.h==9,"guide matches canvas dimensions");require(guide.rgba[3]==0&&guide.rgba[(4*9+4)*4+3]==0,"guide preserves exterior and selection holes");require(guide.rgba[(2*9+2)*4+3]==26&&guide.rgba[(1*9+4)*4+3]==230,"guide distinguishes interior and boundary");
    auto annotated=annotateImage(maskBase,guide);require(maskBase.rgba[5]==255&&annotated.rgba[5]==255&&annotated.rgba[(1*9+4)*4+1]<255,"annotation retains original and affects only marked pixels");rejects([&]{selectionMarkup(maskBase,{1});},"invalid guide mask rejected");rejects([&]{annotateImage(src,guide);},"mismatched annotation rejected");
    Image replacement=maskBase;std::fill(replacement.rgba.begin(),replacement.rgba.end(),0);for(size_t i=3;i<replacement.rgba.size();i+=4)replacement.rgba[i]=255;auto irregular=blendSelection(maskBase,replacement,shape);require(irregular.rgba[(4*9+4)*4]==255&&irregular.rgba[0]==255&&irregular.rgba[(1*9+1)*4]==127,"irregular mask holes exterior and feather preserved");
    rejects([]{readImageFile(L"Z:\\not-a-real-cloud-image-file.png");},"missing reference fails before generation");
    Settings s;s.key="unit-test-placeholder";s.prompt="背景を青に\nkeep geometry";auto req=makeRequest(s,png);std::string body(req.body.begin(),req.body.end());require(req.host==L"api.openai.com"&&req.path==L"/v1/images/edits","OpenAI official endpoint");require(body.find("gpt-image-2.6-sunburst")!=std::string::npos&&body.find("name=\"image[]\"")!=std::string::npos,"2.6 edit multipart");require(body.find("unit-test-placeholder")==std::string::npos,"key excluded from body");require(body.find("input_fidelity")==std::string::npos,"2.6 omits unsupported fidelity");
    require(detectProvider("sk-test-placeholder",1)==0&&detectProvider("AIza-test-placeholder",0)==1,"provider auto-detected before endpoint selection");require(detectProvider("unknown-placeholder",1)==1,"unknown formats respect explicit provider");rejects([]{detectProvider("",0);},"empty key cannot connect");
    auto listing=modelListRequest(0,"unit-test-placeholder");require(listing.method==L"GET"&&listing.path==L"/v1/models"&&listing.body.empty(),"OpenAI discovery is non-generating GET");listing=modelListRequest(1,"unit-test-placeholder","x+&/?=");require(listing.path==L"/v1beta/models?pageSize=1000&pageToken=x%2B%26%2F%3F%3D"&&listing.path.find(L"unit-test-placeholder")==std::wstring::npos,"Gemini pagination encoded and key absent from URL");rejects([]{modelListRequest(1,"bad\r\nkey");},"discovery blocks header injection");
    auto discovered=parseModelList(0,{{"data",Json::array({{{"id","gpt-image-2.6-flare"}},{{"id","gpt-image-2.6-sunburst"}},{{"id","gpt-image-2.6-flare"}},{{"id","gpt-6.1"}},{{"id","gpt-image-2.6/evil"}}})}});require(discovered==std::vector<std::string>({"gpt-image-2.6-flare","gpt-image-2.6-sunburst"}),"discovery filters duplicates text models unsafe IDs");
    std::vector<std::string> versions={"gemini-2.5-flash-image","gemini-nano-banana-2.9","gemini-3-pro-image","gemini-nano-banana-2.10"};orderImageModels(1,versions);require(versions[0]=="gemini-nano-banana-2.10"&&versions[1]=="gemini-nano-banana-2.9"&&versions[2]=="gemini-3-pro-image","default follows newest available numeric release");versions={"gemini-2.5-flash-image","gemini-3-pro-image"};orderImageModels(1,versions);require(versions[0]=="gemini-3-pro-image","latest available model when newest family is unavailable");versions={"gpt-image-2.6-sunburst","gpt-image-2.6-flare-2026-10-01","gpt-image-2.6-flare"};orderImageModels(0,versions);require(versions[0]=="gpt-image-2.6-sunburst","current OpenAI alias defaults ahead of older snapshots");
    discovered=parseModelList(1,{{"models",Json::array({{{"name","models/gemini-nano-banana-2.1"},{"supportedGenerationMethods",Json::array({"generateContent"})}},{{"name","models/gemini-3-pro-image"},{"supportedGenerationMethods",Json::array({"generateContent"})}},{{"name","models/gemini-2.5-flash-image"},{"supportedGenerationMethods",Json::array({"generateContent"})}},{{"name","models/gemini-image-no-generation"},{"supportedGenerationMethods",Json::array({"embedContent"})}},{{"name","models/imagen-4"},{"supportedGenerationMethods",Json::array({"predict"})}}})}});require(discovered.size()==3&&discovered[2]=="gemini-2.5-flash-image","Gemini discovery requires image family and generateContent support");rejects([]{parseModelList(0,{{"data","broken"}});},"malformed model list rejected");require(parseModelList(0,{{"data",Json::array()}}).empty(),"authenticated empty list is not replaced by invented models");
    s.edit=false;req=makeRequest(s,{});auto j=Json::parse(req.body);require(j["complexity"]=="high"&&j["quality"]=="max"&&req.path==L"/v1/images/generations","generation fields");s.prompt="  \n";rejects([&]{makeRequest(s,{});},"empty prompt rejected");s.prompt="test";s.key="key\r\ninjection";rejects([&]{makeRequest(s,{});},"header injection rejected");s.key="unit-test-placeholder";
    rejects([&]{makeRequest(s,{},png,png);},"new generation rejects edit attachments");s.edit=true;s.remove=true;s.prompt="";rejects([&]{makeRequest(s,png);},"removal requires selected target");req=makeRequest(s,png,encodePng(guide),png);body=std::string(req.body.begin(),req.body.end());require(body.find("name=\"markup\"")!=std::string::npos&&body.find("name=\"mask\"")==std::string::npos,"OpenAI uses markup without legacy mask");require(body.find("filename=\"canvas.png\"")<body.find("filename=\"reference.png\"")&&body.find("Remove the subject/object")!=std::string::npos,"removal supports empty extra prompt, source precedes reference");s.remove=false;s.prompt="test";
    require(parseImageResponse(0,{{"data",Json::array({{{"b64_json",base64(png)}}})}})==png,"OpenAI image response");rejects([]{parseImageResponse(0,{{"data",Json::array()}});},"200 without image rejected");rejects([]{parseImageResponse(0,{{"data",Json::array({{{"url","https://untrusted.invalid/image"}}})}});},"unexpected URL not followed");
    s.provider=1;s.model="gemini-nano-banana-2.1";s.edit=true;req=makeRequest(s,png);j=Json::parse(req.body);require(req.host==L"generativelanguage.googleapis.com"&&req.path.find(L"key=")==std::wstring::npos,"Gemini key only in header");require(j["contents"][0]["parts"][1]["inlineData"]["mimeType"]=="image/png","Gemini image request");
    s.imageSize="2K";auto guidePng=encodePng(guide);req=makeRequest(s,png,guidePng,encodePng(annotated));j=Json::parse(req.body);auto parts=j["contents"][0]["parts"];require(parts.size()==4&&parts[1]["inlineData"]["data"]==base64(png)&&parts[2]["inlineData"]["data"]==base64(guidePng)&&parts[3]["inlineData"]["data"]==base64(encodePng(annotated)),"Gemini source guide reference order");require(j["generationConfig"]["imageConfig"]["imageSize"]=="2K","Gemini resolution honored");s.imageSize="8K";rejects([&]{makeRequest(s,png);},"unsupported Gemini resolution rejected");s.imageSize="1K";
    Json gem={{"candidates",Json::array({{{"content",{{"parts",Json::array({{{"thought",true},{"inlineData",{{"mimeType","image/png"},{"data","bad"}}}},{{"text","explanation"}},{{"inlineData",{{"mimeType","image/png"},{"data",base64(png)}}}}})}}}}})}};
    require(parseImageResponse(1,gem)==png,"ignore thinking images, use final image");rejects([]{parseImageResponse(1,{{"promptFeedback",{{"blockReason","SAFETY"}}}});},"blocked Gemini response rejected");
    s.provider=3;rejects([&]{makeRequest(s,png);},"unknown provider rejected");
    for(auto area:{std::pair<int,int>{1280,720},{800,600},{1920,1080}}){double f=fitClientScale(1560,1240,area.first,area.second,32,78);require(1560*f+32<=area.first-39 && 1240*f+78<=area.second-39,"scaled client plus native frame fits workarea");}
    require(!emergencyStopVisible(false,false),"startup without session key hides stop");
    require(emergencyStopVisible(true,false),"workready idle shows disabled stop");
    require(emergencyStopVisible(false,true),"active work retains stop during connection changes");
    require(!emergencyStopEnabled(false,false),"idle emergency stop disabled");
    require(emergencyStopEnabled(true,false),"running emergency stop enabled");
    require(!emergencyStopEnabled(true,true),"requested stop immediately disabled");
    require(!emergencyStopEnabled(false,true),"stopped emergency stop disabled");
    require(emergencyStopEnabled(true,false),"fresh request can stop again");
    Settings auth;auth.key="invalid-test-placeholder";auth.prompt="Test";auth.edit=false;
    Cancel stopped;stopped.stop();rejects([&]{generate(auth,src,stopped);},"cancelled request never proceeds");
    if(argc==2&&std::string(argv[1])=="--network-errors"){
        Cancel authCancel;int code=0;try{generate(auth,src,authCancel);}catch(const ApiError& e){code=e.status;}
        require(code==401,"real OpenAI invalid key returns 401");std::cout<<"OpenAI invalid-key HTTP "<<code<<"\n";
        Cancel running;bool rejected=false;std::thread worker([&]{try{generate(auth,src,running);}catch(const ApiError& e){rejected=running.cancelled&&e.status==0;}});
        for(int i=0;i<1000&&!running.request.load();i++)std::this_thread::sleep_for(std::chrono::milliseconds(1));
        running.stop();worker.join();require(rejected,"in-flight request cancellation is reported without an image");
        auth.provider=1;auth.model="gemini-nano-banana-2.1";Cancel gemCancel;code=0;try{generate(auth,src,gemCancel);}catch(const ApiError& e){code=e.status;}
        require(code==400||code==401||code==403,"real Gemini invalid key is rejected");std::cout<<"Gemini invalid-key HTTP "<<code<<" (generation not verified)\n";
        for(int provider:{0,1}){Cancel modelCancel;code=0;try{listImageModels(provider,"invalid-test-placeholder",modelCancel);}catch(const ApiError& e){code=e.status;}require(code==400||code==401||code==403,"real model discovery invalid key rejected");std::cout<<"Model discovery "<<provider<<" invalid-key HTTP "<<code<<"\n";}
    }
    std::cout<<"PASS "<<total<<" assertions\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<total<<" assertions: "<<e.what()<<"\n";return 1;}}

