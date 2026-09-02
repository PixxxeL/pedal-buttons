$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$versionFile = Join-Path $scriptDir "version.h"
$outputFile = Join-Path $scriptDir "version.props"

$content = Get-Content $versionFile -Raw
if ($content -match 'PEDAL_BUTTONS_VERSION\s+"([^"]+)"') {
    $version = $Matches[1]
    $props = @"
<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <AppVersion>$version</AppVersion>
  </PropertyGroup>
</Project>
"@
    Set-Content -Path $outputFile -Value $props -Encoding UTF8
}
