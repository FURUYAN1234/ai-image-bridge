AI Image Bridge - Windows installation

1. Extract the ZIP into a local folder.
2. Close CLIP STUDIO PAINT.
3. Double-click Install.cmd. Administrator rights are not required.
4. Start CLIP STUDIO PAINT and select Filter > AI Image Bridge.

Install.ps1 checks the package hashes and installs the plug-in plus its three
PDF manuals into the fixed per-user CLIP STUDIO PAINT plug-in folder.
If PowerShell refuses an unsigned or downloaded script, no installation has
occurred. Follow your Windows or organization policy; this installer does not
change execution policy or bypass signature requirements.

Close CLIP STUDIO PAINT and double-click Uninstall.cmd to remove this version.
Diagnostics and other user files are retained. Uninstall one SDK variant before
installing the other. Both variants share the same plug-in identifier.

Previous plug-in files are backed up outside the host plug-in folder:
%LOCALAPPDATA%\AIImageBridge\backups
An existing CloudImage.cpm is moved there to prevent duplicate loading.
Uninstall does not automatically restore an older plug-in.
