# SessionStart hook: make VS C++ projects safe from Korean (CP949) corruption.
#
# Two protections, applied at session start BEFORE Claude reads/edits anything:
#   1. Drop a root .editorconfig (charset=utf-8-bom) if missing, so VS in Korean locale
#      stops re-encoding/BOM-dropping sources on save (Common/claude.md 2B.1 Step 0).
#   2. Convert every CP949 (or BOM-less UTF-8) source *and VS project file* that contains
#      non-ASCII bytes to UTF-8 BOM. Doing this up front means Claude's later Read sees
#      clean UTF-8 (correct old_string) and its Write can't re-encode CP949 into U+FFFD.
#
# Only touches files that actually need it:
#   - UTF-8 BOM  -> skip (already safe)
#   - UTF-16     -> skip (.rc/resource.h, VS-managed)
#   - pure ASCII -> add BOM (so Korean can be added later without MSVC reading it as CP949)
#   - RC-included headers (targetver.h) -> skip by name (rc.exe needs an RC-friendly
#     encoding; a Korean .h converted to UTF-8 BOM made rc.exe fail with RC 0x40)
#   - non-ASCII, no BOM: valid UTF-8 -> add BOM; else decode CP949 -> re-encode UTF-8 + BOM
# Build output dirs are excluded. Idempotent; silent unless it changes something.

$ErrorActionPreference = 'SilentlyContinue'

# Resolve project dir: prefer hook stdin JSON {cwd}, fall back to current location.
$proj = (Get-Location).Path
try {
    $raw = [Console]::In.ReadToEnd()
    if ($raw) {
        $j = $raw | ConvertFrom-Json
        if ($j.cwd) { $proj = $j.cwd }
    }
} catch {}

if (-not (Test-Path -LiteralPath $proj)) { exit 0 }

# Only act on Visual Studio C++ projects/solutions. cwd may be a *solution folder*
# whose .vcxproj files live in per-project subfolders (e.g. nFTDServer\, nFTDClient\),
# so a root .sln counts too, and .vcxproj is searched recursively (depth-limited) --
# not just directly in cwd. (Without this, solution-folder cwds were skipped entirely.)
$vcx = @(Get-ChildItem -LiteralPath $proj -Filter *.vcxproj -File -ErrorAction SilentlyContinue)
$sln = @(Get-ChildItem -LiteralPath $proj -Filter *.sln -File -ErrorAction SilentlyContinue)
if ($vcx.Count -eq 0 -and $sln.Count -eq 0) {
    $vcx = @(Get-ChildItem -LiteralPath $proj -Recurse -Depth 2 -Filter *.vcxproj -File -ErrorAction SilentlyContinue)
}
if ($vcx.Count -eq 0 -and $sln.Count -eq 0) { exit 0 }

$actions = @()

# --- 1. .editorconfig ---------------------------------------------------------
$ecPath = Join-Path $proj '.editorconfig'
if (-not (Test-Path -LiteralPath $ecPath)) {
    $ec = @'
root = true

# Auto-added by ensure-cpp-encoding hook. Forces UTF-8 BOM for C/C++ sources so VS in
# Korean locale (ANSI=CP949) does not silently re-encode Korean comments/strings.
# .rc/.rc2 stay UTF-16 LE per VS Resource Editor convention.

[*.{cpp,h,hpp,c,cc,cxx,inl,ipp}]
charset = utf-8-bom
end_of_line = crlf
indent_style = tab
tab_width = 4
insert_final_newline = true

[*.{vcxproj,filters,user,vcxitems,props,targets,sln}]
charset = utf-8-bom
end_of_line = crlf

[*.{rc,rc2}]
charset = utf-16le
end_of_line = crlf

[*.md]
charset = utf-8
end_of_line = crlf
'@
    [IO.File]::WriteAllText($ecPath, $ec, (New-Object Text.UTF8Encoding($false)))
    $actions += '.editorconfig'
}

# --- 2. Convert CP949 / BOM-less sources to UTF-8 BOM -------------------------
# C/C++ sources + VS project files ONLY. Allowlist (NOT denylist) so binaries (.png/.lib/.dll/.ico/.exe...)
# are never CP949-misdecoded and destroyed. Text docs (.md/.txt/.log/.csv/.ini/.xml/.html)
# are intentionally NOT here: a BOM breaks them -- notably claude.md, whose first line
# `@../Common/claude.md` (import directive) stops resolving with a leading BOM. .rc/.rc2
# stay UTF-16 (VS-managed); BOM-hostile text (.bat/.cmd/.sh/.json/.yaml) excluded too.
$exts = @('.cpp', '.h', '.hpp', '.c', '.cc', '.cxx', '.inl', '.ipp')

#20260723 by claude. VS project/solution files. Matched by full-name suffix rather than by
# extension, because the bare extensions ('.user', '.filters', '.props') are too loose to
# claim on their own -- only the real MSBuild set qualifies.
# These MUST be covered: VS itself writes them as UTF-8 BOM, and .vcxproj/.filters carry
# encoding="utf-8" in their XML declaration, so CP949 bytes are ALWAYS a defect -- VS
# decodes per the declaration, every Korean filter name becomes U+FFFD, and Solution
# Explorer shows "???? ????" with no way to recover the names by hand.
# Real case: SCColorTable.vcxproj.filters at commit d6f7336 (2026-07-13 18:01:20). BOTH
# merge parents were UTF-8 BOM; the merge result was CP949 with the BOM dropped, i.e. some
# tool decoded the text and rewrote it with the system ANSI codepage (the PowerShell
# Set-Content / Out-File default on ko-KR). SCColorTable.cpp was rewritten in that SAME
# commit and survived -- purely because .cpp was on this allowlist and .filters was not.
$projSuffixes = @('.vcxproj', '.vcxproj.filters', '.vcxproj.user',
                  '.vcxitems', '.vcxitems.filters', '.sln', '.props', '.targets')

