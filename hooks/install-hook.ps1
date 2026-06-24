# Install BOTH git-synced C++ Korean-encoding hooks into this machine's Claude Code
# settings.json. Idempotent: run once per machine (settings.json is NOT git-synced, but
# the hook scripts and project paths are identical on both machines).
#
#   1. SessionStart -> ensure-cpp-encoding.ps1       (bulk-convert existing sources at start)
#   2. PreToolUse (Read|Edit|Write|MultiEdit) -> protect-cpp-encoding-file.ps1
#                                                    (convert a file added mid-session, on touch)
#   3. PostToolUse (Write|Edit|MultiEdit) -> ensure-cpp-encoding-after-write.ps1
#                                                    (re-add the BOM+CRLF that `Write` drops, after write)
#
# Why string injection (not ConvertTo-Json round-trip): Windows PowerShell 5.1 collapses a
# single-element array into a bare object, which would turn the required hook-event ARRAYS
# ("SessionStart": [ {...} ]) into objects and break Claude Code's schema. String injection
# also keeps the rest of settings.json -- including the Korean "내 드라이브" paths -- byte-for-byte.

$ErrorActionPreference = 'Stop'

$settings = Join-Path $env:USERPROFILE '.claude\settings.json'
$dir      = 'D:\1.Projects_C++\Common\hooks'
$ensure   = Join-Path $dir 'ensure-cpp-encoding.ps1'
$protect  = Join-Path $dir 'protect-cpp-encoding-file.ps1'
$after    = Join-Path $dir 'ensure-cpp-encoding-after-write.ps1'

foreach ($s in @($ensure, $protect, $after)) {
    if (-not (Test-Path -LiteralPath $s)) { throw "[install-hook] missing hook script: $s" }
}
if (-not (Test-Path -LiteralPath $settings)) {
    New-Item -ItemType Directory -Force -Path (Split-Path $settings) | Out-Null
    [IO.File]::WriteAllText($settings, "{`r`n}`r`n", (New-Object Text.UTF8Encoding($false)))
}

# JSON-escape a hook command (double backslashes first, then escape quotes so the \" added
# by the quote pass is not itself doubled).
function Esc($cmd) { $cmd.Replace('\', '\\').Replace('"', '\"') }
$ensureCmd  = Esc('powershell -NoProfile -ExecutionPolicy Bypass -File "' + $ensure + '"')
$protectCmd = Esc('powershell -NoProfile -ExecutionPolicy Bypass -File "' + $protect + '"')
$afterCmd   = Esc('powershell -NoProfile -ExecutionPolicy Bypass -File "' + $after + '"')

# Build a hook-event array literal. $matcher = $null for events that take none (SessionStart).
# $pad = base indentation for the event key (e.g. 4 spaces, matching siblings under "hooks").
function Block([string]$event, [string]$matcher, [string]$cmd, [int]$timeout, [string]$pad) {
    $m = if ($matcher) { "$pad    `"matcher`": `"$matcher`",`r`n" } else { '' }
    @"
$pad`"$event`": [
$pad  {
$m$pad    `"hooks`": [
$pad      {
$pad        `"type`": `"command`",
$pad        `"command`": `"$cmd`",
$pad        `"timeout`": $timeout
$pad      }
$pad    ]
$pad  }
$pad]
"@
}

$text = [IO.File]::ReadAllText($settings, [Text.Encoding]::UTF8)
$changed = $false

# --- 1. SessionStart: fix an old path in place, else mark for insertion -------
$oldEnsure = 'C:\\Users\\scpar\\.claude\\hooks\\ensure-cpp-encoding.ps1'
$newEnsure = 'D:\\1.Projects_C++\\Common\\hooks\\ensure-cpp-encoding.ps1'
if (-not $text.Contains($newEnsure) -and $text.Contains($oldEnsure)) {
    $text = $text.Replace($oldEnsure, $newEnsure); $changed = $true
}
# Match exact filenames: 'ensure-cpp-encoding.ps1' is NOT a substring of
# 'ensure-cpp-encoding-after-write.ps1' (the '-after-write' breaks the '.ps1' boundary),
# so the three needles stay independent.
$needSession = -not $text.Contains('ensure-cpp-encoding.ps1')
$needPre     = -not $text.Contains('protect-cpp-encoding-file')
$needPost    = -not $text.Contains('ensure-cpp-encoding-after-write')

if ($needSession -or $needPre -or $needPost) {
    $newBlocks = @()
    if ($needSession) { $newBlocks += (Block 'SessionStart' $null $ensureCmd 15 '    ') }
    if ($needPre)     { $newBlocks += (Block 'PreToolUse' 'Read|Edit|Write|MultiEdit' $protectCmd 10 '    ') }
    if ($needPost)    { $newBlocks += (Block 'PostToolUse' 'Write|Edit|MultiEdit' $afterCmd 10 '    ') }

    $m = [regex]::Match($text, '"hooks"\s*:\s*\{')
    if ($m.Success) {
        # Insert inside the existing "hooks" object, right after its opening brace.
        $insertAt = $m.Index + $m.Length
        $rest = $text.Substring($insertAt)
        $empty = $rest -match '^\s*\}'      # "hooks": {}  -> no sibling to comma onto
        $joined = ($newBlocks -join ",`r`n")
        $sep = if ($empty) { "`r`n" } else { ",`r`n" }   # trailing comma only if props follow
        $text = $text.Substring(0, $insertAt) + "`r`n" + $joined + $sep + $rest
    } else {
        # No "hooks" key: append a whole "hooks" object before the root's closing brace,
        # adding a comma after the last existing property.
        $hooksObj = "  `"hooks`": {`r`n" + ($newBlocks -join ",`r`n") + "`r`n  }"
        $text = [regex]::Replace($text, '(\S)(\s*)\}\s*$', ('$1,' + "`r`n" + $hooksObj + "`r`n}`r`n"), 1)
    }
    $changed = $true
}

if (-not $changed) {
    Write-Host "[install-hook] all hooks already installed; nothing to do."
    exit 0
}

# sanity: must be valid JSON before we overwrite
try { [void]($text | ConvertFrom-Json) }
catch { throw "[install-hook] aborted: produced invalid JSON. settings.json left untouched.`r`n$_" }

[IO.File]::WriteAllText($settings, $text, (New-Object Text.UTF8Encoding($false)))
Write-Host "[install-hook] settings.json updated: SessionStart + PreToolUse + PostToolUse encoding hooks installed."
exit 0
