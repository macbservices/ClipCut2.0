# AUDITORIA TÉCNICA DO DRIFT — FASE 1
## Projeto Clip Cut 2.0
**Data de Execução:** Novembro de 2026  
**Repositório Auditado:** `https://github.com/CutWire-Studios/Drift`  
**Commit Analisado:** `e6ddbae7e80ec310ad8fb6242e00fb4029f079f3`  
**Autor do Commit:** Suhas Dissanayake (`suhasdissa@gmail.com`)  
**Mensagem:** `feat: editor improvements based on Idelson Queiroz Boone's work (#406)`  
**Versão Base Identificada:** Drift `0.9.0` (Schema de Projeto: v21)  

---

## 1. RESUMO EXECUTIVO E VIABILIDADE
O Drift é um editor de vídeo desktop e mobile nativo de alta performance escrito em **C++20** com interface gráfica declarativa em **Qt 6 / QML**, renderização vetorial com **Google Skia**, processamento de áudio profissional com **JUCE** e **SoundTouch**, decodificação/codificação multimídia via **FFmpeg 7.1** e subsistema de inferência de inteligência artificial local via **ONNX Runtime (OrtRuntime)**.

**Conclusão Fundamental:** O Drift **já possui nativamente**:
1. Um subsistema **MCP (Model Context Protocol)** robusto e completo (`McpController`, `McpDispatcher`, `McpServer`, `McpStdioServer`), expondo mais de 73 operações estruturadas em JSON sobre a timeline, clips, faixas, áudio, renderização e inteligência artificial.
2. Mecanismos locais de **Detecção de Silêncio** (`energy` em dB e `vad` neural via Silero VAD) e **Corte Automático Não-Destrutivo** (`detect_silence`, `remove_silence`, `AudioAlignController`, `CutOps::packedSegments`).
3. Transcrição local de voz para texto baseada em **OpenAI Whisper (whisper-small)** rodando em **ONNX Runtime** com alinhamento fonético CTC (`WhisperTranscriber`, `CtcAligner`, `SileroVad`), com suporte completo a português (`pt-BR`).
4. Arquitetura com **QUndoStack** e semântica imutável de snapshots com hash SHA-256 (`ProjectSnapshotCommand`, `HistoryState`), garantindo Undo/Redo atômico para qualquer operação da IA ou do usuário.
5. Motor de exportação real com **FFmpeg** (`Exporter`), suportando 16:9, 9:16 e 1:1, H.264/H.265/AAC, medição de sonoridade ITU-R BS.1770 (LUFS) e controle de cancelamento/progresso assíncrono.

---

## 2. ARQUITETURA DOCUMENTADA DO DRIFT

### 2.1 Camadas do Sistema (System Layers)

```
┌────────────────────────────────────────────────────────────────────────┐
│                   CLIP CUT 2.0 - CAMADA DE INTERFACE                   │
│  Main.qml | TimelinePanel.qml | PreviewPanel.qml | PropertiesPanel.qml │
│  AssetsPanel.qml | [NOVO] AiAssistantPanel.qml (Gemini Chat + Plano)   │
└────────────────────────────────────┬───────────────────────────────────┘
                                     │ C++ Q_PROPERTY / Slots
┌────────────────────────────────────▼───────────────────────────────────┐
│                      CAMADA DE APLICAÇÃO (src/app/)                    │
│  - AppController: Estado global do editor (EditorState singleton)       │
│  - ProjectFileController: Salvamento, autosave, recuperação e pacotes  │
│  - McpController: Controlador de automação MCP (HTTP/Stdio/In-Process) │
│  - McpDispatcher: Despachante de comandos JSON tipados e validados     │
│  - GeminiClient (Clip Cut 2.0): Tradução de linguagem natural em JSON  │
└──────────────────┬───────────────────────────────┬─────────────────────┘
                   │                               │
┌──────────────────▼──────────────┐ ┌──────────────▼─────────────────────┐
│  DOMÍNIO DO PROJETO (src/core/) │ │    MOTOR DE EXECUÇÃO (src/engine/) │
│  - Project (v21) & TrackList    │ │  - decode/ MediaProbe & ClipReader │
│  - Clip & Track (Value Semantics│ │  - encode/ Exporter (FFmpeg HW)    │
│  - CutOps & TimelineOps         │ │  - audio/ AudioMixer & Loudness    │
│  - ProjectSnapshotCommand       │ │  - compositor/ OpenGL 3.3          │
│    (QUndoStack + SHA-256 state) │ │  - ml/ OrtRuntime, Whisper, Vad    │
│  - SubtitleCue & Transcript     │ │  - render2d/ Skia (Ganesh/GL)      │
└─────────────────────────────────┘ └────────────────────────────────────┘
```

