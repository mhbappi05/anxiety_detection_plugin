$searchPaths = @(
    "C:\Program Files\CodeBlocks",
    "C:\Users\User\AppData\Roaming\CodeBlocks\share\codeblocks\plugins",
    "C:\msys64\ucrt64\bin",
    "C:\Windows\System32",
    "C:\Windows\SysWOW64"
)
function Find-Dll([string]$name) {
    foreach ($p in $searchPaths) {
        $f = Join-Path $p $name
        if (Test-Path $f) { return $f }
    }
    return $null
}
function Get-Deps([string]$path) {
    objdump -p $path 2>$null | Select-String 'DLL Name:' | ForEach-Object {
        ($_ -replace '.*DLL Name:\s*', '').Trim()
    }
}

$q = [Collections.Queue]::new()
$seen = @{}
$root = "C:\Users\User\AppData\Roaming\CodeBlocks\share\codeblocks\plugins\anxiety_plugin.dll"
$q.Enqueue($root); $seen[$root] = 1

Write-Host "=== Dep tree (with plugin dir in search path) ===`n"
while ($q.Count -gt 0) {
    $dll = $q.Dequeue()
    Get-Deps $dll | ForEach-Object {
        if ($seen.ContainsKey($_)) { return }
        $seen[$_] = 1
        $found = Find-Dll $_
        if ($found) {
            Write-Host "OK   $_"
            if ($found -notmatch "Windows") { $q.Enqueue($found) }
        }
        else {
            Write-Host "MISS $_ <-- MISSING"
        }
    }
}
