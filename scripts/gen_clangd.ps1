$buildDir = (Resolve-Path -Path 'build' -ErrorAction Stop).Path -replace '\\', '/'
$cl = ''
$includes = @()

if ($env:VCToolsInstallDir) {
    $vcDir = $env:VCToolsInstallDir -replace '\\', '/'
    if (-not $vcDir.EndsWith('/')) { $vcDir += '/' }
    $cl = $vcDir + 'bin/Hostx64/x64/cl.exe'
    $includes += ('-I' + $vcDir + 'include')
}

# Find Windows SDK
$sdkDir = $env:WindowsSdkDir
$sdkVer = $env:WindowsSDKVersion

# Resolve SDK directory — derive from compiler path (VC/../Windows Kits)
if (-not $sdkDir -and $cl) {
    $dir = Split-Path $cl -Parent
    while ($dir) {
        if ((Split-Path $dir -Leaf) -eq 'VC') {
            $sdkCandidate = Join-Path (Split-Path $dir -Parent) 'Windows Kits\10'
            if (Test-Path $sdkCandidate) { $sdkDir = $sdkCandidate }
            break
        }
        $parent = Split-Path $dir -Parent
        if ($parent -eq $dir) { break }
        $dir = $parent
    }
}

# Resolve SDK version - prefer what's actually on disk
if ($sdkDir -and (Test-Path "$sdkDir\Include")) {
    $installedVersions = Get-ChildItem "$sdkDir\Include" -Directory | Sort-Object Name -Descending
    if ($sdkVer) {
        $sdkVerClean = $sdkVer.TrimEnd('\').TrimEnd('/')
        $matched = $installedVersions | Where-Object { $_.Name -eq $sdkVerClean }
        if ($matched) {
            $sdkVer = $matched.Name
        } else {
            $sdkVer = ($installedVersions | Select-Object -First 1).Name
        }
    } else {
        $sdkVer = ($installedVersions | Select-Object -First 1).Name
    }
}

if ($sdkDir -and $sdkVer) {
    $sdkDir = $sdkDir -replace '\\', '/'
    if (-not $sdkDir.EndsWith('/')) { $sdkDir += '/' }
    $includes += ('-I' + $sdkDir + 'Include/' + $sdkVer + '/ucrt')
    $includes += ('-I' + $sdkDir + 'Include/' + $sdkVer + '/shared')
    $includes += ('-I' + $sdkDir + 'Include/' + $sdkVer + '/um')
}

$content = "CompileFlags:`n"
$content += "  CompilationDatabase: $buildDir`n"
if ($cl) { $content += "  Compiler: $cl`n" }
$content += "  Add:`n"
foreach ($i in $includes) { $content += "    - $i`n" }
$content += "    - -Wno-c++98-compat`n"
$content += "    - -Wno-c++98-compat-pedantic`n"
$content += "    - -Wno-missing-prototypes`n"
$content += "    - -Wno-switch-default`n"
$content += "    - -D_HAS_CXX17=1`n"
$content += "    - -D_HAS_CXX20=1`n"
$content += "    - -D_HAS_CXX23=1`n"
$content += "    - /std:c++latest`n"

Set-Content -Path '.clangd' -Value $content -NoNewline
Write-Host 'Generated .clangd' -ForegroundColor Green
