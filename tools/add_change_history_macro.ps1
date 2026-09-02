<#
.SYNOPSIS
    Confluence 스페이스의 모든 문서 맨 앞에 Change History("/버전") 매크로를 추가한다.

.DESCRIPTION
    Confluence REST API v2 로 각 문서의 ADF 본문을 받아 최상단 노드만 하나 끼워넣고 되돌려쓴다.
    본문의 나머지 바이트는 손대지 않으므로 표·매크로·인라인 코멘트가 그대로 보존된다.

    이미 맨 앞에 Change History 매크로가 있는 문서는 건너뛴다(멱등).

    API 경로는 두 가지가 있으며 시작할 때 자동으로 살아있는 쪽을 고른다.
      (1) 사이트 직접  : https://<site>/wiki/api/v2/...
      (2) 게이트웨이   : https://api.atlassian.com/ex/confluence/<cloudId>/wiki/api/v2/...
    최근 발급되는 스코프형 API 토큰은 (2) 만 동작하는 경우가 있다.

.PARAMETER Diagnose
    어느 경로·엔드포인트가 되는지 상태코드와 응답을 전부 출력하고 종료한다.

.PARAMETER ListSpaces
    접근 가능한 스페이스의 id/key/name 을 출력하고 종료한다. SpaceId 확인용.

.PARAMETER Apply
    실제로 저장한다. 지정하지 않으면 무엇을 바꿀지 목록만 출력한다(기본값 = 미리보기).

.EXAMPLE
    .\add_change_history_macro.ps1 -Email you@example.com -ApiToken xxxx -Diagnose
    .\add_change_history_macro.ps1 -Email you@example.com -ApiToken xxxx -ListSpaces
    .\add_change_history_macro.ps1 -Email you@example.com -ApiToken xxxx
    .\add_change_history_macro.ps1 -Email you@example.com -ApiToken xxxx -Apply
#>
param(
    [Parameter(Mandatory = $true)][string] $Email,
    [Parameter(Mandatory = $true)][string] $ApiToken,
    [string] $Site    = 'koinodoc.atlassian.net',
    [string] $SpaceId = '557063',
    [switch] $Apply,
    [switch] $ListSpaces,
    [switch] $Diagnose
)

$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$auth    = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes("${Email}:${ApiToken}"))
$headers = @{ Authorization = "Basic $auth"; Accept = 'application/json' }
$root    = "https://$Site"

function Get-ErrorBody
{
    param($ErrorRecord)

    $response = $ErrorRecord.Exception.Response
    if (-not $response)
    {
        return ''
    }

    try
    {
        $reader = New-Object IO.StreamReader($response.GetResponseStream(), [Text.Encoding]::UTF8)
        $text   = $reader.ReadToEnd()
        $reader.Close()
        return $text
    }
    catch
    {
        return ''
    }
}

#상태코드만 조용히 확인한다. 성공하면 본문 문자열, 실패하면 $null.
function Test-Endpoint
{
    param([string] $Url, [switch] $NoAuth, [switch] $Verbose)

    $h = if ($NoAuth) { @{ Accept = 'application/json' } } else { $headers }

    try
    {
        $r = Invoke-WebRequest -Uri $Url -Headers $h -Method Get -UseBasicParsing
        if ($Verbose)
        {
            Write-Host ("  OK   {0}  {1}" -f $r.StatusCode, $Url) -ForegroundColor Green
        }
        return [Text.Encoding]::UTF8.GetString($r.RawContentStream.ToArray())
    }
    catch
    {
        if ($Verbose)
        {
            $status = ''
            if ($_.Exception.Response) { $status = [int]$_.Exception.Response.StatusCode }
            Write-Host ("  FAIL {0}  {1}" -f $status, $Url) -ForegroundColor Red
            $body = Get-ErrorBody $_
            if ($body) { Write-Host ("       {0}" -f $body) -ForegroundColor DarkYellow }
        }
        return $null
    }
}

function Get-CloudId
{
    $json = Test-Endpoint -Url "$root/_edge/tenant_info" -NoAuth
    if (-not $json) { return $null }

    try   { return ($json | ConvertFrom-Json).cloudId }
    catch { return $null }
}

