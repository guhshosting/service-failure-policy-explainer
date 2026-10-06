param([string]$Artifacts = 'artifacts')
$ErrorActionPreference='Stop'
New-Item -ItemType Directory -Force $Artifacts | Out-Null
$vs = & 'C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe' -latest -products '*' -property installationPath
$vc = (Get-ChildItem "$vs/VC/Tools/MSVC" -Directory | Select-Object -Last 1).FullName
$sdk = (Get-ChildItem 'C:/Program Files (x86)/Windows Kits/10/Include' -Directory | Sort-Object Name | Select-Object -Last 1).FullName
$args = @('--extra-arg=/std:c++17','--extra-arg=/EHsc','--extra-arg=/DUNICODE','--extra-arg=/D_UNICODE','--extra-arg=/D_WIN32_WINNT=0x0A00','--extra-arg=/DNOMINMAX','--extra-arg=/DWIN32_LEAN_AND_MEAN',"--extra-arg=/I$vc/include", "--extra-arg=/I$sdk/ucrt", "--extra-arg=/I$sdk/shared", "--extra-arg=/I$sdk/um", '--extra-arg=/Isrc')
$failed=$false
foreach($file in Get-ChildItem src,tests -Filter *.cpp) {
  & 'C:/Program Files/LLVM/bin/clang-tidy.exe' $file.FullName '--header-filter=.*[/\\](src|tests)[/\\].*' '--checks=clang-analyzer-*,bugprone-*,performance-*' '--warnings-as-errors=*' @args -- --driver-mode=cl 2>&1 | Tee-Object "$Artifacts/tidy-$($file.BaseName).txt"
  if($LASTEXITCODE -ne 0) {$failed=$true}
}
& 'C:/Program Files/Cppcheck/cppcheck.exe' --enable=warning,performance,portability --error-exitcode=1 --std=c++17 --platform=win64 --inline-suppr -Isrc src tests 2>&1 | Tee-Object "$Artifacts/cppcheck.txt"
if($LASTEXITCODE -ne 0) {$failed=$true}
if($failed) {exit 1}
