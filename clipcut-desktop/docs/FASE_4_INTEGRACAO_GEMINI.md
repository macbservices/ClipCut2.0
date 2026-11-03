# CLIP CUT 2.0 — IMPLEMENTAÇÃO DA FASE 4: INTEGRAÇÃO DO GEMINI
**Data de Conclusão:** Novembro de 2026  
**Alvo:** Windows 10/11 x64 (Nativo C++20 / Qt 6.10 / QML / Google Gemini REST API)  
**Código Desktop:** `/clipcut-desktop/`  
**Relatório de Evidências dos Testes:** `/docs/RELATORIO_TESTES_FASE_4.json`

---

## 1. RESUMO EXECUTIVO E ARQUITETURA DA FASE 4

A Fase 4 integra o **Google Gemini** como oráculo de inteligência e assistente de planejamento audiovisual do Clip Cut 2.0. O Gemini atua exclusivamente na camada cognitiva (compreensão de linguagem natural, raciocínio contextual e estruturação de planos de edição), enquanto o motor nativo em C++/Qt 6 e os serviços multimídia do FFmpeg/ONNX são os executores reais e soberanos da aplicação.

### Princípios Arquiteturais Inegociáveis:
1. **Separação Rígida entre Pensamento e Execução:** O Gemini **não executa ações diretamente**, não tem acesso a shell/terminal do sistema operacional e não manipula arquivos brutos no disco. Ele emite estritamente um plano em formato JSON versionado.
2. **Executor Nativo Soberano (`EditCommandExecutor`):** Toda e qualquer ação contida no plano é validada matematicamente contra o estado real do projeto (`Project`, `TrackList`, `Clip`) antes de ser aplicada.
3. **Privacidade e Coleta Mínima de Contexto:** Vídeos e áudios pesados **nunca são enviados para a nuvem**. O editor extrai apenas metadados compactos em texto (duração, contagem de clipes, silêncios identificados e legendas transcritas localmente).
4. **Sem Cobranças Automáticas e Fallback Gracioso:** O sistema nunca ativa planos pagos automaticamente. Caso a cota gratuita seja atingida (HTTP 429) ou a rede falhe, o editor ativa transparentemente o motor de automação local da Fase 3.
5. **Undo/Redo Atômico:** Todo o conjunto de alterações gerado por um plano do Gemini é empacotado em um único comando no `QUndoStack` (`ProjectSnapshotCommand`), permitindo desfazer o plano inteiro com um só `Ctrl+Z`.

---

## 2. ARQUIVOS IMPLEMENTADOS E MODIFICADOS

### 2.1 `src/app/ai/GeminiProvider.h` e `.cpp`
Camada isolada de comunicação de rede com a API oficial do Google Gemini:
* **Protocolo:** Requisições HTTPS REST assíncronas utilizando `QNetworkAccessManager`.
* **Endpoint:** `https://generativelanguage.googleapis.com/v1beta/models/{modelName}:generateContent?key={apiKey}`.
* **Modelo Padrão:** `gemini-2.5-flash` (ou `gemini-1.5-flash`), configurável pelo usuário.
* **Segurança de Credenciais:** A chave de API nunca é gravada em código-fonte nem exposta em logs. É lida da variável de ambiente `GEMINI_API_KEY` ou informada pelo usuário em campo mascarado na interface.
* **Tratamento Robusto de Rede e Erros:**
  - Status 200 OK: Extração e desserialização do JSON estrito retornado pelo modelo.
  - Status 429 (Too Many Requests): Emissão do sinal `quotaExceeded()` e notificação amigável instruindo o uso do motor local gratuito.
  - Status 400/401/403: Identificação de chave ausente, revogada ou inválida.
  - Timeout: Temporizador de 30 segundos com interrupção forçada via `QNetworkReply::abort()`.
  - Cancelamento em voo: Método `cancelRequest()` que aborta requisições pendentes.

### 2.2 `src/app/ai/EditCommandExecutor.h` e `.cpp`
Motor validador e executor nativo que processa os planos JSON do Gemini:
* **Validação de Esquema (Schema 2.0):** Exige `schema_version == "2.0"` e presença do array `actions`.
* **Validação de Limites na Timeline:**
  - Valida se o índice de trilha existe (`0 <= track < tracks.size()`).
  - Valida se o índice do clipe existe na trilha selecionada.
  - Para `split_clip`: valida rigorosamente se o ponto temporal `at` está dentro dos limites do clipe (`0 < at < timelineDuration`).
  - Rejeita qualquer ação fora da lista permitida (`split_clip`, `delete_clip`, `move_clip`, `remove_silence`, `normalize_volume`, `add_subtitles`, `add_transition`, `export_video`).
* **Execução Real:** Conecta-se diretamente aos despachantes nativos do `AppController`, `AutoEditService` e `McpDispatcher`.
* **Agrupamento Atômico:** Cria um snapshot de estado antes da execução (`Project before = m_project.detachedCopy()`) e registra no `QUndoStack`.

### 2.3 `src/app/ai/AiBridge.h` e `.cpp`
Ponte de orquestração que integra o `GeminiProvider` e o `EditCommandExecutor` ao editor desktop:
* Gerencia o histórico da conversa do projeto (`conversationHistory`), permitindo diálogos contínuos com bolhas de usuário, Gemini e sistema.
* Implementa o método `collectProjectContext()`, montando o payload seguro de metadados.
* Métodos invocáveis pelo QML: `sendMessage()`, `setApiKey()`, `clearConversation()`, `executePlan()`, `rejectPlan()`, `cancelCurrentOperation()`.

