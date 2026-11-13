// Public ABI assertions and callback dispatch tests; no SDK required in default build.
#include "../src/host.h"
#include "../vendor/compat/FilterPlugIn.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
using namespace FilterPlugIn;
static_assert(sizeof(Int)==4 && sizeof(Bool)==1 && sizeof(UniChar)==2);
static_assert(sizeof(Rect)==16 && sizeof(Point)==8 && alignof(Server)==1);
static_assert(sizeof(RecordSuite)==256*sizeof(void*));
static_assert(sizeof(ServiceSuite)==256*sizeof(void*));
static_assert(sizeof(Server)==513*sizeof(void*));
static_assert(sizeof(FilterRunRecord)==19*sizeof(void*));
#define SLOT(type,member,index) static_assert(offsetof(type,member)==index*sizeof(void*))
SLOT(RecordSuite,filterInitializeRecord,5); SLOT(RecordSuite,filterRunRecord,6);
SLOT(ServiceSuite,offscreenService,2);
SLOT(FilterRunRecord,isLayerMaskSelectedProc,5); SLOT(FilterRunRecord,isAlphaLockedProc,6);
SLOT(FilterRunRecord,getSourceOffscreenProc,7); SLOT(FilterRunRecord,getDestinationOffscreenProc,8);
SLOT(FilterRunRecord,getSelectAreaRectProc,9); SLOT(FilterRunRecord,hasSelectAreaOffscreenProc,10);
SLOT(FilterRunRecord,getSelectAreaOffscreenProc,11); SLOT(FilterRunRecord,updateDestinationOffscreenRectProc,12);
SLOT(FilterRunRecord,processProc,16); SLOT(FilterRunRecord,setProgressTotalProc,17); SLOT(FilterRunRecord,setProgressDoneProc,18);
SLOT(OffscreenService,getRectProc,5); SLOT(OffscreenService,getRGBChannelIndexProc,8);
SLOT(OffscreenService,getBlockImageProc,12); SLOT(OffscreenService,getBlockAlphaProc,13); SLOT(OffscreenService,getBlockSelectAreaProc,14);
#ifdef AIB_OFFICIAL_SDK
#define MATCH(type) static_assert(sizeof(FilterPlugIn::type)==sizeof(TriglavPlugIn##type))
MATCH(Server); MATCH(RecordSuite); MATCH(ServiceSuite); MATCH(FilterRunRecord); MATCH(FilterInitializeRecord); MATCH(ModuleInitializeRecord); MATCH(StringService); MATCH(OffscreenService);
#define FIELD(type,member) static_assert(offsetof(FilterPlugIn::type,member)==offsetof(TriglavPlugIn##type,member))
FIELD(FilterRunRecord,isLayerMaskSelectedProc); FIELD(FilterRunRecord,hasSelectAreaOffscreenProc); FIELD(FilterRunRecord,processProc); FIELD(OffscreenService,getBlockImageProc); FIELD(Server,hostObject);
#endif
static int checks=0;
static void check(bool b){++checks;if(!b){std::fprintf(stderr,"Host ABI check %d failed\n",checks);std::exit(1);}}
static AibHostInt maskValue=0,selectionValue=0,stateSeen=0;
static AibHostInt mask(AibHostBool* b, decltype(AibHostServer::hostObject)){*b=(AibHostBool)maskValue;return 0;}
static AibHostInt selection(AibHostBool* b, decltype(AibHostServer::hostObject)){*b=(AibHostBool)selectionValue;return 0;}
static AibHostInt process(AibHostInt* result, decltype(AibHostServer::hostObject),AibHostInt state){stateSeen=state;*result=state==kAibHostFilterRunProcessStateAbort?kAibHostFilterRunProcessResultExit:257;return 0;}
int main(){
    AibHostServer server{};
#ifdef AIB_OFFICIAL_SDK
    TriglavPlugInFilterRunRecord run{};
#else
    FilterRunRecord run{};
#endif
    server.recordSuite.filterRunRecord=&run;run.isLayerMaskSelectedProc=mask;run.hasSelectAreaOffscreenProc=selection;run.processProc=process;
    for(int m:{0,1})for(int s:{0,1}){maskValue=m;selectionValue=s;AibHostBool b=99;check(AibHostFilterRunIsLayerMaskSelected(&server.recordSuite,&b,server.hostObject)==0);check(b==m);check(AibHostFilterRunHasSelectAreaOffscreen(&server.recordSuite,&b,server.hostObject)==0);check(b==s);}
    for(auto state:{kAibHostFilterRunProcessStateStart,kAibHostFilterRunProcessStateContinue,kAibHostFilterRunProcessStateEnd,kAibHostFilterRunProcessStateAbort}){AibHostInt result=0;check(AibHostFilterRunProcess(&server.recordSuite,&result,server.hostObject,state)==0);check(stateSeen==state);check(result==(state==kAibHostFilterRunProcessStateAbort?kAibHostFilterRunProcessResultExit:257));}
    check(kAibHostNeedHostVersion==1);check(kAibHostModuleSwitchKindFilter==0x4380);check(kAibHostFilterTargetKindRasterLayerRGBAlpha==0x0102);
    std::printf("Host ABI: %d runtime checks passed; compile-time layout assertions passed.\n",checks);
}
