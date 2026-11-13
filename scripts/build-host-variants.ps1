param([ValidateSet('unofficial','official','both')][string]$Variant='unofficial')
$ErrorActionPreference='Stop'
$repo=Split-Path $PSScriptRoot -Parent
$msbuild='C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe'
$variants=if($Variant -eq 'both'){@('unofficial','official')}else{@($Variant)}
foreach($item in $variants){
    if($item -eq 'official' -and !(Test-Path -LiteralPath (Join-Path $repo 'src\host_official.h'))){throw 'Private official adapter/SDK is not included in public source.'}
    foreach($project in @('CloudImage.vcxproj','HostAbiTests.vcxproj')){
        & $msbuild (Join-Path $repo $project) /p:Configuration=Release /p:Platform=x64 "/p:HostVariant=$item" /verbosity:minimal /nologo
        if($LASTEXITCODE -ne 0){throw "$project ($item) failed: $LASTEXITCODE"}
    }
    & (Join-Path $repo "build\$item\host_abi_tests.exe")
    if($LASTEXITCODE -ne 0){throw "ABI tests ($item) failed: $LASTEXITCODE"}
}
