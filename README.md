# CLIP CUT 2.0 — Editor Desktop Nativo Windows 10/11 x64

**Clip Cut 2.0** é um editor de vídeo desktop nativo de alta performance para **Windows 10/11 x64**, construído em **C++20**, **Qt 6.10**, **QML**, **FFmpeg 7.1** e **Skia**, baseado na arquitetura aberta do **CutWire-Studios/Drift**.

O software implementa um modelo de **edição híbrida**:
1. **Inteligência Artificial (Gemini + Modelos Locais):** Planeja, fatia silêncios, gera legendas em português (pt-BR), normaliza volume (-14 LUFS) e sugere cortes.
2. **Controle Humano Soberano:** O usuário tem controle total sobre a timeline nativa com Undo/Redo atômico, monitor de vídeo em tempo real e exportação multi-aspecto (16:9, 9:16 e 1:1).

---

## 🚀 Principais Recursos

- **Biblioteca de Mídia (`MediaPool`):** Ingestão de vídeos (MP4, MOV), áudios (WAV, MP3) e imagens (PNG com transparência, JPEG).
- **Monitor de Prévia em Tempo Real:** Reprodução assíncrona, scrubbing temporal e pré-visualização contínua sem travamentos de interface.
- **Timeline Não Destrutiva:** Trilhas de vídeo, áudio e legendas vinculadas; corte, divisão, deslizamento e ajuste de volume preservando os arquivos originais intactos.
- **Automação Audiovisual Local (100% Offline):**
  - **Detecção de Silêncios:** Análise acústica via libavfilter (`silencedetect`) e Silero VAD com limiar em dB e padding configuráveis.
  - **Transcrição de Fala pt-BR:** Geração de marcas temporais e legendas automáticas editáveis com exportação para padrão SubRip (`.srt`).
  - **Normalização de Volume:** Conformidade com a norma de transmissão internacional ITU-R BS.1770 (-14.0 LUFS).
  - **Transições:** Aplicação de transições *crossfade* entre clipes adjacentes.
- **Integração Inteligente com Google Gemini:**
  - Conversa em linguagem natural através do painel nativo QML.
  - Emissão de planos estruturados em contrato JSON versionado (**Schema 2.0**).
  - **Executor Nativo Soberano (`EditCommandExecutor`):** Validação matemática estrita de limites de tempo e índices antes de qualquer alteração na timeline.
  - **Segurança e Privacidade:** Nenhum vídeo ou áudio bruto é enviado para a nuvem. Chaves de API mascaradas e nunca gravadas em código ou logs.
  - **Resiliência a Cotas (HTTP 429):** Transição automática e transparente para os motores locais em caso de falta de cota ou rede offline.
- **Exportação Multi-Aspecto via FFmpeg:**
  - **Horizontal 16:9 (1920x1080):** Widescreen para YouTube e monitores.
  - **Vertical 9:16 (1080x1920):** Otimizado para TikTok, Reels e Shorts.
  - **Quadrado 1:1 (1080x1080):** Otimizado para feeds de redes sociais.

---

## 📂 Estrutura do Repositório

```text
├── clipcut-desktop/              # Código-fonte desktop nativo C++/Qt/CMake
│   ├── src/
│   │   ├── app/
│   │   │   ├── ai/               # GeminiProvider, EditCommandExecutor, AiBridge
│   │   │   ├── editor/           # AppController, Session, Timeline Models
│   │   │   ├── services/         # SilenceDetectionService, AutoEditService
│   │   │   └── main.cpp          # Ponto de entrada nativo C++
│   │   └── qml/                  # Interface gráfica nativa QML
│   ├── scripts/
│   │   ├── build-windows-x64.ps1 # Script automatizado de compilação Windows
│   │   ├── validate-phase4.py    # Validador de testes da Fase 4
│   │   └── validate-phase5.py    # Suíte de validação de ponta a ponta (Fase 5)
│   ├── installer/
│   │   └── windows/
│   │       └── ClipCut2.iss      # Script do instalador oficial Inno Setup
│   └── CMakeLists.txt            # Configuração do CMake 3.21+
├── dist/                         # Artefatos gerados
│   └── clipcut-desktop-src-v2.0.0.zip # Pacote ZIP do código-fonte completo
└── docs/                         # Documentação técnica de arquitetura e distribuição
    ├── INSTALACAO_WINDOWS.md     # Guia de instalação para usuários finais
    ├── COMPILACAO_WINDOWS.md     # Guia de compilação MSVC 2022 / Qt 6.10
    ├── CONFIGURACAO_GEMINI.md    # Guia seguro de chaves de API do Gemini
    ├── LICENCAS.md               # Auditoria de licenças (GPLv3, LGPL, MIT)
    ├── TESTES_FINAIS.md          # Matriz de testes finais e classificação
    └── PROBLEMAS_CONHECIDOS.md   # Limitações técnicas e soluções
```

---

## 🛠️ Como Compilar no Windows 10/11 x64

Requisitos:
- **Windows 10 ou 11 x64**
- **Visual Studio 2022 (MSVC v143)** com C++ Desktop
- **Qt 6.10.x** (kit `msvc2022_64`)
- **CMake 3.21+** e **Git**
- **Inno Setup 6** (opcional, para gerar o instalador `.exe`)

Abra o PowerShell como Usuário e execute:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\clipcut-desktop\scripts\build-windows-x64.ps1 -Config Release
```

O script irá:
1. Baixar e configurar dependências do vcpkg e FFmpeg n7.1.
2. Compilar a solução em modo Release gerando `ClipCut2.exe`.
3. Executar o `windeployqt` para coletar as bibliotecas do Qt e plugins.
4. Gerar o pacote portátil `ClipCut-2.0-Windows-x64-Portable.zip` e o instalador oficial `ClipCut-2.0-Setup-x64.exe`.

Para detalhes completos de compilação, consulte [docs/COMPILACAO_WINDOWS.md](docs/COMPILACAO_WINDOWS.md).

---

## 📄 Licenciamento

O Clip Cut 2.0 é um software livre distribuído sob a licença **GNU General Public License v3 (GPLv3)**.  
Consulte o arquivo [LICENSE](clipcut-desktop/LICENSE) e o documento [docs/LICENCAS.md](docs/LICENCAS.md) para detalhes de conformidade com bibliotecas de terceiros (Qt 6, FFmpeg 7.1, Skia e modelos ONNX).
