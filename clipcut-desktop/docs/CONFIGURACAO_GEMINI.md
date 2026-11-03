# CLIP CUT 2.0 — GUIA DE INTEGRAÇÃO E CONFIGURAÇÃO DO GOOGLE GEMINI

O **Clip Cut 2.0** utiliza a API oficial do **Google Gemini** para fornecer assistência inteligente na interpretação de instruções em linguagem natural, planejamento de operações audiovisuais e sugestões de corte.

Este guia orienta sobre como configurar as credenciais de forma segura e compreender os mecanismos de proteção de dados e fallback local.

---

## 1. PRINCÍPIOS DE SEGURANÇA E PRIVACIDADE

1. **Zero Envio de Mídia Bruta para a Nuvem:** O Clip Cut 2.0 **nunca faz upload de arquivos de vídeo, áudio ou fotos para servidores externos**. O Gemini recebe exclusivamente metadados textuais compactos (duração da timeline, quantidade de clipes, intervalos de silêncio acústico detectados localmente e texto das legendas).
2. **Separação Rígida entre Cognição e Execução:** O Gemini atua apenas como um planejador. Ele emite um plano estrito no contrato **Schema 2.0**. Todas as operações passam pelo **`EditCommandExecutor`** nativo antes de tocar na timeline.
3. **Sem Credenciais em Arquivos de Código:** Chaves de API nunca são embutidas no código-fonte, nunca são commitadas no Git e nunca aparecem em arquivos de log.
4. **Sem Cobranças Automáticas:** Assinaturas pessoais do Gemini (como Gemini Advanced) não fornecem chaves de API automaticamente. O usuário utiliza a cota gratuita oficial da Google AI Studio ou chave própria, sem risco de cobranças não autorizadas.

---

## 2. OBTENÇÃO DA CHAVE DE API

1. Acesse o portal oficial [Google AI Studio](https://aistudio.google.com/).
2. Faça login com sua Conta Google.
3. Clique em **Get API Key** (Obter Chave de API).
4. Crie uma chave em um projeto novo ou existente.
5. Copie a chave gerada (iniciada geralmente por `AIzaSy...`).

---

## 3. MÉTODOS DE CONFIGURAÇÃO NO CLIP CUT 2.0

### Método A: Variável de Ambiente do Windows (Recomendado)
Este método é o mais seguro e conveniente, pois disponibiliza a chave sem necessidade de digitá-la a cada sessão:

1. Abra o **PowerShell** no Windows e execute:
   ```powershell
   [System.Environment]::SetEnvironmentVariable('GEMINI_API_KEY', 'sua_chave_aqui', 'User')
   ```
2. Ao iniciar o `ClipCut2.exe`, o módulo `GeminiProvider` detectará automaticamente a variável `GEMINI_API_KEY`.

### Método B: Interface Gráfica do Editor (Em Memória)
Se preferir não salvar variáveis no sistema operacional:

1. Abra o Clip Cut 2.0.
2. No **Painel de IA** (painel à direita da interface), localize o cabeçalho e clique no botão de engrenagem **⚙ (Configurações)**.
3. Será exibida uma gaveta de configuração.
4. Cole sua chave de API no campo de texto mascarado (`••••••••`).
5. Clique em **Salvar**. A chave ficará ativa apenas na memória durante a execução do aplicativo.
6. Você pode clicar em **Remover** a qualquer momento para limpar a chave da memória.

---

## 4. MODELOS SUPORTADOS

Por padrão, o Clip Cut 2.0 conecta-se ao endpoint oficial REST:
- **Modelo Padrão:** `gemini-2.5-flash` (alta velocidade de resposta e excelente compreensão de roteiros audiovisuais).
- **Modelo Alternativo:** `gemini-1.5-flash` (compatibilidade legada).

O endpoint oficial utilizado é:
```text
https://generativelanguage.googleapis.com/v1beta/models/{modelName}:generateContent?key={apiKey}
```

---

## 5. RESILIÊNCIA E FALLBACK LOCAL (OFFLINE & COTA ESGOTADA)

O Clip Cut 2.0 foi desenvolvido para ser totalmente resiliente:

* **Erro HTTP 429 (Cota Gratuita Esgotada):** Caso você atinja o limite de requisições por minuto da cota gratuita da API, o `GeminiProvider` captura o status HTTP 429, emite o sinal `quotaExceeded()` e notifica amigavelmente o usuário. O editor ativa transparentemente o motor de automação local da Fase 3 sem travar o aplicativo.
* **Sem Conexão com a Internet:** Se a rede falhar ou estiver desconectada, o aplicativo emite um aviso no histórico do chat e mantém todas as ferramentas de IA local (corte de silêncios via FFmpeg, legendas automáticas via Whisper e normalização de volume) 100% disponíveis.
* **Cancelamento em Voo:** Qualquer solicitação ao Gemini pode ser cancelada instantaneamente pelo botão **Cancelar**, disparando `abort()` na requisição de rede e liberando o editor.
