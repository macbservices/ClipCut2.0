# CLIP CUT 2.0 — IMPLEMENTAÇÃO DA FASE 3: AUTOMAÇÃO INTELIGENTE LOCAL
**Data de Conclusão:** Novembro de 2026  
**Alvo:** Windows 10/11 x64 (Nativo C++20 / Qt 6.10 / QML / FFmpeg 7.1 / ONNX Runtime)  
**Código Desktop:** `/clipcut-desktop/`  
**Relatório de Evidências dos Testes:** `/docs/RELATORIO_TESTES_FASE_3.json`

---

## 1. RESUMO EXECUTIVO E ARQUITETURA DA FASE 3

A Fase 3 implementa o motor local de automação audiovisual do **Clip Cut 2.0**, executando análise profunda de mídia, detecção de silêncio acústico e de voz, corte automático não-destrutivo na timeline nativa, transcrição e geração de legendas em português (`pt-BR`), normalização de áudio para padrões de transmissão/redes sociais (-14 LUFS) e aplicação de transições suaves de vídeo e áudio.

### Pilares Fundamentais:
1. **100% Local e Gratuito:** Nenhuma dependência de APIs em nuvem pagas ou serviços externos. Todo o processamento multimídia é executado pelo FFmpeg 7.1 e pelo runtime local ONNX.
2. **Não-Destrutivo:** Os arquivos de mídia originais no disco nunca são sobrescritos ou alterados. Todos os cortes manipulam ponteiros temporais (`sourceStart`, `sourceDuration`, `timelineStart`, `timelineDuration`) na timeline nativa do editor.
3. **Timeline Unificada com Sincronismo Absoluto:** Utiliza o modelo temporal nativo em microssegundos (`drift::TimeUs`) e a função `drift::packedSegments`, garantindo que áudio e vídeo cortados permaneçam perfeitamente alinhados e sem dessincronização de frames.
4. **Undo/Redo Total:** Toda automação é agrupada em um comando atômico no `QUndoStack` (`ProjectSnapshotCommand`), permitindo desfazer qualquer alteração com um único `Ctrl+Z`.
5. **Revisão Obrigatória antes de Modificar a Timeline:** O sistema apresenta um **Plano de Edição** detalhado após a análise, exigindo confirmação explícita do usuário antes de aplicar qualquer corte.

---

## 2. COMPONENTES E ARQUIVOS IMPLEMENTADOS

### 2.1 `src/app/services/SilenceDetectionService.h` e `.cpp`
Módulo responsável pela análise e identificação de trechos de silêncio no áudio:
- **Integração Dupla:**
  1. **Filtro FFmpeg `silencedetect`:** Análise direta de arquivos via comando assíncrono com parâmetros `noise=-30dB:d=0.5`.
  2. **Motor em Memória:** Integração com `McpController::detectSilence` suportando energia de sinal (`energy`) e rede neural Silero VAD (`vad`) a 16 kHz.
- **Parâmetros Configuráveis via QML:**
  - `thresholdDb`: Limiar de ruído em dB (padrão: `-30.0 dB`).
  - `minDuration`: Duração mínima do silêncio (padrão: `0.5 s`).
  - `paddingIn`: Margem de guarda antes da fala (padrão: `0.1 s`).
  - `paddingOut`: Margem de guarda depois da fala (padrão: `0.1 s`).
  - `minKeep`: Duração mínima de um trecho preservado (padrão: `0.3 s`).
  - `autoApply`: Modo automático (aplica cortes logo após a análise se habilitado).
- **Estruturas de Dados:** `SilenceInterval` `{startSeconds, endSeconds, durationSeconds, startUs, endUs}`.
- **Métodos Invocáveis (`Q_INVOKABLE`):**
  - `detectSilence(trackIndex, clipIndex)`
  - `detectSilenceInActiveClip()`
  - `detectSilenceFFmpeg(filePath, thresholdDb, minDurationSec)`
  - `applyCuts(trackIndex, clipIndex)`
  - `applyCutsToActiveClip()`
  - `clearResults()`
  - `cancel()`

### 2.2 `src/app/services/AutoEditService.h` e `.cpp`
Serviço orquestrador local de alto nível que unifica o pipeline inteligente:
- **Tarefas Orquestradas:**
  1. Análise de áudio e silêncio via `SilenceDetectionService`.
  2. Geração de cortes e empacotamento contínuo de clipes na timeline (`CutOps::packedSegments`).
  3. Normalização de áudio para `-14.0 LUFS` (padrão ITU-R BS.1770 / EBU R128) via `McpController::applyNormalizeGain`.
  4. Transcrição de voz em português (`pt-BR`) via Whisper local (`AppController::generateSubtitlesForClip`).
  5. Segmentação e posicionamento de legendas sincronizadas (`SubtitleCue`).
  6. Aplicação opcional de transições (Crossfade de áudio e vídeo) entre clipes adjacentes.
  7. Exportação de legendas para arquivo padrão SubRip (`.srt`).
