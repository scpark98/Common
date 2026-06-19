# Point this machine's Claude Code SessionStart C++ encoding hook at the
# git-synced Common copy (D:\1.Projects_C++\Common\hooks\ensure-cpp-encoding.ps1).
#
# settings.json lives in ~/.claude and is NOT git-synced, so each machine must run
# this once. Project paths are identical on both machines, so the same script works.
#
# Idempotent string patch (no JSON re-serialization, so the rest of settings.json —
# including the Korean "내 드라이브" paths — is preserved byte-for-byte).
# If no SessionStart hook block exists yet, it injects one.

$ErrorActionPreference = 'Stop'

$settings = Join-Path $env:USERPROFILE '.claude\settings.json'
# JSON-escaped (doubled backslash) forms, matching how paths appear inside settings.json.
$oldEsc = 'C:\\Users\\scpar\\.claude\\hooks\\ensure-cpp-encoding.ps1'
$newEsc = 'D:\\1.Projects_C++\\Common\\hooks\\ensure-cpp-encoding.ps1'

if (-not (Test-Path -LiteralPath $settings)) {
    Write-Host "[install-hook] settings.json not found: $settings"
    exit 1
}

$text = [IO.File]::ReadAllText($settings, [Text.Encoding]::UTF8)

if ($text.Contains($newEsc)) {
    Write-Host "[install-hook] already pointing at Common hook; nothing to do."
    exit 0
}

if ($text.Contains($oldEsc)) {
    $text = $text.Replace($oldEsc, $newEsc)
    [IO.File]::WriteAllText($settings, $text, (New-Object Text.UTF8Encoding($false)))
    Write-Host "[install-hook] settings.json updated -> Common hook."
    exit 0
}

Write-Host "[install-hook] no existing ensure-cpp-encoding hook path found in settings.json."
Write-Host "[install-hook] Add a SessionStart hook running:"
Write-Host "  powershell -NoProfile -ExecutionPolicy Bypass -File `"$($newEsc.Replace('\\','\'))`""
exit 1