### 2.2 Estrutura Modular Real dos Arquivos

| Camada / Módulo | Caminho no Código-Fonte | Componentes e Classes Reais |
| :--- | :--- | :--- |
| **Ponto de Entrada** | `src/app/main.cpp` | `main()`, inicialização de Qt 6, registro de tipos QML, ativação de `--mcp-stdio`, inicialização de OpenGL e áudio. |
| **Controlador Mestre** | `src/app/editor/AppController.h` / `.cpp` | `class AppController : public QObject`. Expõe `EditorState` para o QML. Contém seleção, playhead, reprodução, histórico, trilhas, clipes e o ponteiro `mcp()`. |
| **Projetos e Autosave** | `src/app/editor/ProjectFileController.h` / `.cpp` | `class ProjectFileController : public QObject`. Gerencia carregamento/salvamento (`.drift`), autosave periódico, recuperação de falhas e empacotamento (`ProjectBundle`). |
| **MCP: Servidores** | `src/app/mcp/transport/McpServer.h` / `McpStdioServer.h` | `class McpServer` (transporte HTTP localhost com bearer token) e `class McpStdioServer` (transporte stdio IPC para subprocessos de automação). |
| **MCP: Despachante** | `src/app/mcp/tools/McpDispatcher.h` / `.cpp` | `class McpDispatcher`. Valida JSON contra schemas tipados e despacha operações: `opImportMedia`, `opSplitClip`, `opDeleteClip`, `opMoveClip`, `opUndo`, `opRedo`, etc. |
| **MCP: Controlador** | `src/app/mcp/McpController.h` / `.cpp` | `class McpController`. Executa 73 operações de alto nível: `detectSilence`, `removeSilence`, `transcribe`, `getTranscript`, `diarize`, `cutWords`, `normalizeVolume`, `audioSync`, `exportVideo`. |
| **Modelo de Dados** | `src/core/project/Project.h` / `.cpp` | `class Project`. Semântica de valor imutável para snapshots com cópia profunda (`TrackList::deepDetach`). Versão do schema: `kCurrentVersion = 21`. |
| **Operações de Corte** | `src/core/edit/CutOps.h` / `.cpp` | `sliceClipExact()`, `keptTimelineIntervals()`, `packedSegments()`, `segmentsFromSourceRanges()`. Suporta corte com declick de áudio e preservação de IDs. |
| **Timeline e Snapping** | `src/core/edit/TimelineOps.h` / `.cpp` | `struct SnapTargets`, `snapTimeTo()`, `clampClipStartNoOverlap()`, `resolveClipStart()`. Resolução e ordenação temporal em microssegundos (`TimeUs`). |
| **Undo / Redo** | `src/core/edit/commands/ProjectCommands.h` / `.cpp` | `class HistoryState` (mantém snapshot com hash SHA-256 e referências imutáveis) e `class ProjectSnapshotCommand : public QUndoCommand`. |
| **Decodificação de Mídia** | `src/engine/decode/probe/MediaProbe.h` & `readers/ClipReader.h` | Sondagem de metadados FFmpeg (`MediaProbe`) e decodificador assíncrono multithread com aceleração de hardware (`D3d11GlInterop.h` para Direct3D 11 / OpenGL no Windows). |
| **Exportação FFmpeg** | `src/engine/encode/export/Exporter.h` / `.cpp` | `class Exporter`. Renderização quadro a quadro direto do `FrameCompositor` e mixagem do `AudioMixer`. Suporta H.264, HEVC/H.265, ProRes, CRF/Bitrate e cancelamento seguro. |
| **IA: Runtime ONNX** | `src/engine/ml/runtime/OrtRuntime.h` / `.cpp` | `class OrtRuntime`. Gerenciador singleton da biblioteca Microsoft ONNX Runtime C++ API, suportando CPU e DirectML no Windows. |
| **IA: Detecção de Voz** | `src/engine/ml/speech/transcription/SileroVad.h` / `.cpp` | `class SileroVad`. Inferência do modelo neural `silero_vad.onnx` a 16 kHz para classificação precisa de quadros com fala humana vs. silêncio/ruído. |
| **IA: Transcrição Whisper**| `src/engine/ml/speech/transcription/WhisperTranscriber.h` / `.cpp` | `class WhisperTranscriber`. Transcrição completa via ONNX (`openai/whisper-small` em fp16: `encoder_model_fp16.onnx`, `decoder_model_fp16.onnx`, `vocab.json`). Gera `QList<SubtitleCue>`. |
| **Áudio & Sonoridade** | `src/engine/audio/analysis/LoudnessMeter.h` / `AudioAlign.h` | Medição de sonoridade integrada ITU-R BS.1770 (LUFS), True Peak, alinhamento temporal por correlação de onda e normalização de ganho. |
| **Interface QML** | `src/qml/Main.qml`, `TimelinePanel.qml`, `PreviewPanel.qml`, `AssetsPanel.qml`, `PropertiesPanel.qml` | Interface desktop Qt Quick 6.10 fluida e responsiva com aceleração gráfica, temas escuro/claro e docking de janelas. |

