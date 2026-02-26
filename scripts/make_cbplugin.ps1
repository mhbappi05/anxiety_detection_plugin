$proj = "E:\UIU\12th Tri\FYDP II\anxiety_detection_intervention_plugin\anxiety_detection_plugin"
$tmp = Join-Path $proj "cbplugin_tmp"
$innerTmp = Join-Path $proj "cbplugin_inner_tmp"
$out = Join-Path $proj "anxiety_plugin.cbplugin"

Add-Type -AssemblyName System.IO.Compression.FileSystem

# ── Clean staging dirs ─────────────────────────────────────────────────────────
foreach ($d in @($tmp, $innerTmp)) {
    if (Test-Path $d) { Remove-Item $d -Recurse -Force }
    New-Item -ItemType Directory -Path $d -Force | Out-Null
}

# ── Step 1: Build the INNER ZIP (anxiety_plugin.zip) containing manifest.xml ──
#    Code::Blocks calls ReadManifestFile() which takes the DLL filename,
#    replaces .dll with .zip, and opens THAT file from inside the .cbplugin.
#    Inside that inner zip it looks for manifest.xml.
$manifestSrc = Join-Path $proj "resources\manifest.xml"
Copy-Item $manifestSrc (Join-Path $innerTmp "manifest.xml")

$innerZip = Join-Path $tmp "anxiety_plugin.zip"
[System.IO.Compression.ZipFile]::CreateFromDirectory($innerTmp, $innerZip)

# ── Step 2: Copy the DLL directly into the outer staging dir ──────────────────
$dllSrc = Join-Path $proj "anxiety_plugin.dll"
Copy-Item $dllSrc (Join-Path $tmp "anxiety_plugin.dll")

# ── Step 3: Create the outer .cbplugin (DLL + inner zip at root) ──────────────
if (Test-Path $out) { Remove-Item $out }
[System.IO.Compression.ZipFile]::CreateFromDirectory($tmp, $out)

# ── Cleanup ────────────────────────────────────────────────────────────────────
Remove-Item $tmp      -Recurse -Force
Remove-Item $innerTmp -Recurse -Force

Write-Host "`nDone! Created: $out"
Write-Host "`nOuter .cbplugin contents:"
$z = [System.IO.Compression.ZipFile]::OpenRead($out)
$z.Entries | Select-Object FullName, @{N = 'SizeKB'; E = { [math]::Round($_.Length / 1KB, 1) } }
$z.Dispose()
