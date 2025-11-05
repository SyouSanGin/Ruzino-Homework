# DLL 加载诊断工具 (PowerShell 版本)
# 用法: .\diagnose_dll.ps1 -ModuleName GUI_py -BinariesPath C:\path\to\Binaries\Release

param(
    [Parameter(Mandatory=$true)]
    [string]$ModuleName,
    
    [Parameter(Mandatory=$true)]
    [string]$BinariesPath,
    
    [string]$PythonExe = "python"
)

function Write-Diagnostic {
    param([string]$Message)
    Write-Host "[DIAGNOSTIC] $Message" -ForegroundColor Cyan
}

function Write-Error-Msg {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

Write-Host "`n========== DLL 诊断工具 ==========" -ForegroundColor Yellow
Write-Diagnostic "开始诊断 DLL 加载问题"
Write-Diagnostic "模块名: $ModuleName"
Write-Diagnostic "二进制文件路径: $BinariesPath"
Write-Diagnostic "Python 可执行文件: $PythonExe"
Write-Host ""

# 检查路径是否存在
if (-not (Test-Path $BinariesPath)) {
    Write-Error-Msg "路径不存在: $BinariesPath"
    exit 1
}

Write-Diagnostic "路径验证通过"
Write-Host ""

# 列出 Binaries 目录的内容
Write-Host "========== Binaries 目录内容 ==========" -ForegroundColor Yellow
Write-Diagnostic "Binaries 目录: $BinariesPath"

$dllFiles = Get-ChildItem -Path $BinariesPath -Filter "*.dll" -ErrorAction SilentlyContinue
if ($dllFiles) {
    Write-Diagnostic "找到 DLL 文件:"
    foreach ($dll in $dllFiles) {
        $size = $dll.Length
        Write-Host "  - $($dll.Name) ($($size) 字节)"
    }
} else {
    Write-Error-Msg "未找到 DLL 文件!"
}

Write-Host ""

# 检查是否存在目标模块对应的 PYD 文件
$pydFile = Join-Path $BinariesPath "$ModuleName.pyd"
if (Test-Path $pydFile) {
    Write-Success "找到模块文件: $pydFile"
    $pydSize = (Get-Item $pydFile).Length
    Write-Diagnostic "文件大小: $pydSize 字节"
} else {
    Write-Error-Msg "未找到模块文件: $pydFile"
}

Write-Host ""

# 尝试列出所有相关的 OpenUSD DLL
Write-Host "========== OpenUSD 相关 DLL ==========" -ForegroundColor Yellow
$usdDlls = Get-ChildItem -Path $BinariesPath -Filter "*usd*.dll" -ErrorAction SilentlyContinue
$vdbDlls = Get-ChildItem -Path $BinariesPath -Filter "*vdb*.dll" -ErrorAction SilentlyContinue
$tbbDlls = Get-ChildItem -Path $BinariesPath -Filter "*tbb*.dll" -ErrorAction SilentlyContinue

if ($usdDlls) {
    Write-Diagnostic "USD 相关 DLL:"
    foreach ($dll in $usdDlls) {
        Write-Host "  - $($dll.Name)"
    }
} else {
    Write-Error-Msg "未找到 USD DLL!"
}

Write-Host ""

if ($vdbDlls) {
    Write-Diagnostic "OpenVDB 相关 DLL:"
    foreach ($dll in $vdbDlls) {
        Write-Host "  - $($dll.Name)"
    }
} else {
    Write-Error-Msg "未找到 OpenVDB DLL!"
}

Write-Host ""

if ($tbbDlls) {
    Write-Diagnostic "TBB 相关 DLL:"
    foreach ($dll in $tbbDlls) {
        Write-Host "  - $($dll.Name)"
    }
} else {
    Write-Diagnostic "未找到 TBB DLL (可能不需要)"
}

Write-Host ""

# 检查系统路径
Write-Host "========== 系统 PATH 环境变量 ==========" -ForegroundColor Yellow
Write-Diagnostic "前 10 个 PATH 条目:"
$pathEntries = $env:PATH -split ";"
for ($i = 0; $i -lt [Math]::Min(10, $pathEntries.Length); $i++) {
    if ($pathEntries[$i]) {
        Write-Host "  [$i] $($pathEntries[$i])"
    }
}

Write-Host ""

# 尝试使用 Python 诊断脚本
Write-Host "========== Python 诊断 ==========" -ForegroundColor Yellow
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$diagScript = Join-Path $scriptDir "diagnose_dll.py"

if (Test-Path $diagScript) {
    Write-Diagnostic "运行 Python 诊断脚本..."
    Write-Host ""
    
    & $PythonExe $diagScript $ModuleName $BinariesPath
    
    if ($LASTEXITCODE -eq 0) {
        Write-Success "Python 诊断成功"
    } else {
        Write-Error-Msg "Python 诊断失败 (退出代码: $LASTEXITCODE)"
    }
} else {
    Write-Error-Msg "未找到 Python 诊断脚本: $diagScript"
}

Write-Host ""
Write-Host "========== 诊断完成 ==========" -ForegroundColor Yellow

# 建议
Write-Host ""
Write-Host "可能的解决方案:" -ForegroundColor Yellow
Write-Host "1. 检查是否所有必需的 DLL 都存在于 $BinariesPath"
Write-Host "2. 使用 Dependency Walker (depends.exe) 检查 $pydFile 的依赖关系"
Write-Host "3. 验证 Python 版本是否匹配 (32-bit vs 64-bit)"
Write-Host "4. 重新编译 OpenUSD 或重新提取 SDK"
Write-Host ""