---

## 3. INVENTÁRIO DE LICENÇAS E COMPLIANCE

O Clip Cut 2.0 distribui binários nativos no Windows. A conformidade de licenças foi minuciosamente auditada:

| Componente | Versão / Tag | Licença | Impacto / Obrigações para o Clip Cut 2.0 |
| :--- | :--- | :--- | :--- |
| **Drift (Base)** | Commit `e6ddbae` (v0.9.0) | **GPL-3.0-only** (`LICENSE`) | O código do Clip Cut 2.0 derivado do Drift deve permanecer sob **GPL-3.0**. O código fonte deve ser disponibilizado conforme termos da GPLv3 ao distribuir os binários. |
| **Qt 6 Framework** | 6.10.3 | **LGPLv3 / GPLv3** | No Windows, a compilação usa Qt dinâmico (DLLs carregadas em runtime via `windeployqt`). Permite substituição de bibliotecas conforme LGPLv3. |
| **FFmpeg Builds** | `CutWire-Studios/FFmpeg-Builds` (n7.1) | **GPLv3** (Build com `libx264`, `libx265`, `gpl-shared`) | Como inclui x264/x265 para encode H.264/HEVC, o build do FFmpeg é explicitamente GPLv3. Perfeitamente compatível com o Drift (GPLv3). |
| **Google Skia** | `m131` / vcpkg port | **BSD 3-Clause** | Compatível com GPLv3 e código proprietário. Requer manutenção dos avisos de copyright. |
| **JUCE Framework** | `DRIFT_JUCE_TAG` | **GPLv3** (Free Open Source) | JUCE é configurado em modo open-source GPLv3. 100% alinhado com a licença do Drift. |
| **Microsoft ONNX Runtime** | 1.20+ | **MIT License** | Compatível e permissiva. |
| **Silero VAD** | v4 / v5 | **MIT License** | Modelo ONNX aberto e gratuito. Permite uso local irrestrito sem taxas ou autenticação. |
| **OpenAI Whisper (Weights)**| whisper-small | **MIT License** | Pesos e código abertos, permitindo execução local sem restrição de royalties. |
| **SoundTouch** | 2.3.3 | **LGPL-2.1** | Biblioteca de pitch-shifting e time-stretching. Compatível. |
| **OpenSSL** | 3.x | **Apache 2.0** | Criptografia e verificação de integridade de addons. |
| **zstd & zlib** | zstd 1.5.x, zlib 1.3.x | **BSD / zlib License** | Permissivas. |
| **Tipografia & Ícones** | Inter, HarmonyOS, Lucide | **SIL OFL 1.1 / Apache 2.0 / ISC** | Presentes em `resources/licenses/LICENSE-inter.txt` e `LICENSE-lucide.txt`. |