### 2.4 `src/qml/AiAssistantPanel.qml`
Interface visual nativa QML atualizada para o assistente Gemini:
* **Histórico de Chat:** `ListView` com balões de conversa estilizados, timestamps e diferenciação visual entre usuário, Gemini e mensagens de status do sistema.
* **Gaveta de Configuração de Chave:** Interface recolhível com campo mascarado para inserção da chave de API e alternador para forçar o modo local.
* **Indicador de Status Dinâmico:** Chip indicando "Gemini Online", "Motor Local" ou "Pensando…".
* **Cartão de Revisão do Plano do Gemini:** Exibe o resumo do plano em verde e requer aprovação explícita ("✓ Aprovar e Executar" vs "✕ Descartar").
* **Sugestões Rápidas:** Botões com prompts frequentes para agilizar o fluxo de edição.

---

## 3. CONTRATO JSON VERSIONADO (SCHEMA 2.0)

O Gemini opera sob um contrato de esquema JSON rigoroso, tipado e validado:

```json
{
  "schema_version": "2.0",
  "request_id": "7b8f2190-c11a-4d2b-b6d8-8123456789ab",
  "project_id": "proj-clipcut-001",
  "explanation": "Identifiquei 3 trechos de silêncio no áudio e uma faixa de volume baixo. Proponho cortar os silêncios, normalizar o volume para -14 LUFS e gerar legendas em português.",
  "requires_confirmation": true,
  "actions": [
    {
      "action": "remove_silence",
      "track": 0,
      "clip": 0,
      "parameters": {
        "thresholdDb": -30.0,
        "minDuration": 0.5,
        "padding": 0.1
      }
    },
    {
      "action": "normalize_volume",
      "track": 0,
      "clip": 0,
      "parameters": {
        "targetLufs": -14.0
      }
    },
    {
      "action": "add_subtitles",
      "track": 0,
      "clip": 0,
      "parameters": {
        "languageCode": "pt"
      }
    },
    {
      "action": "add_transition",
      "track": 0,
      "clip": 0,
      "parameters": {
        "kind": "crossfade",
        "durationSec": 0.5
      }
    }
  ]
}
```

---

## 4. MATRIZ DOS 10 TESTES OBRIGATÓRIOS (FASE 4)

A suíte automatizada `clipcut-desktop/scripts/validate-phase4.py` validou todos os 10 cenários obrigatórios:

| # | Cenário de Teste | Status | Evidência Técnica e Resultado Real |
| :---: | :--- | :---: | :--- |
| **1** | **Solicitação simples de corte** | **PASSOU** | Plano estruturado 2.0 com `split_clip` validado em `t=2.5s` no clipe 0; geradas duas partes contíguas com limites corretos. |
| **2** | **Solicitação com várias operações** | **PASSOU** | Plano multi-tarefa validado contendo 4 ações encadeadas (`remove_silence`, `normalize_volume`, `add_subtitles`, `add_transition`). |
| **3** | **Comando para clipe inexistente** | **PASSOU** | Ação direcionada ao clipe 99 rejeitada pelo validador com a mensagem: *"Clipe inexistente: índice 99 fora dos limites da trilha 0"*; a timeline permaneceu intocada. |
| **4** | **Intervalo temporal inválido** | **PASSOU** | Ação de corte em `15.0s` em um clipe de `5.0s` rejeitada pelo validador: *"Intervalo temporal inválido: corte em 15.0s fora da duração do clipe (5.0s)"*. |
| **5** | **Falha de conexão** | **PASSOU** | Simulação de indisponibilidade de rede tratada com mensagem de orientação e ativação automática do motor local sem travamentos. |
| **6** | **Limite de uso da API (HTTP 429)** | **PASSOU** | Código de erro HTTP 429 interceptado pelo `GeminiProvider`; emitido sinal `quotaExceeded()` e ativado o modo local gratuito. |
| **7** | **Cancelamento** | **PASSOU** | Método `cancelRequest()` abortou a requisição em voo e restaurou o estado `busy: false` imediatamente. |
| **8** | **Undo/Redo do plano Gemini** | **PASSOU** | Plano multi-etapa executado e revertido via `QUndoStack` (restaurando o clipe íntegro de 5.0s) e reaplicado via Redo. |
| **9** | **Atualização visual da timeline** | **PASSOU** | Métodos `pushProjectEdit` e `finishEdit` emitiram sinais para o `TimelineClipsModel`, atualizando o número de clipes e as durações visuais. |
| **10** | **Persistência das alterações** | **PASSOU** | Projeto salvo em arquivo `.drift` com histórico de operações do Gemini e reaberto mantendo 100% da integridade de dados e metadados. |

---

## 5. INSTRUÇÕES DE CONFIGURAÇÃO DA API GEMINI

Para utilizar o assistente Gemini no Clip Cut 2.0:

1. **Obtenção da Chave:** Obtenha uma chave gratuita no [Google AI Studio](https://aistudio.google.com/).
2. **Configuração via Variável de Ambiente (Recomendado):**
   ```powershell
   [System.Environment]::SetEnvironmentVariable('GEMINI_API_KEY', 'sua_chave_aqui', 'User')
   ```
3. **Configuração via Interface Gráfica:**
   No painel *Assistente Gemini & IA* do editor, clique no ícone **⚙** (engrenagem), cole a chave no campo mascarado e clique em **Salvar**. A chave permanecerá em memória durante a sessão.
4. **Operação Offline / Sem Chave:**
   Caso nenhuma chave seja inserida, o editor opera normalmente no modo local, executando todas as tarefas através do motor da Fase 3.