#사이트 직접 경로와 게이트웨이 경로 중 실제로 동작하는 쪽을 고른다.
function Resolve-ApiPrefix
{
    param([switch] $Verbose)

    if ($Verbose) { Write-Host '[1] 사이트 직접 경로' }
    if (Test-Endpoint -Url "$root/wiki/api/v2/pages?limit=1" -Verbose:$Verbose)
    {
        return $root
    }

    if ($Verbose) { Write-Host '[2] 게이트웨이 경로 (api.atlassian.com)' }
    $cloudId = Get-CloudId
    if (-not $cloudId)
    {
        if ($Verbose) { Write-Host '  cloudId 를 얻지 못했습니다.' -ForegroundColor Red }
        return $null
    }

    if ($Verbose) { Write-Host ("  cloudId = {0}" -f $cloudId) }
    $gw = "https://api.atlassian.com/ex/confluence/$cloudId"
    if (Test-Endpoint -Url "$gw/wiki/api/v2/pages?limit=1" -Verbose:$Verbose)
    {
        return $gw
    }

    return $null
}

if ($Diagnose)
{
    #이 토큰이 "어느 계정" 으로 인증되는지 확인한다. Confluence 권한과 무관하게 응답하므로
    #403(권한 없음)과 계정 불일치를 구분하는 데 결정적이다.
    Write-Host '=== 토큰 소유 계정 (api.atlassian.com/me) ==='
    $me = Test-Endpoint -Url 'https://api.atlassian.com/me' -Verbose
    if ($me)
    {
        try
        {
            $meObj = $me | ConvertFrom-Json
            Write-Host ("  account_id : {0}" -f $meObj.account_id) -ForegroundColor Cyan
            Write-Host ("  email      : {0}" -f $meObj.email) -ForegroundColor Cyan
            Write-Host ("  name       : {0}" -f $meObj.name) -ForegroundColor Cyan
        }
        catch
        {
            Write-Host ("  {0}" -f $me)
        }
    }

    Write-Host ''
    Write-Host '=== 인증 확인 ==='
    [void](Test-Endpoint -Url "$root/wiki/rest/api/user/current" -Verbose)

    Write-Host ''
    Write-Host '=== v1 API ==='
    [void](Test-Endpoint -Url "$root/wiki/rest/api/space?limit=1" -Verbose)

    Write-Host ''
    Write-Host '=== v2 API 경로 탐색 ==='
    $prefix = Resolve-ApiPrefix -Verbose

    Write-Host ''
    if ($prefix)
    {
        Write-Host ("사용 가능한 API prefix : {0}" -f $prefix) -ForegroundColor Green
    }
    else
    {
        Write-Host '동작하는 v2 경로를 찾지 못했습니다.' -ForegroundColor Red
    }
    return
}

$apiPrefix = Resolve-ApiPrefix
if (-not $apiPrefix)
{
    Write-Host 'Confluence v2 API 에 접근할 수 없습니다. -Diagnose 로 다시 실행해 원인을 확인하십시오.' -ForegroundColor Red
    return
}

