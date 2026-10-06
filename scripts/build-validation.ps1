$ErrorActionPreference='Stop'
New-Item -ItemType Directory artifacts -Force | Out-Null
foreach($item in @(@('build-native','Debug'),@('build-native','Release'),@('build-independent','Release'),@('build-analysis','Release'),@('build-asan','RelWithDebInfo'))) {
  & cmake --build $item[0] --config $item[1] 2>&1 | Tee-Object "artifacts/build-$($item[0])-$($item[1]).txt"
  if($LASTEXITCODE -ne 0) {throw "Build failed: $item"}
}
& cmake --install build-native --config Release --prefix install
if($LASTEXITCODE -ne 0) {throw 'Install failed'}
