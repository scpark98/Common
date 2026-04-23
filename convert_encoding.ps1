#===============================================================================
# convert_encoding.ps1
#
# 현재 폴더 이하의 모든 소스 파일을 UTF-8 with BOM으로 변환한다.
#
# 동작 규칙 (위에서부터 순서대로 판정):
#   1) 이미 UTF-8 BOM (EF BB BF)  -> 건너뜀
#   2) UTF-16 LE BOM  (FF FE)     -> UTF-16 LE로 디코드 후 UTF-8 BOM으로 재저장
#   3) UTF-16 BE BOM  (FE FF)     -> UTF-16 BE로 디코드 후 UTF-8 BOM으로 재저장
#   4) 순수 ASCII                  -> BOM만 앞에 붙여 재저장
#   5) 그 외                       -> CP949(EUC-KR)로 디코드 후 UTF-8 BOM으로 재저장
#
# 실행 방법:
#   미리 확인 : powershell -ExecutionPolicy Bypass -File convert_encoding.ps1 -DryRun
#   실제 변경 : powershell -ExecutionPolicy Bypass -File convert_encoding.ps1
#===============================================================================

param(
    [string]$Path = '.',
    [string[]]$Extensions = @('*.cpp','*.h','*.hpp','*.c','*.cc','*.cxx','*.rc','*.rc2','*.inl'),
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

$cp949   = [System.Text.Encoding]::GetEncoding(949)
$utf16le = [System.Text.Encoding]::Unicode
$utf16be = [System.Text.Encoding]::BigEndianUnicode
$utf8Bom = New-Object System.Text.UTF8Encoding($true)
$bomBytes = [byte[]](0xEF, 0xBB, 0xBF)

$stats = [ordered]@{
    'already_utf8_bom' = 0
    'utf16_le'         = 0
    'utf16_be'         = 0
    'ascii'            = 0
    'cp949'            = 0
    'skipped_error'    = 0
}

function Write-FileUtf8Bom {
    param([string]$FullPath, [string]$Text)
    if ($DryRun) { return }
    [System.IO.File]::WriteAllText($FullPath, $Text, $utf8Bom)
}

function Write-FileBytes {
    param([string]$FullPath, [byte[]]$Bytes)
    if ($DryRun) { return }
    [System.IO.File]::WriteAllBytes($FullPath, $Bytes)
}

Write-Host "Scan root : $((Resolve-Path $Path).Path)"
Write-Host "Extensions: $($Extensions -join ', ')"
if ($DryRun) { Write-Host "Mode      : DRY RUN (no files will be modified)" -ForegroundColor Yellow }
Write-Host ""

Get-ChildItem -Path $Path -Recurse -Include $Extensions -File | ForEach-Object {
    $file = $_.FullName
    $rel  = Resolve-Path -Relative $file

    try {
        $bytes = [System.IO.File]::ReadAllBytes($file)

        # 1) 이미 UTF-8 BOM
        if ($bytes.Length -ge 3 -and
            $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
            $stats['already_utf8_bom']++
            Write-Host "  [skip-utf8] $rel"
            return
        }

        # 2) UTF-16 LE BOM (FF FE) — .rc, resource.h 등 Visual Studio 리소스 편집기 기본
        if ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFF -and $bytes[1] -eq 0xFE) {
            $text = $utf16le.GetString($bytes, 2, $bytes.Length - 2)
            Write-FileUtf8Bom -FullPath $file -Text $text
            $stats['utf16_le']++
            Write-Host "  [utf16le ] $rel" -ForegroundColor Cyan
            return
        }

        # 3) UTF-16 BE BOM (FE FF)
        if ($bytes.Length -ge 2 -and $bytes[0] -eq 0xFE -and $bytes[1] -eq 0xFF) {
            $text = $utf16be.GetString($bytes, 2, $bytes.Length - 2)
            Write-FileUtf8Bom -FullPath $file -Text $text
            $stats['utf16_be']++
            Write-Host "  [utf16be ] $rel" -ForegroundColor Cyan
            return
        }

        # 4) 순수 ASCII — BOM만 붙인다
        $isAscii = $true
        foreach ($b in $bytes) { if ($b -gt 0x7F) { $isAscii = $false; break } }
        if ($isAscii) {
            Write-FileBytes -FullPath $file -Bytes ($bomBytes + $bytes)
            $stats['ascii']++
            Write-Host "  [ascii   ] $rel"
            return
        }

        # 5) CP949 (EUC-KR)로 디코드하여 UTF-8 BOM으로 재저장
        $text = $cp949.GetString($bytes)
        Write-FileUtf8Bom -FullPath $file -Text $text
        $stats['cp949']++
        Write-Host "  [cp949   ] $rel" -ForegroundColor Green
    }
    catch {
        $stats['skipped_error']++
        Write-Warning ("{0}: {1}" -f $rel, $_.Exception.Message)
    }
}

Write-Host ""
Write-Host "=== Summary ==="
foreach ($k in $stats.Keys) {
    "{0,-20} : {1}" -f $k, $stats[$k] | Write-Host
}
if ($DryRun) { Write-Host "`n(DryRun: 실제 파일은 수정되지 않았습니다)" -ForegroundColor Yellow }
