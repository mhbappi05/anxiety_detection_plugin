Add-Type @"
using System;
using System.Runtime.InteropServices;
public class DllTest3 {
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Auto)]
    public static extern bool SetDllDirectory(string lpPathName);
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Auto)]
    public static extern bool AddDllDirectory(string lpPathName);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hReservedNull, uint dwFlags);
    [DllImport("kernel32.dll", SetLastError=true)]
    public static extern uint GetLastError();
    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr hModule);
}
"@

$cbDir = "C:\Program Files\CodeBlocks"
$pluginDir = "C:\Users\User\AppData\Roaming\CodeBlocks\share\codeblocks\plugins"
$dll = Join-Path $pluginDir "anxiety_plugin.dll"

# LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR = 0x1000 | 0x100
$LOAD_WITH_ALTERED_SEARCH_PATH = 0x8

Write-Host "Adding search dirs..."
[DllTest3]::SetDllDirectory($pluginDir) | Out-Null

Write-Host "Testing: $dll"
$h = [DllTest3]::LoadLibraryEx($dll, [IntPtr]::Zero, $LOAD_WITH_ALTERED_SEARCH_PATH)
if ($h -eq [IntPtr]::Zero) {
    $err = [DllTest3]::GetLastError()
    Write-Host "FAILED. Error code: $err (0x$([Convert]::ToString($err, 16)))"
    # Error 193 = not a valid Win32 app (wrong arch)
    # Error 126 = module not found (missing dep)  
    # Error 998 = invalid access to memory location
}
else {
    Write-Host "SUCCESS! DLL loaded."
    [DllTest3]::FreeLibrary($h) | Out-Null
}

# Also test what Windows can actually find
Write-Host "`nChecking key DLL presence in known paths..."
@("C:\Windows\System32\ucrtbase.dll",
    "C:\Windows\System32\downlevel\api-ms-win-crt-runtime-l1-1-0.dll",
    "$pluginDir\wxbase32u_gcc_custom.dll",
    "$pluginDir\libjpeg-8.dll") | ForEach-Object {
    Write-Host "  $(Test-Path $_)  $_"
}