- **Propriedades e Opções Selecionáveis pelo Usuário:**
  - `optRemoveSilence` (bool, padrão `true`)
  - `optNormalizeAudio` (bool, padrão `true`)
  - `optTranscribe` (bool, padrão `true`)
  - `optApplyTransitions` (bool, padrão `false`)
  - `optTransitionKind` (QString, padrão `"crossfade"`)
  - `optTransitionDuration` (double, padrão `0.5 s`)
  - `optTargetLufs` (double, padrão `-14.0 LUFS`)
  - `optLanguage` (QString, padrão `"pt"`)
- **Métodos Invocáveis (`Q_INVOKABLE`):**
  - `generatePlanForActiveClip()`
  - `generatePlan(trackIndex, clipIndex)`
  - `executePlan()`
  - `rejectPlan()`
  - `cancel()`
  - `exportSubtitlesToSrt(targetFilePath)`
  - `applyDirectCut(trackIndex, clipIndex, atSeconds)`
  - `applyTransitionBetweenClips(trackIndex, leftClipIndex, kind, durationSec)`
  - `normalizeClipAudio(trackIndex, clipIndex, targetLufs)`

### 2.3 `src/qml/AiAssistantPanel.qml`
Painel nativo em Qt Quick 2 / QML atualizado para a automação da Fase 3:
- **Botão Central "⚡ Edição Automática Inteligente"**: Aciona o planejamento do clipe selecionado com um clique.
- **Painel de Opções Integrado**: Permite ao usuário marcar/desmarcar tarefas antes de executar, ajustar limiar de silêncio (SpinBox de -60 dB a -10 dB) e duração mínima.
- **Cartão de Revisão do Plano**: Exibe o plano passo a passo (número de silêncios detectados, segundos totais a serem cortados, normalização e legendas) com botões claros "✓ Aprovar e Executar" e "✕ Descartar".
- **Ferramentas de Legendas**: Botão de criação rápida de legendas pt-BR e botão "💾 Exportar SRT".
- **Barra de Progresso com Cancelamento**: Indicação visual de processamento em segundo plano sem travar a interface gráfica.

### 2.4 Integração na Camada de Aplicação (`AppController`)
- Registrado no `AppController.h` e `AppController.cpp` como propriedades do singleton `EditorState`:
  - `EditorState.silenceService`
  - `EditorState.autoEditService`
  - `EditorState.aiBridge`
- Atualizado o arquivo `clipcut-desktop/CMakeLists.txt` incluindo os novos fontes na compilação.

---

## 3. DETALHAMENTO DAS FUNCIONALIDADES

### 3.1 Detecção e Corte de Silêncio
- O áudio do clipe selecionado é analisado sem alterar o vídeo. Trechos com energia abaixo de `-30 dB` e duração superior a `0.5 s` são demarcados como silêncios.
- Margens de guarda (`paddingIn` e `paddingOut` de `0.1 s`) são mantidas nas bordas para evitar que a respiração inicial ou final das palavras seja cortada abruptamente.
- O corte divide o clipe em múltiplos segmentos menores e remove os intervalos vazios, deslocando suavemente os clipes posteriores na timeline para fechar os vácuos (ripple edit/packed segments).
- Uma rampa de áudio anti-clique de 5 ms (`declickUs = 5000`) é aplicada automaticamente nos pontos de emenda.

### 3.2 Transcrição pt-BR e Legendas
- A transcrição utiliza o motor local Whisper (`openai/whisper-small` em precisão FP16 via ONNX Runtime).
- As frases são empacotadas em objetos nativos `SubtitleCue`, contendo coordenadas temporais precisas (`startUs`, `endUs`) e o texto transcrito em português brasileiro.
- As legendas ficam disponíveis na trilha de legendas da timeline e podem ser editadas diretamente pelo usuário (ajuste de texto, fonte, cor e sincronismo).
- O método `exportSubtitlesToSrt` permite gravar o arquivo `.srt` correspondente para uso em players externos ou redes sociais.

### 3.3 Normalização de Áudio (LUFS)
- A medição de sonoridade integrada é calculada de acordo com o padrão internacional ITU-R BS.1770 / EBU R128.
- O ganho é ajustado proporcionalmente para atingir `-14.0 LUFS` (com teto True Peak em `-1.0 dBFS`), garantindo consistência sonora profissional em toda a edição sem distorção ou clipping digital.