**Conclusão de Licenciamento:** Todo o ecossistema do Drift é estritamente compatível com GPLv3. O modelo híbrido com Google Gemini (que atua como oráculo de instrução via API HTTPS externa) não viola a GPLv3, pois o editor se comunica com o Gemini via cliente de rede padrão (REST HTTPS), mantendo o motor do editor livre e independente de código proprietário embutido.

---

## 4. MAPA DAS INTEGRAÇÕES EXISTENTES NO DRIFT

### 4.1 MCP (Model Context Protocol) Existente
O Drift possui a arquitetura MCP mais avançada entre editores de vídeo de código aberto:
- **Transporters:**
  - **HTTP (`McpServer.cpp`):** Inicia na porta configurada (ex: `--mcp-port 44101`) com autenticação via token Bearer (`--mcp-token <uuid>`).
  - **Stdio (`McpStdioServer.cpp`):** Lê mensagens JSON-RPC 2.0 de `stdin` e responde em `stdout`. Permite acoplamento sem rede por subprocessos.
  - **In-Process (`McpController::dispatcher()`):** Permite despacho direto em memória C++ sem serialização por socket.
- **Ferramentas MCP Existentes Relevantes para o MVP:**
  - `import_media`: Importa mídia para o bin do projeto.
  - `detect_silence`: Detecta trechos silenciosos com método `energy` ou `vad`, retornando array de intervalos `{start, end}`.
  - `remove_silence`: Realiza o corte atômico dos silêncios na timeline com empacotamento contínuo de clips.
  - `split_clip`: Divide o clipe na coordenada temporal indicada (`at`).
  - `delete_clip`: Remove o clipe indicado por ID.
  - `move_clip`: Move o clipe temporalmente para `at`.
  - `transcribe`: Executa Whisper local sobre o clipe e gera legendas sincronizadas.
  - `normalize_volume`: Analisa e aplica ganho LUFS alvo.
  - `undo` / `redo`: Desfaz e refaz comandos na `QUndoStack`.
  - `save_project`: Grava o arquivo de projeto no disco.
  - `export_video`: Inicia a renderização para MP4.

### 4.2 Inteligência Artificial Local Existente
- **VAD Neural:** `drift::SileroVad::instance().probabilities(mono_pcm)` processa áudio 16 kHz e identifica com precisão os timestamps de fala humana.
- **Transcrição Fonética:** `drift::WhisperTranscriber::instance().transcribe(pcm, progressCallback, "pt", maxWordsPerCue)` decodifica a fala em português e gera `QList<SubtitleCue>` com texto e limites de tempo em microssegundos.
- **Redução de Ruído:** `drift::DeepFilterDenoiser` (DeepFilterNet3) para supressão de ruídos de fundo em trilhas de voz.
- **Visão Computacional:** `drift::RvmMatter` (Robust Video Matting para remoção de fundo de pessoas sem tela verde) e `drift::Sam2Segmenter` (Segment Anything 2 para máscaras de objetos).

---

## 5. PLANO DE IMPLEMENTAÇÃO DO CLIP CUT 2.0 (FASES 2 A 6)

