Add-Type -AssemblyName System.IO.Compression.FileSystem
$cbplugin = "E:\UIU\12th Tri\FYDP II\anxiety_detection_intervention_plugin\anxiety_detection_plugin\anxiety_plugin.cbplugin"
$z = [System.IO.Compression.ZipFile]::OpenRead($cbplugin)
Write-Host "Contents of anxiety_plugin.cbplugin:"
$z.Entries | Select-Object FullName, @{N = 'SizeKB'; E = { [math]::Round($_.Length / 1KB, 1) } }
$z.Dispose()
