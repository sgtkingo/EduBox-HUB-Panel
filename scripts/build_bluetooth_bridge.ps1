param(
    [string]$ArduinoCli = "arduino-cli",
    [int]$Jobs = 4
)
$ErrorActionPreference = "Stop"
$bridgePanelRoot = Split-Path -Parent $PSScriptRoot
$bridgeBuildPath = Join-Path $bridgePanelRoot ".build/bluetooth_bridge"
$bridgeLibraries = Join-Path $bridgePanelRoot "libraries"
$bridgeSketch = Join-Path $bridgePanelRoot "ui"
$bridgeFqbn = "esp32:esp32:esp32s3:UploadSpeed=921600,USBMode=hwcdc,CDCOnBoot=default,MSCOnBoot=default,DFUOnBoot=default,UploadMode=default,CPUFreq=240,FlashMode=qio,FlashSize=4M,PartitionScheme=huge_app,DebugLevel=none,PSRAM=opi,LoopCore=1,EventsCore=1,EraseFlash=all,JTAGAdapter=default,ZigbeeMode=default"
$bridgeFlags = "-DCONFIG_BT_NIMBLE_MAX_CONNECTIONS=1 -DMYNEWT_VAL_BLE_SM_SC_ONLY=1 -DMYNEWT_VAL_BLE_SM_LEGACY=0"
& $ArduinoCli compile --jobs $Jobs --fqbn $bridgeFqbn --libraries $bridgeLibraries --build-path $bridgeBuildPath --build-property "compiler.cpp.extra_flags=$bridgeFlags" --build-property "compiler.c.extra_flags=$bridgeFlags" $bridgeSketch
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
Write-Host "Firmware built without upload: $bridgeBuildPath"
