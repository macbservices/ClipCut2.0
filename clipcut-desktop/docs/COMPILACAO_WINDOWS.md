# CLIP CUT 2.0 — GUIA TÉCNICO DE COMPILAÇÃO NO WINDOWS 10/11 x64

Este documento fornece as instruções reproduzíveis para compilar o **Clip Cut 2.0** a partir do código-fonte em um computador **Windows 10/11 x64**, utilizando ferramentas oficiais e a cadeia de compilação Microsoft Visual C++ 2022.

---

## 1. REQUISITOS DO AMBIENTE DE DESENVOLVIMENTO

Antes de iniciar, certifique-se de ter instalado:

1. **Visual Studio 2022 Community ou Professional:**
   - Cargas de trabalho obrigatórias: **Desenvolvimento para Desktop com C++**.
   - Componentes individuais: **MSVC v143 - Ferramentas de Build C++ x64/x86**, **SDK do Windows 10/11** e **CMake Tools para Windows**.
2. **Qt 6.10.x para Windows (MSVC 2022 64-bit):**
   - Baixe via instalador oficial do Qt (*Qt Online Installer*).
   - Componentes selecionados:
     - `Qt 6.10.x` > `MSVC 2022 64-bit`
     - `Qt Quick` / `Qt QML`
     - `Qt Shader Tools`
     - `Qt 5 Compatibility Module` (opcional)
   - Adicione `C:\Qt\6.10.x\msvc2022_64\bin` ao `PATH` do sistema.
3. **Git para Windows:** `git --version` >= 2.40.
4. **CMake:** `cmake --version` >= 3.21.
5. **Inno Setup 6 (Opcional):** Para gerar o instalador `ClipCut-2.0-Setup-x64.exe` a partir do script `.iss`.

---

## 2. PROCEDIMENTO AUTOMATIZADO COM POWERSHELL (RECOMENDADO)

O projeto inclui um script de compilação de ponta a ponta que automatiza a obtenção das bibliotecas e a montagem do pacote de distribuição:

1. Abra o **PowerShell** ou o **x64 Native Tools Command Prompt for VS 2022**.
2. Navegue até a pasta do projeto:
   ```powershell
   cd C:\caminho\para\ClipCut2
   ```
3. Permita a execução de scripts se necessário:
   ```powershell
   Set-ExecutionPolicy -Scope Process Bypass
   ```
4. Execute o script de compilação Release:
   ```powershell
   .\clipcut-desktop\scripts\build-windows-x64.ps1 -Config Release
   ```

### O que o script realiza automaticamente:
1. Localiza a instalação do Qt 6 em `C:\Qt\6.10.*\msvc2022_64` ou lê a variável `QT_ROOT_DIR`.
2. Clona o **vcpkg** (caso não exista) e instala pacotes dinâmicos (`zstd`, `openssl`, `soundtouch`) e estáticos (`skia`, `zlib`).
3. Baixa e descompacta os binários oficiais do **FFmpeg n7.1 GPL Shared** para Windows x64 em `.deps\ffmpeg`.
4. Configura o projeto via CMake com o gerador `"Visual Studio 17 2022"` na arquitetura `x64`.
5. Compila a solução gerando o executável nativo `ClipCut2.exe`.
6. Monta o diretório de distribuição `dist\bin`, copiando as DLLs do FFmpeg, Skia e dependências.
7. Executa o **`windeployqt.exe`** para coletar automaticamente os módulos QML e plugins de plataforma (`qwindows.dll`).
8. Compila o instalador via **Inno Setup** (`ISCC.exe`) e gera o arquivo ZIP portátil `dist\ClipCut-2.0-Windows-x64-Portable.zip`.

---

## 3. PROCEDIMENTO MANUAL PASSO A PASSO (CMAKE)

Caso deseje compilar manualmente via linha de comando ou Visual Studio IDE:

### Passo 1: Dependências via vcpkg
```powershell
git clone --depth 1 https://github.com/microsoft/vcpkg.git vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg.exe install zstd openssl soundtouch --triplet x64-windows
.\vcpkg\vcpkg.exe install zlib "skia[gl,harfbuzz,icu,freetype,png]" --triplet x64-windows-static-md
```

### Passo 2: FFmpeg n7.1 Headers e Bibliotecas
Baixe o pacote compartilhado de compilação n7.1 x64 do FFmpeg e descompacte em `C:\FFmpeg-n7.1`:
- Headers em: `C:\FFmpeg-n7.1\include`
- Bibliotecas `.lib` em: `C:\FFmpeg-n7.1\lib`
- DLLs em: `C:\FFmpeg-n7.1\bin`

### Passo 3: Configuração do CMake
```powershell
cmake -B build -S clipcut-desktop -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DCMAKE_PREFIX_PATH="C:\Qt\6.10.3\msvc2022_64;C:\FFmpeg-n7.1;vcpkg\installed\x64-windows-static-md;vcpkg\installed\x64-windows" `
    -DZLIB_INCLUDE_DIR="vcpkg\installed\x64-windows-static-md\include" `
    -DZLIB_LIBRARY="vcpkg\installed\x64-windows-static-md\lib\zs.lib" `
    -DDRIFT_BUNDLE_ONNXRUNTIME=OFF `
    -DDRIFT_BUNDLE_CORE_PACKAGES_ONLY=ON `
    -DDRIFT_WITH_SKIA=ON `
    -DDRIFT_DISTRIBUTION=windows
```

### Passo 4: Compilação
```powershell
cmake --build build --config Release --parallel 8
```

### Passo 5: Implantação de Bibliotecas com windeployqt
```powershell
mkdir -p dist\bin
copy build\Release\ClipCut2.exe dist\bin\
copy C:\FFmpeg-n7.1\bin\*.dll dist\bin\
copy vcpkg\installed\x64-windows\bin\*.dll dist\bin\

C:\Qt\6.10.3\msvc2022_64\bin\windeployqt.exe `
    --release `
    --no-translations `
    --no-opengl-sw `
    --qmldir clipcut-desktop\src\qml `
    dist\bin\ClipCut2.exe
```

---

## 4. SOLUÇÃO DE PROBLEMAS DE COMPILAÇÃO

| Sintoma / Erro | Causa Mais Comum | Solução Recomendada |
| :--- | :--- | :--- |
| `Qt6_DIR not found` | Variável `QT_ROOT_DIR` ou caminho no `-DCMAKE_PREFIX_PATH` incorreto. | Aponte para `C:\Qt\<versao>\msvc2022_64`, nunca para a pasta raiz do Qt. |
| `Cannot open include file: 'libavcodec/avcodec.h'` | Caminho do FFmpeg ausente ou versão incompatível. | Verifique se a pasta `include` do FFmpeg n7.1 está listada no `CMAKE_PREFIX_PATH`. |
| `Missing qwindows.dll platform plugin at launch` | O `windeployqt` não foi executado ou o diretório `plugins/platforms` não foi copiado. | Execute o comando `windeployqt` especificando `--qmldir` apontando para `src\qml`. |
| `LNK2038: mismatch detected for 'RuntimeLibrary'` | Conflito entre bibliotecas estáticas (MD vs MT). | Utilize estritamente os triplets `x64-windows` (dinâmico) e `x64-windows-static-md` (estático compatível com CRT dinâmica do MSVC). |
| `Module 'QtQuick.Controls' not installed` no QML | O `windeployqt` não escaneou todos os arquivos QML do projeto. | Assegure que a opção `--qmldir src/qml` foi passada ao invocar o `windeployqt`. |
