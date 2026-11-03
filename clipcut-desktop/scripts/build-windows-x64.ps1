<#
.SYNOPSIS
    Script automatizado de compilação do Clip Cut 2.0 para Windows 10/11 x64.
.DESCRIPTION
    Configura vcpkg, FFmpeg n7.1, compila via CMake/MSVC 2022, executa windeployqt
    e gera o pacote instalador Inno Setup e versão portátil zip.
#>
param (
    [string]$QtDir = $env:QT_ROOT_DIR,
    [string]$VcpkgDir = "$PSScriptRoot\..\vcpkg",
    [string]$Config = "Release",
    [switch]$SkipVcpkg = $false,
    [switch]$SkipFFmpeg = $false,
    [switch]$PackageInstaller = $true
)

$ErrorActionPreference = "Stop"
$ScriptRoot = $PSScriptRoot
$ProjectRoot = Resolve-Path "$ScriptRoot\.."

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "   CLIP CUT 2.0 - SCRIPT DE COMPILAÇÃO WINDOWS X64        " -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "Diretório do Projeto: $ProjectRoot"

# 1. Verificar Qt
if (-not $QtDir) {
    if (Test-Path "C:\Qt\6.10.3\msvc2022_64") {
        $QtDir = "C:\Qt\6.10.3\msvc2022_64"
    } elseif (Test-Path "C:\Qt\6.8.2\msvc2022_64") {
        $QtDir = "C:\Qt\6.8.2\msvc2022_64"
    } else {
        Write-Error "Qt 6 não encontrado! Defina -QtDir ou a variável de ambiente QT_ROOT_DIR apontando para o kit msvc2022_64."
    }
}
Write-Host "[✓] Qt 6 localizado em: $QtDir" -ForegroundColor Green

# 2. Inicializar vcpkg
if (-not $SkipVcpkg) {
    Write-Host "`n[1/6] Configurando dependências via vcpkg..." -ForegroundColor Yellow
    if (-not (Test-Path "$VcpkgDir\vcpkg.exe")) {
        Write-Host "Clonando vcpkg..."
        git clone --depth 1 https://github.com/microsoft/vcpkg.git $VcpkgDir
        & "$VcpkgDir\bootstrap-vcpkg.bat"
    }

    Write-Host "Instalando pacotes dinâmicos (x64-windows)..."
    & "$VcpkgDir\vcpkg.exe" install zstd openssl soundtouch --triplet x64-windows

    Write-Host "Instalando Skia e zlib estático-md (x64-windows-static-md)..."
    & "$VcpkgDir\vcpkg.exe" install zlib "skia[gl,harfbuzz,icu,freetype,png]" --triplet x64-windows-static-md
}

# 3. Baixar FFmpeg n7.1 Pinned
$FfmpegDir = "$ProjectRoot\.deps\ffmpeg"
if (-not $SkipFFmpeg -and -not (Test-Path "$FfmpegDir\lib\avcodec.lib")) {
    Write-Host "`n[2/6] Baixando FFmpeg n7.1 GPL compartilhado..." -ForegroundColor Yellow
    $FfmpegZip = "$ProjectRoot\.deps\ffmpeg.zip"
    New-Item -ItemType Directory -Force -Path "$ProjectRoot\.deps" | Out-Null
    
    $FfmpegUrl = "https://github.com/CutWire-Studios/FFmpeg-Builds/releases/download/n7.1/ffmpeg-n7.1-win64-gpl-shared.zip"
    Invoke-WebRequest -Uri $FfmpegUrl -OutFile $FfmpegZip
    Expand-Archive -Path $FfmpegZip -DestinationPath "$ProjectRoot\.deps\ffmpeg_extracted" -Force
    $ExtractedRoot = (Get-ChildItem "$ProjectRoot\.deps\ffmpeg_extracted")[0].FullName
    New-Item -ItemType Directory -Force -Path $FfmpegDir | Out-Null
    Copy-Item -Path "$ExtractedRoot\*" -Destination $FfmpegDir -Recurse -Force
    Remove-Item -Force $FfmpegZip
    Remove-Item -Recurse -Force "$ProjectRoot\.deps\ffmpeg_extracted"
}
Write-Host "[✓] FFmpeg n7.1 pronto em: $FfmpegDir" -ForegroundColor Green

