param([string]$Executable, [string]$Tests, [string]$Artifacts = 'artifacts')
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force $Artifacts | Out-Null
function Assert($value, $message) { if (-not $value) { throw $message } }
$fixture = & $Tests
Assert ($LASTEXITCODE -eq 0) 'Synthetic tests failed'
$parsed = $fixture | ConvertFrom-Json
Assert ($parsed.service.name -eq '服务') 'Unicode name roundtrip failed'
Assert ($parsed.recovery_command -eq '工具.exe --値=秘密') 'Unicode command roundtrip failed'
Assert ($parsed.failure_actions.Count -eq 4) 'Action array mismatch'
$fixture | Set-Content -Encoding utf8 "$Artifacts/fixture.json"
$results = @()
foreach ($name in @('Spooler','EventLog','RpcSs','DcomLaunch','W32Time','BITS','WinDefend','Netlogon','Null','__sfp_missing_service__','服务不存在')) {
  $raw = & $Executable --json $name 2> "$Artifacts/error-$name.txt"
  $exit = $LASTEXITCODE
  if ($exit -eq 0) {
    $r = $raw | ConvertFrom-Json
    Assert ($r.schema_version -eq 1) 'Invalid schema'
    Assert ($r.application_version -eq '1.0.0') 'Invalid version'
    Assert ($r.service.name -eq $name) 'Service identity mismatch'
    Assert ($null -ne $r.failure_actions) 'Missing actions'
    foreach ($a in $r.failure_actions) { Assert ($a.index -ge 1 -and $a.delay_ms -ge 0) 'Invalid action' }
    $raw | Set-Content -Encoding utf8 "$Artifacts/live-$name.json"
    $results += [pscustomobject]@{name=$name;exit=$exit;actions=$r.failure_actions.Count;non_crash=$r.failure_actions_on_non_crash_failures}
  } else {
    Assert ($exit -in @(3,4,5)) "Unexpected exit $exit for $name"
    Assert (-not $raw) 'Error contaminated JSON stdout'
    $results += [pscustomobject]@{name=$name;exit=$exit;actions=$null;non_crash=$null}
  }
}
$results | ConvertTo-Json | Set-Content "$Artifacts/live-summary.json"
$results | Format-Table
$bad = & $Executable --fix 2> "$Artifacts/usage-error.txt"
Assert ($LASTEXITCODE -eq 2 -and -not $bad) 'Usage exit mismatch'
$source = (Get-ChildItem src -File | Get-Content -Raw) -join "`n"
$banned = '(ChangeServiceConfig|StartService|ControlService|CreateService|DeleteService|InitiateSystemShutdown|ExitWindows|CreateProcess|ShellExecute|WinExec|RegSetValue|WSAStartup|WinHttp|WinInet|getaddrinfo|socket\s*\(|\bsystem\s*\(|\b_?[wp]?popen\s*\()'
Assert (-not ($source -match $banned)) 'Forbidden production API detected'
$dumpbin = Get-ChildItem 'C:/Program Files (x86)/Microsoft Visual Studio/*/BuildTools/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe' | Select-Object -Last 1
$imports = & $dumpbin.FullName /imports $Executable
Assert ($LASTEXITCODE -eq 0) 'Import audit failed'
$imports | Set-Content "$Artifacts/imports.txt"
Assert (-not (($imports -join "`n") -match $banned)) 'Forbidden import detected'
Assert (-not (($imports -join "`n") -match '(WS2_32|WINHTTP|WININET)\.dll')) 'Networking library imported'
whoami /groups | Set-Content "$Artifacts/token-groups.txt"
Get-ItemProperty 'HKLM:/SOFTWARE/Microsoft/Windows NT/CurrentVersion' | Select-Object ProductName,DisplayVersion,CurrentBuild,UBR | ConvertTo-Json | Set-Content "$Artifacts/platform.json"
Write-Output 'JSON, Unicode, live SCM, exit codes and source/import audits passed.'
