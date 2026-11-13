# Unofficial host ABI provenance

Subset (ABI declarations through `#pragma pack(pop)`) of `FilterPlugIn.h` from potechitakusan/ComfyUIPlugin commit `37101fe5c023e3759e8e3158a706b85cda502cf2`, blob `03a8f7b80b4654634d029dea36c0c8f9126043e7`; upstream authors AonekoSS / consome hollywood, originating in AonekoSS/SDPlugin. Upstream Unlicense is preserved in LICENSE. C++ wrapper code after the ABI was omitted.

Local modifications: named the final slot of `dummy1[5]` as `isLayerMaskSelectedProc` and `dummy2[1]` as `hasSelectAreaOffscreenProc` without changing layout. Their signatures/positions were checked against the locally held official CELSYS SDK. This is **not** a clean-room claim or evidence of CELSYS permission. No SDK file text is included here. Local adapter constants and compatibility checks also reflect the locally verified host contract.

Upstream README reports switching away from the official SDK after CELSYS objected to OSS use of that SDK. That report does not establish permission for this project. Public source/package excludes the official SDK, official adapter and official binary; CELSYS submission is a separate private build.
