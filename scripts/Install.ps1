[CmdletBinding()]
param([switch]$CheckOnly,[string]$DestinationRoot,[switch]$TestMode)
$ErrorActionPreference='Stop'
$script:AibKnownFiles=@('AIImageBridge.cpm','manifest.json','manuals/AI-Image-Bridge-Manual.pdf','manuals/OpenAI-API-Setup.pdf','manuals/Gemini-API-Setup.pdf','LICENSE','THIRD_PARTY_NOTICES.md','licenses/SDPlugin-Unlicense.txt','licenses/nlohmann-json-MIT.txt')
function Assert-AibStopped {
    if(@(Get-Process -ErrorAction SilentlyContinue | Where-Object {$_.ProcessName -ieq 'CLIPStudioPaint'}).Count){throw 'Close CLIP STUDIO PAINT before installing or uninstalling.'}
}
function Get-AibPaths([string]$DestinationRoot,[bool]$TestMode){
    $fixed=Join-Path $env:APPDATA 'CELSYSUserData\CELSYS\CLIPStudioModule\PlugIn\PAINT'
    if(!$DestinationRoot){$DestinationRoot=$fixed}
    $root=[IO.Path]::GetFullPath($DestinationRoot).TrimEnd('\','/')
    if($TestMode){
        $allowed=[IO.Path]::GetFullPath((Join-Path (Split-Path $PSScriptRoot -Parent) 'work\installer-tests')).TrimEnd('\','/')+'\'
        if(!$root.StartsWith($allowed,[StringComparison]::OrdinalIgnoreCase)){throw 'Test destination must be inside work/installer-tests.'}
        $backup=Join-Path (Split-Path $root -Parent) 'backups'
    }else{
        if($root -ine [IO.Path]::GetFullPath($fixed).TrimEnd('\','/')){throw 'Destination is fixed to the per-user CLIP STUDIO PAINT plug-in directory.'}
        $backup=Join-Path $env:LOCALAPPDATA 'AIImageBridge\backups'
    }
    # Validate every existing ancestor and managed child before any mutation.
    $scan=$root
    while($scan){if(Test-Path -LiteralPath $scan){if((Get-Item -LiteralPath $scan -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse point is not supported: $scan"}};$parent=Split-Path $scan -Parent;if($parent -eq $scan){break};$scan=$parent}
    foreach($child in @('AIImageBridge','AIImageBridge\manuals','CloudImage')){$dir=Join-Path $root $child;if((Test-Path -LiteralPath $dir) -and ((Get-Item -LiteralPath $dir -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Reparse point is not supported: $dir"}}
    [pscustomobject]@{Root=$root;Target=(Join-Path $root 'AIImageBridge');Legacy=(Join-Path $root 'CloudImage\CloudImage.cpm');Backup=$backup}
}
function Assert-AibPackage([string]$PackageRoot){
    $manifestPath=Join-Path $PackageRoot 'manifest.json'
    if(!(Test-Path -LiteralPath $manifestPath -PathType Leaf)){throw 'manifest.json is missing.'}
    $manifest=Get-Content -LiteralPath $manifestPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if($manifest.name -cne 'AI Image Bridge' -or $manifest.variant -cnotin @('official','unofficial') -or !$manifest.version){throw 'Invalid package identity or build variant.'}
    foreach($relative in $script:AibKnownFiles){
        $path=Join-Path $PackageRoot $relative
        if(!(Test-Path -LiteralPath $path -PathType Leaf)){throw "Required package file missing: $relative"}
        if($relative -eq 'manifest.json'){continue}
        $property=if($manifest.files){$manifest.files.PSObject.Properties[$relative]}else{$null}
        $expected=if($property){[string]$property.Value}elseif($relative -eq 'AIImageBridge.cpm'){[string]$manifest.pluginSha256}else{''}
        if($expected -notmatch '^[0-9a-fA-F]{64}$'){throw "SHA256 is missing: $relative"}
        if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine $expected){throw "Package checksum mismatch: $relative"}
        if($relative -eq 'AIImageBridge.cpm' -and $expected -ine $manifest.pluginSha256){throw 'Binary hash declarations disagree.'}
    }
    return $manifest
}
function Install-Aib([string]$PackageRoot,[string]$DestinationRoot,[bool]$TestMode,[bool]$CheckOnly){
    Assert-AibStopped
    $paths=Get-AibPaths $DestinationRoot $TestMode
    $manifest=Assert-AibPackage $PackageRoot
    $installedManifest=Join-Path $paths.Target 'manifest.json'
    if(Test-Path -LiteralPath $installedManifest){$installed=Get-Content -LiteralPath $installedManifest -Raw -Encoding UTF8 | ConvertFrom-Json;if($installed.variant -and $installed.variant -cne $manifest.variant){throw 'A different SDK variant is installed. Uninstall it before switching variants.'}}
    foreach($relative in $script:AibKnownFiles){$target=Join-Path $paths.Target $relative;if(Test-Path -LiteralPath $target){if((Get-Item -LiteralPath $target -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse point is not supported: $target"}}}
    if($CheckOnly){Write-Output "Validated $($manifest.variant) $($manifest.version): $($paths.Target)";return}
    $backup=Join-Path $paths.Backup ((Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-'+[guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $backup -Force | Out-Null
    $previous=@();$written=@();$legacyMoved=$false
    try {
        foreach($relative in $script:AibKnownFiles){$target=Join-Path $paths.Target $relative;if(Test-Path -LiteralPath $target -PathType Leaf){$saved=Join-Path $backup ('previous\'+$relative);New-Item -ItemType Directory -Path (Split-Path $saved -Parent) -Force | Out-Null;Copy-Item -LiteralPath $target -Destination $saved;$previous+=$relative}}
        if(Test-Path -LiteralPath $paths.Legacy -PathType Leaf){Move-Item -LiteralPath $paths.Legacy -Destination (Join-Path $backup 'CloudImage.cpm');$legacyMoved=$true}
        foreach($relative in $script:AibKnownFiles){$target=Join-Path $paths.Target $relative;New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null;$written+=$relative;Copy-Item -LiteralPath (Join-Path $PackageRoot $relative) -Destination $target -Force}
        Assert-AibPackage $paths.Target | Out-Null
    }catch{
        $cause=$_
        foreach($relative in $written){$target=Join-Path $paths.Target $relative;if($previous -contains $relative){Copy-Item -LiteralPath (Join-Path $backup ('previous\'+$relative)) -Destination $target -Force}elseif(Test-Path -LiteralPath $target -PathType Leaf){Remove-Item -LiteralPath $target -Force}}
        if($legacyMoved){Move-Item -LiteralPath (Join-Path $backup 'CloudImage.cpm') -Destination $paths.Legacy}
        throw $cause
    }
    Write-Output "Installed $($manifest.variant) $($manifest.version). Start CLIP STUDIO PAINT. Backup: $backup"
}
function Uninstall-Aib([string]$DestinationRoot,[bool]$TestMode,[bool]$CheckOnly){
    Assert-AibStopped
    $paths=Get-AibPaths $DestinationRoot $TestMode
    foreach($relative in $script:AibKnownFiles){$target=Join-Path $paths.Target $relative;if(Test-Path -LiteralPath $target){if((Get-Item -LiteralPath $target -Force).Attributes -band [IO.FileAttributes]::ReparsePoint){throw "Reparse point is not supported: $target"}}}
    if($CheckOnly){Write-Output "Uninstall validated: $($paths.Target)";return}
    $backup=Join-Path $paths.Backup ((Get-Date -Format 'yyyyMMdd-HHmmss-fff')+'-uninstall-'+[guid]::NewGuid().ToString('N'))
    foreach($relative in $script:AibKnownFiles){$target=Join-Path $paths.Target $relative;if(Test-Path -LiteralPath $target -PathType Leaf){$saved=Join-Path $backup $relative;New-Item -ItemType Directory -Path (Split-Path $saved -Parent) -Force | Out-Null;Copy-Item -LiteralPath $target -Destination $saved}}
    foreach($relative in $script:AibKnownFiles){$target=Join-Path $paths.Target $relative;if(Test-Path -LiteralPath $target -PathType Leaf){Remove-Item -LiteralPath $target -Force}}
    Write-Output "Uninstalled. User data and backups were retained: $($paths.Target)"
}
if($MyInvocation.InvocationName -ne '.'){Install-Aib $PSScriptRoot $DestinationRoot ([bool]$TestMode) ([bool]$CheckOnly)}