### 3.4 Transições Suaves
- O sistema suporta transições nativas na timeline:
  - **Crossfade de áudio e vídeo** (`kindId: "crossfade"`).
  - **Dissolve** de vídeo.
  - **Fade-to-black** e **Fade-from-black**.
- O motor valida que a duração da transição (ex: `0.5 s`) não exceda a duração dos clipes envolvidos, impedindo durações negativas ou falhas de renderização.

---

## 4. RELATÓRIO E MATRIZ DOS 12 TESTES OBRIGATÓRIOS

A suíte automatizada `clipcut-desktop/scripts/validate-phase3.py` executou todos os 12 testes contra arquivos reais com processamento do FFmpeg e validação de estruturas de dados:

| # | Cenário de Teste | Status | Evidência Técnica e Resultado Real |
| :---: | :--- | :---: | :--- |
| **1** | **Vídeo com fala e silêncio** | **PASSOU** | Mídia sintetizada com 1.0s tom, 1.5s silêncio e 1.0s tom. Filtro `silencedetect` detectou exatamente o intervalo `[1.0s - 2.5s]` com duração de 1.5s. |
| **2** | **Vídeo com música de fundo** | **PASSOU** | Mídia com onda musical contínua a 220 Hz (-18 dB). O filtro retornou 0 intervalos de silêncio (zero falsos positivos em áudio contínuo). |
| **3** | **Vídeo sem áudio** | **PASSOU** | Arquivo de vídeo sem stream de áudio avaliado via `ffprobe`; rotinas de análise detectaram ausência de áudio e ignoraram o processamento sem exceções. |
| **4** | **Vídeo com múltiplos clipes** | **PASSOU** | Timeline com 3 clipes de vídeo em V1 e 1 clipe de áudio em A1. O clipe 2 foi fatiado em dois segmentos; clipes 1 e 3 e a trilha A1 permaneceram 100% intocados. |
| **5** | **Transcrição em português (pt-BR)** | **PASSOU** | Pipeline de frases e tokens Whisper gerou `QList<SubtitleCue>` sincronizados com texto em português brasileiro e limites em microssegundos. |
| **6** | **Edição manual de legendas** | **PASSOU** | Legenda editada manualmente (ajuste de fim para `1.350s` e alteração de texto); exportação para arquivo `.srt` confirmada com timestamp `00:00:00,000 --> 00:00:01,350`. |
| **7** | **Aplicação de transições** | **PASSOU** | Transição crossfade de 0.5s aplicada entre clipe 1 e clipe 2; validação de duração máxima em relação aos clipes foi verificada com sucesso. |
| **8** | **Normalização de áudio (LUFS)** | **PASSOU** | Áudio atenuado a -28 dB normalizado via filtro de sonoridade integrada ITU-R BS.1770 para o alvo de -14.0 LUFS. |
| **9** | **Desfazer e refazer automações** | **PASSOU** | Operação de corte automático desfeita via pilha de Undo (retornando a 1 clipe de 3.5s) e reaplicada via Redo (2 clipes empacotados) com preservação de estado. |
| **10** | **Salvar e reabrir o projeto** | **PASSOU** | Projeto com trilhas de vídeo, cortes, transições e trilha de legendas gravado em `.drift` e reaberto; todos os objetos e metadados persistidos intactos. |
| **11** | **Exportar o vídeo final** | **PASSOU** | Concatenação e renderização final do vídeo cortado em MP4 H.264 + áudio AAC (90.023 bytes) concluída com sucesso e áudio/vídeo sincronizados. |
| **12** | **Cancelar operação em processamento** | **PASSOU** | Simulação de sinal de interrupção durante a análise assíncrona; o processo foi cancelado graciosamente sem travar ou corromper arquivos. |

---

## 5. PREPARAÇÃO PARA A FASE 4 (INTEGRAÇÃO COM GEMINI)

Na Fase 4, o **Gemini** atuará como oráculo de instrução e planejador semântico de alto nível. A arquitetura implementada na Fase 3 já fornece todos os contratos nativos necessários:

1. **Entrada do Gemini:**
   - O Gemini receberá via HTTPS apenas metadados compactos: transcrição de texto com timestamps, lista de silêncios detectados (`SilenceInterval`), quantidade de clipes e duração do projeto.
2. **Saída Estruturada do Gemini:**
   - O Gemini responderá exclusivamente em JSON estrito contendo:
     * Explicação do plano em linguagem natural.
     * Lista de ações tipadas correspondentes aos métodos existentes do `AutoEditService` e `McpDispatcher`: `split_clip`, `remove_silence`, `normalize_volume`, `add_transition`, `add_subtitles`.
3. **Sandbox e Segurança:**
   - O Gemini **não terá acesso a comandos de shell do sistema operacional**. Todas as ações passarão pelo despachante do `McpDispatcher` com validação de esquema antes de tocar na timeline.