### Fase 2: Editor Base (Desktop C++/Qt 6)
- Configurar projeto com as 5 áreas na interface QML desktop:
  1. **Monitor de Prévia:** `PreviewPanel.qml` conectado ao `FrameCompositor`.
  2. **Timeline Multi-trilha:** `TimelinePanel.qml` com vídeo, áudio e legendas sincronizadas.
  3. **Biblioteca de Mídia:** `AssetsPanel.qml` para importação e inspeção de metadados.
  4. **Painel de Propriedades:** `PropertiesPanel.qml` para edição manual de volume, fade, posição e efeitos.
  5. **Painel IA:** `AiAssistantPanel.qml` integrado para instruções, plano e feedback em tempo real.
- Assegurar seleção por ID, preservação de trilhas não selecionadas, Undo/Redo e salvamento/autosave de projeto.

### Fase 3: IA Local (Silêncio, Cortes, Transcrição pt-BR e Áudio)
- Expor no backend C++ um serviço unificado `SmartCutEngine` que orquestra:
  - Análise de áudio para detecção de silêncio (configurável: limiar em dB, duração mínima e margem de guarda/padding).
  - Execução dos cortes não-destrutivos via `CutOps::packedSegments`, preservando a mídia original e criando um ponto no `QUndoStack`.
  - Transcrição pt-BR automática via `WhisperTranscriber` local, gerando trilha de legendas editável na timeline.
  - Ajuste de volume e normalização de sonoridade (LUFS) com crossfades suaves para evitar "clicks".

### Fase 4: Integração com Gemini (Planejador Inteligente)
- Criar a classe `GeminiClient` em C++ com `QNetworkAccessManager`:
  - Envia apenas contexto essencial do projeto (duração, lista de clipes, transcrição de fala com timestamps, metadados) sem enviar vídeos brutos pesados à nuvem.
  - Gemini gera um plano de edição em linguagem natural e um array de ações estruturadas em JSON estrito.
  - Validação rigorosa do JSON pelo `McpValidate` antes da execução.
  - Execução atômica no motor nativo via `McpDispatcher`, com solicitação de confirmação para ações destrutivas.

### Fase 5: Validação do Fluxo Completo Híbrido
- Testar o fluxo ponta a ponta:
  `Novo Projeto` → `Importar Mídia` → `Análise IA Local / Gemini` → `Edição Automática Aplicada` → `Revisão e Ajustes Manuais do Usuário na Timeline` → `Salvar / Reabrir Projeto` → `Exportar MP4 (16:9, 9:16 e 1:1)`.

### Fase 6: Preparação para Compilação e Distribuição Windows
- Scripts de automação CMake para compilação Release no Windows 10/11 x64.
- Empacotamento de DLLs com `windeployqt`, inclusão das bibliotecas do FFmpeg e dependências do vcpkg.
- Criação de instalador Inno Setup (`installer/windows/drift.iss`) e versão portátil zip.

---

## 6. CHECKLIST DE COMPILAÇÃO NATIVA NO WINDOWS 10/11 x64

Para compilar o binário nativo do Clip Cut 2.0 no Windows sem erros:

### 1. Pré-requisitos de Sistema
- [ ] Windows 10 ou 11 (64-bit).
- [ ] **Visual Studio 2022** (v17.x) com a carga de trabalho *"Desenvolvimento para Desktop com C++"* instalada (incluindo MSVC v143, Windows 10/11 SDK e CMake).
- [ ] **CMake 3.25+** no PATH.
- [ ] **Git for Windows** instalado.
- [ ] **Python 3.10+** no PATH (para scripts auxiliares).

### 2. Instalação do Qt 6
- [ ] Qt versão **6.10.3** (kit `win64_msvc2022_64`).
- [ ] Módulos obrigatórios:
  - `qtbase` (Core, Gui, Widgets, OpenGL, Network, Concurrent, Test).
  - `qtdeclarative` (Quick, QuickControls2, QuickEffects).
  - `qtmultimedia` (Backend multimídia e áudio).
  - `qtimageformats` (Decodificadores WebP e TIFF).
  - `qtsvg` (Renderização de ícones SVG).
