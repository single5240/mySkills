param(
    [ValidateSet('Prepare', 'Inspect', 'Seed', 'Review', 'Simulator', 'SetMode', 'SelfTest')]
    [string]$Action = 'Inspect',
    [Parameter(Mandatory = $true)][string]$BackendRoot,
    [string]$DeviceNo,
    [ValidateRange(1, 10)][int]$Count = 3,
    [ValidateSet('Success', 'Timeout')][string]$Mode = 'Success',
    [ValidateRange(1, 86400)][int]$DurationSeconds = 1800,
    [ValidateRange(-1, 65535)][int]$UsedCount = -1,
    [ValidateRange(-1, 65535)][int]$SurplusCount = -1,
    [string]$Since,
    [switch]$AllowWrite
)

$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path -LiteralPath $BackendRoot).Path
if (-not (Test-Path -LiteralPath (Join-Path $taskRoot 'dorta-admin/pom.xml'))) {
    throw 'BackendRoot 必须是已核实的 dorta-center 根目录。'
}
if ($Action -notin @('Prepare', 'SelfTest') -and $DeviceNo -notmatch '^[A-Za-z0-9_-]{13}$') {
    throw '必须显式指定获准测试的 13 位 ASCII DeviceNo。'
}
if ($Action -in @('Seed', 'Simulator') -and -not $AllowWrite) {
    throw '此操作会修改该设备的本地测试数据。仅在本轮已授权时传入 -AllowWrite。'
}
$taskRuntime = Join-Path $taskRoot 'dorta-admin/target/skill-local-mqtt-test'
if ($Action -eq 'SetMode') {
    if (-not (Test-Path -LiteralPath $taskRuntime)) { throw '先执行 Prepare。' }
    [System.IO.File]::WriteAllText((Join-Path $taskRuntime "$DeviceNo.mode"), $Mode.ToLowerInvariant(), [System.Text.UTF8Encoding]::new($false))
    Write-Output "只更新本地模拟模式：device=$DeviceNo mode=$Mode；未发送命令。"
    return
}

$taskJar = Join-Path $taskRoot 'dorta-admin/target/dorta-admin.jar'
if (-not (Test-Path -LiteralPath $taskJar)) { throw '先在后端根目录重新构建 dorta-admin.jar。' }
$taskHash = (Get-FileHash -LiteralPath $taskJar -Algorithm SHA256).Hash.Substring(0, 12).ToLowerInvariant()
$taskLibraries = Join-Path $taskRuntime "lib-$taskHash"
if ($Action -eq 'Prepare') {
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    if (-not (Test-Path -LiteralPath $taskLibraries)) { New-Item -ItemType Directory -Path $taskLibraries -Force | Out-Null }
    $taskArchive = [System.IO.Compression.ZipFile]::OpenRead($taskJar)
    try {
        $taskEntries = @($taskArchive.Entries | Where-Object { $_.FullName -match '^BOOT-INF/lib/[^/]+\.jar$' })
        if (-not $taskEntries.Count) { throw '不是包含 BOOT-INF/lib 的 Spring Boot 产物。' }
        foreach ($taskEntry in $taskEntries) {
            $taskDestination = Join-Path $taskLibraries $taskEntry.Name
            if (-not (Test-Path -LiteralPath $taskDestination)) {
                [System.IO.Compression.ZipFileExtensions]::ExtractToFile($taskEntry, $taskDestination)
            }
        }
        Write-Output "Prepare 完成：复用 $($taskEntries.Count) 个现有依赖，未连接数据库或 MQTT。"
    } finally { $taskArchive.Dispose() }
    return
}
if (-not (Test-Path -LiteralPath $taskLibraries)) { throw '当前 JAR 尚未 Prepare；构建更新后需重新 Prepare。' }
$taskClasses = Join-Path $taskRoot 'dorta-modules/dorta-iot/target/classes'
if (-not (Test-Path -LiteralPath (Join-Path $taskClasses 'org/dorta/iot/mqtt/protocol/IotMqttFrameBuilder.class'))) {
    throw '缺少已编译的 IoT 协议类；先编译后端。'
}
$taskJava = (Get-Command java.exe -ErrorAction Stop).Source
$taskSource = Join-Path $PSScriptRoot 'LocalMqttTestTool.java'
$taskClasspath = "$taskClasses;$taskLibraries/*"
$taskConfig = Join-Path $taskRoot 'dorta-admin/src/main/resources/application-local.yml'
$taskDeviceArgument = if ($DeviceNo) { $DeviceNo } else { '-' }
$taskSinceArgument = if ($Since) { $Since } else { '-' }
$taskPermission = if ($AllowWrite) { 'allow-write' } else { 'read-only' }
$taskArguments = @('-Dfile.encoding=UTF-8', '--class-path', $taskClasspath, $taskSource,
    $Action.ToLowerInvariant(), $taskConfig, $taskDeviceArgument, $taskRuntime,
    "$Count", $Mode.ToLowerInvariant(), "$DurationSeconds", "$UsedCount", "$SurplusCount", $taskSinceArgument, $taskPermission)
& $taskJava @taskArguments
if ($LASTEXITCODE -ne 0) { throw "本地测试工具失败，exit=$LASTEXITCODE；未自动重试。" }
