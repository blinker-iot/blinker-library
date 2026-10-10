param(
    [string]$CppExecutable = "",
    [string]$NoiseCppExecutable = "",
    [string]$ControllerAuthCppExecutable = "",
    [string]$ControllerControlCppExecutable = "",
    [string]$BleEnrollmentCppExecutable = "",
    [string]$DeviceKeySessionCppExecutable = "",
    [string]$DirectSecureCppExecutable = "",
    [string]$EdgeGatewayCppExecutable = "",
    [string]$TypeScriptCompiler = ""
)

$ErrorActionPreference = "Stop"
$goldenDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$testsDir = Split-Path -Parent $goldenDir
$repositoryDir = Split-Path -Parent $testsDir
$workspaceDir = Split-Path -Parent $repositoryDir

function Resolve-GoldenExecutable {
    param([string]$Provided, [string]$BaseName)

    if (-not [string]::IsNullOrWhiteSpace($Provided) -and
        (Test-Path -LiteralPath $Provided -PathType Leaf)) {
        return $Provided
    }

    $candidates = @(
        (Join-Path $testsDir "host\build\Debug\$BaseName.exe"),
        (Join-Path $testsDir "host\build\Release\$BaseName.exe"),
        (Join-Path $testsDir "host\build\RelWithDebInfo\$BaseName.exe"),
        (Join-Path $testsDir "host\build\$BaseName.exe"),
        (Join-Path $testsDir "host/build/$BaseName")
    )
    $resolved = $candidates |
        Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($resolved)) {
        throw "Build $BaseName first or pass its executable explicitly."
    }
    return $resolved
}

$CppExecutable = Resolve-GoldenExecutable `
    $CppExecutable "blinker_device_v2_golden_tests"
$NoiseCppExecutable = Resolve-GoldenExecutable `
    $NoiseCppExecutable "blinker_noise_golden_tests"
$ControllerAuthCppExecutable = Resolve-GoldenExecutable `
    $ControllerAuthCppExecutable "blinker_controller_auth_golden_tests"
$ControllerControlCppExecutable = Resolve-GoldenExecutable `
    $ControllerControlCppExecutable "blinker_controller_control_golden_tests"
$BleEnrollmentCppExecutable = Resolve-GoldenExecutable `
    $BleEnrollmentCppExecutable "blinker_ble_enrollment_golden_tests"
$DeviceKeySessionCppExecutable = Resolve-GoldenExecutable `
    $DeviceKeySessionCppExecutable "blinker_device_key_session_golden_tests"
$DirectSecureCppExecutable = Resolve-GoldenExecutable `
    $DirectSecureCppExecutable "blinker_direct_secure_golden_tests"
$EdgeGatewayCppExecutable = Resolve-GoldenExecutable `
    $EdgeGatewayCppExecutable "blinker_edge_gateway_golden_tests"

if ([string]::IsNullOrWhiteSpace($TypeScriptCompiler)) {
    $compilerCandidates = @(
        (Join-Path $workspaceDir "blinker-app\node_modules\.bin\tsc.cmd"),
        (Join-Path $workspaceDir "blinker-app/node_modules/.bin/tsc"),
        (Join-Path $workspaceDir "node_modules\.bin\tsc.cmd"),
        (Join-Path $workspaceDir "node_modules/.bin/tsc")
    )
    $TypeScriptCompiler = $compilerCandidates |
        Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } |
        Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($TypeScriptCompiler)) {
        $pathCompiler = Get-Command tsc -ErrorAction SilentlyContinue
        if ($null -ne $pathCompiler) {
            $TypeScriptCompiler = $pathCompiler.Source
        }
    }
}
if ([string]::IsNullOrWhiteSpace($TypeScriptCompiler) -or
    -not (Test-Path -LiteralPath $TypeScriptCompiler -PathType Leaf)) {
    throw "Install the App dependencies or pass -TypeScriptCompiler."
}

