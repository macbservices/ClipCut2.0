# CLIP CUT 2.0 — PROBLEMAS CONHECIDOS, LIMITAÇÕES E SOLUÇÕES

Este documento reúne as limitações técnicas, comportamentos esperados e soluções recomendadas para problemas conhecidos no **Clip Cut 2.0** no ambiente **Windows 10/11 x64**.

---

## 1. AMBIENTE GRÁFICO E TELAS DE ALTA RESOLUÇÃO (HIGH-DPI)

### Sintoma
Em computadores com múltiplos monitores e escalas de DPI diferentes (por exemplo, um notebook com tela 4K a 150% de zoom conectado a um monitor secundário 1080p a 100%), alguns elementos de texto ou ícones da interface QML podem sofrer leve desfoque ao arrastar a janela entre as telas.

### Solução / Configuração
O Clip Cut 2.0 inicializa o Qt com escalonamento vetorial por monitor (`Qt::AA_EnableHighDpiScaling`). Caso perceba artefatos gráficos, crie a variável de ambiente no Windows:
```powershell
[System.Environment]::SetEnvironmentVariable('QT_ENABLE_HIGHDPI_SCALING', '1', 'User')
[System.Environment]::SetEnvironmentVariable('QT_SCALE_FACTOR_ROUNDING_POLICY', 'PassThrough', 'User')
```

---

## 2. USO DE MEMÓRIA POR MODELOS NEURAIS DE TRANSCRIÇÃO (WHISPER)

### Sintoma
Sistemas com 8 GB de memória RAM podem apresentar lentidão temporária caso seja selecionado um modelo Whisper de maior porte (como o modelo `medium` ou `large`).

### Recomendação
* Para sistemas com 8 GB de RAM: Utilize o modelo **`whisper-tiny`** ou **`whisper-base`**, que consomem menos de 1 GB de RAM e executam em menos de 10 segundos para vídeos curtos.
* O motor de detecção de silêncio acústico (`SilenceDetectionService` / `silencedetect`) consome menos de 50 MB de RAM e é recomendado como primeira etapa antes de transcrever.

---

## 3. AVISO DO WINDOWS DEFENDER / SMARTSCREEN NA PRIMEIRA EXECUÇÃO

### Sintoma
Ao executar o instalador `ClipCut-2.0-Setup-x64.exe` ou o executável `ClipCut2.exe` compilado localmente, o Windows SmartScreen pode exibir uma tela azul informando:  
*"O Windows protegeu o seu computador - O Microsoft Defender SmartScreen impediu a inicialização de um aplicativo não reconhecido"*.

### Causa
Binários gerados em ambiente de desenvolvimento local não possuem certificado digital comercial de assinatura de código (EV Code Signing Certificate emitido por autoridade certificadora como DigiCert ou Sectigo).

### Solução
Clique em **"Mais informações"** e em seguida no botão **"Executar assim mesmo"**. O executável é 100% seguro, de código aberto e livre de malwares ou rastreadores.

---

## 4. RUÍDOS ACÚSTICOS (CLICKS) EM CORTES RÁPIDOS NA TIMELINE

### Causa e Mitigação
Quando um clipe de áudio é cortado abruptamente em um ponto de pico da onda sonora (diferente de zero), transições bruscas de voltagem podem gerar pequenos estalidos ("clicks") audíveis.

### Solução Implementada no Motor
O Clip Cut 2.0 utiliza a função nativa `drift::packedSegments` com micro-rampas de atenuação (*de-click ramps* de 5 milissegundos) nas bordas de entrada e saída de cada segmento, eliminando completamente estalidos acústicos nas emendas de corte.

---

## 5. CODECS PROPRIETÁRIOS E VÍDEOS DE CÂMERAS PROFISSIONAIS

### Limitação
O Clip Cut 2.0 decodifica e exporta nativamente todos os formatos comuns suportados pelo FFmpeg 7.1 (H.264, HEVC/H.265, VP9, AV1, ProRes padrão, DNxHD, AAC, MP3, WAV). Contudo, formatos RAW altamente proprietários (como REDCODE R3D, ARRI RAW ou Blackmagic BRAW) necessitam de conversão prévia para ProRes ou DNxHR antes da ingestão na timeline.

---

## 6. TIMEOUT OU LIMITE DE REQUISIÇÕES NA API DO GEMINI

### Sintoma
Mensagem *"Limite de requisições por minuto atingido (HTTP 429)"* ou *"Tempo limite de resposta esgotado"*.

### Solução
1. O editor foi arquitetado para nunca travar: ele captura o código HTTP 429 e ativa o motor local gratuito da Fase 3 automaticamente.
2. Aguarde 60 segundos para renovação da janela de cota gratuita da API oficial ou utilize as ferramentas manuais do painel de propriedades.
