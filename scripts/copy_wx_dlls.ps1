$src = "C:\msys64\ucrt64\bin"
$dst = "C:\Users\User\AppData\Roaming\CodeBlocks\share\codeblocks\plugins"

$needed = @(
    "wxbase32u_gcc_custom.dll",
    "wxbase32u_xml_gcc_custom.dll",
    "wxmsw32u_core_gcc_custom.dll",
    "libgcc_s_seh-1.dll",
    "libstdc++-6.dll",
    "libwinpthread-1.dll",
    "liblzma-5.dll",
    "libpcre2-16-0.dll",
    "zlib1.dll",
    "libexpat-1.dll",
    # Image libs (transitive deps of wxmsw32u_core_gcc_custom.dll)
    "libjpeg-8.dll",
    "libpng16-16.dll",
    "libtiff-6.dll"
)

New-Item -ItemType Directory -Path $dst -Force | Out-Null
$copied = 0; $skipped = 0

foreach ($f in $needed) {
    $s = Join-Path $src $f
    $d = Join-Path $dst $f
    if (-not (Test-Path $s)) { Write-Host "NOT IN UCRT64: $f"; continue }
    if (Test-Path $d) { Write-Host "EXISTS:  $f"; $skipped++; continue }
    Copy-Item $s $d -Force
    Write-Host "COPIED:  $f"
    $copied++
}

Write-Host "`nDone. Copied: $copied, Already present: $skipped"
Write-Host "Plugin dir DLL count: $((Get-ChildItem $dst -Filter '*.dll').Count)"