$fixtures = @{
    Codec = Join-Path $goldenDir "bbp2_golden_vectors.properties"
    Noise = Join-Path $goldenDir "portable_noise_golden_vectors.properties"
    ControllerAuth = Join-Path $goldenDir "controller_auth_golden_vectors.properties"
    ControllerControl = Join-Path $goldenDir "controller_control_golden_vectors.properties"
    BleEnrollment = Join-Path $goldenDir "ble_enrollment_golden_vectors.properties"
    DeviceKeySession = Join-Path $goldenDir "device_key_session_golden_vectors.properties"
    DirectSecure = Join-Path $goldenDir "direct_secure_golden_vectors.properties"
    P256 = Join-Path $goldenDir "p256_identity_golden_vectors.properties"
    EdgeGateway = Join-Path $goldenDir "edge_gateway_golden_vectors.properties"
}
$cppCases = @(
    @($CppExecutable, $fixtures.Codec),
    @($NoiseCppExecutable, $fixtures.Noise),
    @($ControllerAuthCppExecutable, $fixtures.ControllerAuth),
    @($ControllerControlCppExecutable, $fixtures.ControllerControl),
    @($BleEnrollmentCppExecutable, $fixtures.BleEnrollment),
    @($DeviceKeySessionCppExecutable, $fixtures.DeviceKeySession),
    @($DirectSecureCppExecutable, $fixtures.DirectSecure),
    @($EdgeGatewayCppExecutable, $fixtures.EdgeGateway)
)
$appCases = @(
    @("app_codec_golden.ts", $fixtures.Codec),
    @("app_noise_golden.ts", $fixtures.Noise),
    @("app_controller_auth_golden.ts", $fixtures.ControllerAuth),
    @("app_controller_control_golden.ts", $fixtures.ControllerControl),
    @("app_device_key_session_golden.ts", $fixtures.DeviceKeySession),
    @("app_direct_secure_golden.ts", $fixtures.DirectSecure),
    @("app_edge_gateway_golden.ts", $fixtures.EdgeGateway)
)
$serverCases = @(
    @("server_codec_golden.cjs", $fixtures.Codec),
    @("server_noise_golden.cjs", $fixtures.Noise),
    @("server_controller_auth_golden.cjs", $fixtures.ControllerAuth),
    @("server_controller_control_golden.cjs", $fixtures.ControllerControl),
    @("server_ble_enrollment_golden.cjs", $fixtures.BleEnrollment),
    @("server_device_key_session_golden.cjs", $fixtures.DeviceKeySession),
    @("server_direct_secure_golden.cjs", $fixtures.DirectSecure),
    @("server_p256_identity_golden.cjs", $fixtures.P256),
    @("server_edge_gateway_golden.cjs", $fixtures.EdgeGateway)
)

$node = Get-Command node -ErrorAction Stop
$outputDir = Join-Path ([System.IO.Path]::GetTempPath()) `
    ("blinker-device-v2-golden-app-" + $PID)
$typeRoots = Join-Path $goldenDir "types"
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

try {
    foreach ($case in $cppCases) {
        & $case[0] $case[1]
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    foreach ($case in $appCases) {
        $source = Join-Path $goldenDir $case[0]
        & $TypeScriptCompiler `
            $source `
            --target ES2017 `
            --module commonjs `
            --strict `
            --skipLibCheck `
            --typeRoots $typeRoots `
            --types golden-harness `
            --outDir $outputDir
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

        $output = Join-Path $outputDir `
            (([System.IO.Path]::GetFileNameWithoutExtension($case[0])) + ".js")
        & $node.Source $output $case[1]
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    foreach ($case in $serverCases) {
        & $node.Source (Join-Path $goldenDir $case[0]) $case[1]
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}
finally {
    if (Test-Path -LiteralPath $outputDir) {
        Remove-Item -LiteralPath $outputDir -Recurse -Force
    }
}