$skipDirs = @('\x64\', '\win32\', '\debug\', '\release\', '\.vs\', '\ipch\', '\obj\')
# Headers that are #include'd by a .rc must keep an RC-friendly encoding. rc.exe chokes
# when such a Korean .h is converted to UTF-8 BOM (e.g. a stray doxygen '@' surfaced as
# "unknown character '0x40'" and aborted the RC preprocessor). Never convert these,
# regardless of their current encoding (compare by file name, case-insensitive).
$skipNames = @('targetver.h')

#20260914 by claude. .rc 가 include 하는 헤더는 프로젝트마다 이름이 달라 미리 알 수 없다.
#rc.exe 는 BOM 에 민감해 전처리가 끊기고, 그 여파가 엉뚱한 컴파일 오류로 나타난다.
#(2026-06-19 KoinoViewer targetver.h) 그래서 .rc 를 훑어 include 된 헤더를 제외 목록에 넣는다.
foreach ($rcFile in (Get-ChildItem -LiteralPath $proj -Recurse -File -Filter *.rc -ErrorAction SilentlyContinue)) {
    try {
        $rcBytes = [IO.File]::ReadAllBytes($rcFile.FullName)
        if ($rcBytes.Length -ge 2 -and $rcBytes[0] -eq 0xFF -and $rcBytes[1] -eq 0xFE) {
            $rcText = [Text.Encoding]::Unicode.GetString($rcBytes)
        } else {
            $rcText = [Text.Encoding]::UTF8.GetString($rcBytes)
        }
        foreach ($m in [regex]::Matches($rcText, '(?i)#includes+"([^"]+.h)"')) {
            $hn = [IO.Path]::GetFileName($m.Groups[1].Value).ToLower()
            if ($skipNames -notcontains $hn) { $skipNames += $hn }
        }
    } catch {}
}
$bom = [byte[]](0xEF, 0xBB, 0xBF)
$utf8Strict = New-Object Text.UTF8Encoding($false, $true)   # throwOnInvalidBytes
$cp949 = [Text.Encoding]::GetEncoding(949)
$converted = 0

$files = Get-ChildItem -LiteralPath $proj -Recurse -File -ErrorAction SilentlyContinue |
    Where-Object {
        if ($exts -contains $_.Extension.ToLower()) { return $true }
        $n = $_.Name.ToLower()
        foreach ($s in $projSuffixes) { if ($n.EndsWith($s)) { return $true } }
        return $false
    }

foreach ($f in $files) {
    $low = $f.FullName.ToLower()
    $skip = $false
    foreach ($d in $skipDirs) { if ($low.Contains($d)) { $skip = $true; break } }
    if ($skip) { continue }
    if ($skipNames -contains $f.Name.ToLower()) { continue }

    $bytes = [IO.File]::ReadAllBytes($f.FullName)
    if ($bytes.Length -lt 1) { continue }

    # UTF-8 BOM -> already safe
    if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) { continue }
    # UTF-16 (BOM) -> leave to VS
    if ($bytes.Length -ge 2 -and (($bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) -or ($bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF))) { continue }

    #20260914 by claude. 순수 ASCII 파일도 BOM 을 붙인다.
    #예전에는 "깨질 한글이 없으니 그대로 둔다" 로 건너뛰었다. 그러면 그 파일에 한글 주석을
    #처음 넣는 순간 MSVC 가 CP949 로 읽어 깨지고, 넣는 사람은 그 사실을 모른다.
    #실제로 LMMHost 의 CursorShapeDetector.cpp 가 그 상태였다(2026-09-14).
    #훅의 목표를 "손상 방지" 에서 "모든 소스를 UTF-8 BOM 으로 통일" 로 넓힌다.
    $hasHigh = $false
    foreach ($b in $bytes) { if ($b -ge 0x80) { $hasHigh = $true; break } }
    if (-not $hasHigh) {
        $out = New-Object byte[] ($bom.Length + $bytes.Length)
        [Array]::Copy($bom, 0, $out, 0, $bom.Length)
        [Array]::Copy($bytes, 0, $out, $bom.Length, $bytes.Length)
        [IO.File]::WriteAllBytes($f.FullName, $out)
        $converted++
        continue
    }

    # non-ASCII, no BOM: decide UTF-8-missing-BOM vs CP949
    $isUtf8 = $true
    try { [void]$utf8Strict.GetString($bytes) } catch { $isUtf8 = $false }

    if ($isUtf8) {
        # already UTF-8, just prepend BOM
        $out = New-Object byte[] ($bom.Length + $bytes.Length)
        [Array]::Copy($bom, 0, $out, 0, $bom.Length)
        [Array]::Copy($bytes, 0, $out, $bom.Length, $bytes.Length)
    } else {
        # CP949 -> UTF-8 (CR/LF are ASCII, preserved), prepend BOM
        $text = $cp949.GetString($bytes)
        $u = ([Text.UTF8Encoding]::new($false)).GetBytes($text)
        $out = New-Object byte[] ($bom.Length + $u.Length)
        [Array]::Copy($bom, 0, $out, 0, $bom.Length)
        [Array]::Copy($u, 0, $out, $bom.Length, $u.Length)
    }
    [IO.File]::WriteAllBytes($f.FullName, $out)
    $converted++
}

if ($converted -gt 0) { $actions += "converted $converted file(s) to UTF-8 BOM" }

if ($actions.Count -gt 0) {
    $msg = "ensure-cpp-encoding [$proj]: " + ($actions -join '; ')
    (@{ systemMessage = $msg } | ConvertTo-Json -Compress)
}
exit 0
