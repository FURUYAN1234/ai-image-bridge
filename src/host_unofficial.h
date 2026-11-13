#pragma once
#include "../vendor/compat/FilterPlugIn.h"
using AibHostInt = FilterPlugIn::Int;
using AibHostBool = FilterPlugIn::Bool;
using AibHostPtr = FilterPlugIn::Ptr;
using AibHostUniChar = FilterPlugIn::UniChar;
using AibHostStringObject = FilterPlugIn::StringObject;
using AibHostRect = FilterPlugIn::Rect;
using AibHostPoint = FilterPlugIn::Point;
using AibHostOffscreenObject = FilterPlugIn::OffscreenObject;
using AibHostOffscreenService = FilterPlugIn::OffscreenService;
using AibHostStringService = FilterPlugIn::StringService;
using AibHostServer = FilterPlugIn::Server;
constexpr AibHostInt kAibHostAPIResultSuccess = 0;
constexpr AibHostInt kAibHostNeedHostVersion = 1;
constexpr AibHostInt kAibHostModuleSwitchKindFilter = 17280;
constexpr AibHostInt kAibHostFilterTargetKindRasterLayerRGBAlpha = 258;
constexpr AibHostInt kAibHostCallResultSuccess = 0;
constexpr AibHostInt kAibHostCallResultFailed = -1;
constexpr AibHostInt kAibHostSelectorModuleInitialize = 257;
constexpr AibHostInt kAibHostSelectorModuleTerminate = 258;
constexpr AibHostInt kAibHostSelectorFilterInitialize = 513;
constexpr AibHostInt kAibHostSelectorFilterRun = 514;
constexpr AibHostInt kAibHostFilterRunProcessStateStart = 257;
constexpr AibHostInt kAibHostFilterRunProcessStateContinue = 258;
constexpr AibHostInt kAibHostFilterRunProcessStateEnd = 259;
constexpr AibHostInt kAibHostFilterRunProcessStateAbort = 260;
constexpr AibHostInt kAibHostFilterRunProcessResultExit = 259;
#define AibHostFilterInitializeSetCanPreview(suite, ...) ((suite)->filterInitializeRecord->setCanPreviewProc(__VA_ARGS__))
#define AibHostFilterInitializeSetFilterCategoryName(suite, ...) ((suite)->filterInitializeRecord->setFilterCategoryNameProc(__VA_ARGS__))
#define AibHostFilterInitializeSetFilterName(suite, ...) ((suite)->filterInitializeRecord->setFilterNameProc(__VA_ARGS__))
#define AibHostFilterInitializeSetTargetKinds(suite, ...) ((suite)->filterInitializeRecord->setTargetKindsProc(__VA_ARGS__))
#define AibHostFilterInitializeSetUseBlankImage(suite, ...) ((suite)->filterInitializeRecord->setUseBlankImageProc(__VA_ARGS__))
#define AibHostFilterRunGetDestinationOffscreen(suite, ...) ((suite)->filterRunRecord->getDestinationOffscreenProc(__VA_ARGS__))
#define AibHostFilterRunGetSelectAreaOffscreen(suite, ...) ((suite)->filterRunRecord->getSelectAreaOffscreenProc(__VA_ARGS__))
#define AibHostFilterRunGetSelectAreaRect(suite, ...) ((suite)->filterRunRecord->getSelectAreaRectProc(__VA_ARGS__))
#define AibHostFilterRunGetSourceOffscreen(suite, ...) ((suite)->filterRunRecord->getSourceOffscreenProc(__VA_ARGS__))
#define AibHostFilterRunHasSelectAreaOffscreen(suite, ...) ((suite)->filterRunRecord->hasSelectAreaOffscreenProc(__VA_ARGS__))
#define AibHostFilterRunIsAlphaLocked(suite, ...) ((suite)->filterRunRecord->isAlphaLockedProc(__VA_ARGS__))
#define AibHostFilterRunIsLayerMaskSelected(suite, ...) ((suite)->filterRunRecord->isLayerMaskSelectedProc(__VA_ARGS__))
#define AibHostFilterRunProcess(suite, ...) ((suite)->filterRunRecord->processProc(__VA_ARGS__))
#define AibHostFilterRunSetProgressDone(suite, ...) ((suite)->filterRunRecord->setProgressDoneProc(__VA_ARGS__))
#define AibHostFilterRunSetProgressTotal(suite, ...) ((suite)->filterRunRecord->setProgressTotalProc(__VA_ARGS__))
#define AibHostFilterRunUpdateDestinationOffscreenRect(suite, ...) ((suite)->filterRunRecord->updateDestinationOffscreenRectProc(__VA_ARGS__))
#define AIB_HOST_CALLBACK __cdecl
#define AIB_HOST_VARIANT_W L"Unofficial SDK"
extern "C" __declspec(dllexport) void __cdecl TriglavPluginCall(AibHostInt*, AibHostPtr*, AibHostInt, AibHostServer*, AibHostPtr);