- [ ] Definir a variável de ambiente:  
  `$env:QT_ROOT_DIR = "C:\Qt\6.10.3\msvc2022_64"`

### 3. Dependências via vcpkg
- [ ] Clonar e inicializar o vcpkg:
  ```powershell
  git clone --depth 1 https://github.com/microsoft/vcpkg.git C:\vcpkg
  C:\vcpkg\bootstrap-vcpkg.bat
  ```
- [ ] Instalar dependências dinâmicas (`x64-windows`):
  ```powershell
  C:\vcpkg\vcpkg.exe install zstd openssl soundtouch --triplet x64-windows
  ```
- [ ] Instalar Skia e zlib estático-md (`x64-windows-static-md`):
  ```powershell
  C:\vcpkg\vcpkg.exe install zlib "skia[gl,harfbuzz,icu,freetype,png]" --triplet x64-windows-static-md
  ```

### 4. FFmpeg Pinned
- [ ] Baixar o pacote de desenvolvimento FFmpeg 7.1 GPL compartilhado (`ffmpeg-n7.1-win64-gpl-shared.zip`):
  ```powershell
  Invoke-WebRequest -Uri "https://github.com/CutWire-Studios/FFmpeg-Builds/releases/download/n7.1/ffmpeg-n7.1-win64-gpl-shared.zip" -OutFile ffmpeg.zip
  Expand-Archive -Path ffmpeg.zip -DestinationPath .deps\ffmpeg_extracted -Force
  Copy-Item -Path ".deps\ffmpeg_extracted\*\*" -Destination .deps\ffmpeg -Recurse -Force
  ```

### 5. Configuração e Geração com CMake
- [ ] Executar no PowerShell no diretório raiz do projeto:
  ```powershell
  cmake -B build -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TOOLCHAIN_FILE="C:\vcpkg\scripts\buildsystems\vcpkg.cmake" `
    -DCMAKE_PREFIX_PATH="$env:QT_ROOT_DIR;${PWD}\.deps\ffmpeg;C:\vcpkg\installed\x64-windows-static-md;C:\vcpkg\installed\x64-windows" `
    -DZLIB_INCLUDE_DIR="C:\vcpkg\installed\x64-windows-static-md\include" `
    -DZLIB_LIBRARY="C:\vcpkg\installed\x64-windows-static-md\lib\zs.lib" `
    -DDRIFT_BUNDLE_ONNXRUNTIME=OFF `
    -DDRIFT_BUNDLE_CORE_PACKAGES_ONLY=ON `
    -DDRIFT_WITH_SKIA=ON `
    -DDRIFT_DISTRIBUTION=windows
  ```

### 6. Compilação
- [ ] Compilar a solução em modo Release:
  ```powershell
  cmake --build build --config Release --parallel 4
  ```

### 7. Distribuição e Deploy de DLLs (windeployqt)
- [ ] Criar diretório de distribuição `dist\bin`:
  ```powershell
  New-Item -ItemType Directory -Force -Path dist\bin
  Copy-Item build\Release\drift.exe dist\bin\
  Copy-Item .deps\ffmpeg\bin\*.dll dist\bin\
  Copy-Item C:\vcpkg\installed\x64-windows\bin\*.dll dist\bin\
  & "$env:QT_ROOT_DIR\bin\windeployqt.exe" --release --no-translations --no-opengl-sw --qmldir src/qml dist\bin\drift.exe
  ```
- [ ] Copiar modelos ONNX (`silero_vad.onnx`, `models/whisper-small/`) e a biblioteca `onnxruntime.dll` para o diretório de execução.
- [ ] Gerar instalador final via Inno Setup compilando `installer/windows/drift.iss`.
