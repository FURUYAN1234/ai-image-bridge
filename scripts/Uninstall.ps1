[CmdletBinding()]
param([switch]$CheckOnly,[string]$DestinationRoot,[switch]$TestMode)
$ErrorActionPreference='Stop'
$uninstallArguments=@{DestinationRoot=$DestinationRoot;TestMode=[bool]$TestMode;CheckOnly=[bool]$CheckOnly}
. (Join-Path $PSScriptRoot 'Install.ps1')
Uninstall-Aib @uninstallArguments