function Invoke-Api
{
    param([string] $Method, [string] $Url, [string] $JsonBody)

    try
    {
        #Invoke-RestMethod 를 쓰지 않는다. PS 5.1 은 응답 헤더에 charset 이 없으면 JSON 을
        #ISO-8859-1 로 디코딩하는데 Atlassian 은 charset 을 붙이지 않는다. 그대로 두면 한글이
        #깨진 채로 읽히고, 그 값을 되쓰는 순간 원본 문서가 깨진 문자로 덮어써진다.
        #응답을 바이트로 받아 UTF-8 로 직접 디코딩한다.
        if ($JsonBody)
        {
            #보낼 때도 마찬가지. 문자열을 주면 Latin-1 로 나가므로 바이트로 넘긴다.
            $bytes    = [Text.Encoding]::UTF8.GetBytes($JsonBody)
            $response = Invoke-WebRequest -Uri $Url -Headers $headers -Method $Method `
                -ContentType 'application/json; charset=utf-8' -Body $bytes -UseBasicParsing
        }
        else
        {
            $response = Invoke-WebRequest -Uri $Url -Headers $headers -Method $Method -UseBasicParsing
        }

        $text = [Text.Encoding]::UTF8.GetString($response.RawContentStream.ToArray())
        if ([string]::IsNullOrWhiteSpace($text))
        {
            return $null
        }

        return ($text | ConvertFrom-Json)
    }
    catch
    {
        $status = ''
        if ($_.Exception.Response) { $status = [int]$_.Exception.Response.StatusCode }

        Write-Host ''
        Write-Host ("요청 실패  [{0}]  {1} {2}" -f $status, $Method, $Url) -ForegroundColor Red
        $body = Get-ErrorBody $_
        if ($body) { Write-Host ("서버 응답: {0}" -f $body) -ForegroundColor DarkYellow }
        throw
    }
}

if ($ListSpaces)
{
    $url = "$apiPrefix/wiki/api/v2/spaces?limit=250"
    while ($url)
    {
        $resp = Invoke-Api -Method Get -Url $url
        foreach ($s in $resp.results)
        {
            Write-Host ("{0,-12} {1,-20} {2}" -f $s.id, $s.key, $s.name)
        }

        if ($resp._links -and $resp._links.next) { $url = $apiPrefix + $resp._links.next } else { $url = $null }
    }
    return
}

#스페이스의 모든 현재 문서 수집 (페이지네이션)
$pages = New-Object System.Collections.ArrayList
$url   = "$apiPrefix/wiki/api/v2/pages?space-id=$SpaceId&status=current&limit=250"

while ($url)
{
    $resp = Invoke-Api -Method Get -Url $url
    foreach ($p in $resp.results)
    {
        [void]$pages.Add([pscustomobject]@{ Id = $p.id; Title = $p.title })
    }

    #next 는 "/wiki/api/v2/..." 형태의 절대 경로다. prefix 에만 이어붙인다.
    if ($resp._links -and $resp._links.next) { $url = $apiPrefix + $resp._links.next } else { $url = $null }
}

Write-Host ("문서 {0}개 확인." -f $pages.Count)
if (-not $Apply)
{
    Write-Host "[미리보기] 실제로 저장하려면 -Apply 를 붙여 다시 실행하십시오." -ForegroundColor Yellow
}

$added  = 0
$kept   = 0
$failed = 0

foreach ($page in $pages)
{
    try
    {
        $detail = Invoke-Api -Method Get -Url "$apiPrefix/wiki/api/v2/pages/$($page.Id)?body-format=atlas_doc_format"
        $adf    = $detail.body.atlas_doc_format.value | ConvertFrom-Json

        $first = $null
        if ($adf.content -and @($adf.content).Count -gt 0)
        {
            $first = @($adf.content)[0]
        }

        $hasMacro = ($first -and
                     $first.type -eq 'extension' -and
                     $first.attrs -and
                     $first.attrs.extensionKey -eq 'change-history')

        if ($hasMacro)
        {
            Write-Host ("  skip  {0}  {1}" -f $page.Id, $page.Title)
            $kept++
            continue
        }

        $node = [pscustomobject]@{
            type  = 'extension'
            attrs = [pscustomobject]@{
                layout        = 'default'
                extensionType = 'com.atlassian.confluence.macro.core'
                extensionKey  = 'change-history'
                parameters    = [pscustomobject]@{
                    macroParams   = [pscustomobject]@{}
                    macroMetadata = [pscustomobject]@{
                        macroId       = [pscustomobject]@{ value = [guid]::NewGuid().ToString() }
                        schemaVersion = [pscustomobject]@{ value = '1' }
                        title         = 'Change History'
                    }
                }
            }
        }

        $adf.content = @($node) + @($adf.content)

        if (-not $Apply)
        {
            Write-Host ("  ADD   {0}  {1}" -f $page.Id, $page.Title) -ForegroundColor Cyan
            $added++
            continue
        }

        $payload = [pscustomobject]@{
            id      = $detail.id
            status  = 'current'
            title   = $detail.title
            body    = [pscustomobject]@{
                representation = 'atlas_doc_format'
                value          = ($adf | ConvertTo-Json -Depth 100 -Compress)
            }
            version = [pscustomobject]@{
                number  = [int]$detail.version.number + 1
                message = '문서 버전(Change History) 매크로 추가'
            }
        }

        [void](Invoke-Api -Method Put -Url "$apiPrefix/wiki/api/v2/pages/$($page.Id)" `
            -JsonBody ($payload | ConvertTo-Json -Depth 100))

        Write-Host ("  done  {0}  {1}" -f $page.Id, $page.Title) -ForegroundColor Green
        $added++
    }
    catch
    {
        Write-Host ("  FAIL  {0}  {1}  =>  {2}" -f $page.Id, $page.Title, $_.Exception.Message) -ForegroundColor Red
        $failed++
    }
}

Write-Host ''
Write-Host ("완료. 추가 {0}건 / 이미 있음 {1}건 / 실패 {2}건" -f $added, $kept, $failed)