# 4. Configurar CMake
Write-Host "`n[3/6] Configurando o CMake (Visual Studio 2022 x64)..." -ForegroundColor Yellow
$BuildDir = "$ProjectRoot\build"

cmake -B $BuildDir -S $ProjectRoot -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_BUILD_TYPE=$Config `
    -DCMAKE_TOOLCHAIN_FILE="$VcpkgDir\scripts\buildsystems\vcpkg.cmake" `
    -DCMAKE_PREFIX_PATH="$QtDir;$FfmpegDir;$VcpkgDir\installed\x64-windows-static-md;$VcpkgDir\installed\x64-windows" `
    -DZLIB_INCLUDE_DIR="$VcpkgDir\installed\x64-windows-static-md\include" `
    -DZLIB_LIBRARY="$VcpkgDir\installed\x64-windows-static-md\lib\zs.lib" `
    -DDRIFT_BUNDLE_ONNXRUNTIME=OFF `
    -DDRIFT_BUNDLE_CORE_PACKAGES_ONLY=ON `
    -DDRIFT_WITH_SKIA=ON `
    -DDRIFT_DISTRIBUTION=windows

# 5. Compilar
Write-Host "`n[4/6] Compilando a solução ($Config)..." -ForegroundColor Yellow
cmake --build $BuildDir --config $Config --parallel 4

# 6. Empacotar DLLs com windeployqt
Write-Host "`n[5/6] Montando árvore de distribuição dist\bin..." -ForegroundColor Yellow
$DistBin = "$ProjectRoot\dist\bin"
New-Item -ItemType Directory -Force -Path $DistBin | Out-Null

if (Test-Path "$BuildDir\$Config\ClipCut2.exe") {
    Copy-Item "$BuildDir\$Config\ClipCut2.exe" "$DistBin\" -Force
} elseif (Test-Path "$BuildDir\$Config\drift.exe") {
    Copy-Item "$BuildDir\$Config\drift.exe" "$DistBin\ClipCut2.exe" -Force
}
Copy-Item "$FfmpegDir\bin\*.dll" "$DistBin\" -Force
Copy-Item "$VcpkgDir\installed\x64-windows\bin\*.dll" "$DistBin\" -Force -ErrorAction SilentlyContinue
Copy-Item "$ProjectRoot\LICENSE" "$DistBin\LICENSE.txt" -Force

# Copiar modelos locais se presentes
if (Test-Path "$ProjectRoot\models") {
    Write-Host "Copiando modelos locais ONNX..."
    Copy-Item "$ProjectRoot\models" "$DistBin\" -Recurse -Force
}

# Executar windeployqt
Write-Host "Executando windeployqt..."
& "$QtDir\bin\windeployqt.exe" --$($Config.ToLower()) --no-translations --no-opengl-sw --qmldir "$ProjectRoot\src\qml" "$DistBin\ClipCut2.exe"

# 7. Inno Setup Installer
if ($PackageInstaller) {
    Write-Host "`n[6/6] Criando instalador Inno Setup e pacote ZIP portátil..." -ForegroundColor Yellow
    $Iscc = "C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
    if (Test-Path $Iscc) {
        & $Iscc /DMyAppVersion="2.0.0" /DMyAppSource="$DistBin" "$ProjectRoot\installer\windows\ClipCut2.iss"
        Write-Host "[✓] Instalador gerado com sucesso em installer\windows\output\ClipCut-2.0-Setup-x64.exe!" -ForegroundColor Green
    } else {
        Write-Warning "Inno Setup (ISCC.exe) não encontrado em $Iscc. Você pode compilar manualmente o arquivo installer\windows\ClipCut2.iss."
    }

    # Gerar ZIP portátil
    $ZipOutput = "$ProjectRoot\dist\ClipCut-2.0-Windows-x64-Portable.zip"
    Write-Host "Comprimindo pacote portátil: $ZipOutput..."
    Compress-Archive -Path "$DistBin\*" -DestinationPath $ZipOutput -Force
    Write-Host "[✓] Pacote portátil ZIP gerado em: $ZipOutput" -ForegroundColor Green
}

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "   COMPILAÇÃO DO CLIP CUT 2.0 CONCLUÍDA COM SUCESSO!     " -ForegroundColor Green
Write-Host "   Executável disponível em: $DistBin\ClipCut2.exe       " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
