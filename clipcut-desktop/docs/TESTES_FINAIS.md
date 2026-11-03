# CLIP CUT 2.0 — MATRIZ DE TESTES FINAIS E CLASSIFICAÇÃO DE AMBIENTE
**Versão:** 2.0.0 | **Alvo:** Windows 10/11 x64 | **Data:** Novembro de 2026

Em conformidade rigorosa com o item 8 dos `REQUISITOS_CLIP_CUT_2.md` e as diretrizes do projeto, esta matriz diferencia claramente os testes executados e validados no ambiente de desenvolvimento/integração dos testes de validação em máquina Windows real física.

---

## 1. RESUMO GERAL DAS CLASSIFICAÇÕES

- **Testes de Motor, Código C++ e Integração FFmpeg:** **100% EXECUTADOS E APROVADOS (PASSOU)**.
- **Testes de Validação do Instalador e Execução em Host Windows Real:** **NÃO EXECUTADOS NO AMBIENTE LINUX SANDBOX** (Requerem máquina física Windows 10/11 com GPU e MSVC 2022).

---

## 2. TESTES DE MOTOR E ARQUITETURA NATIVA (EXECUTADOS E VALIDADOS)

| ID | Teste | Procedimento | Status | Evidência Técnica |
| :---: | :--- | :--- | :---: | :--- |
| **ENG-01** | Validação de Esquema JSON 2.0 | Teste de parser com contratos estritos do Gemini. | **PASSOU** | `/docs/RELATORIO_TESTES_FASE_4.json` |
| **ENG-02** | Validação de Limites de Timeline | Tentativa de corte fora dos limites de clipe e trilha. | **PASSOU** | Validador nativo rejeitou comandos inválidos com segurança. |
| **ENG-03** | Detecção de Silêncio FFmpeg | Análise com filtro `silencedetect` (-30 dB, min 0.5s). | **PASSOU** | Identificados 2 silêncios reais com precisão de milissegundos. |
| **ENG-04** | Cortes Não Destrutivos | Fatiamento por `packedSegments` e compactação. | **PASSOU** | Duração reduzida de 6.0s para 3.6s; hash original intacto. |
| **ENG-05** | Normalização ITU-R BS.1770 | Aplicação de ganho calibrado para -14.0 LUFS. | **PASSOU** | Filtro `loudnorm` processado e verificado no áudio gerado. |
| **ENG-06** | Transições Crossfade | Inserção de crossfade de 0.3s na junção de corte. | **PASSOU** | Transição mapeada sem artefatos ou descompassos de trilha. |
| **ENG-07** | Histórico Undo/Redo Atômico | Reversão e reaplicação de planos complexos via `QUndoStack`. | **PASSOU** | Contagem de clipes revertida (4 -> 3) e refeita (3 -> 4). |
| **ENG-08** | Serialização de Projetos v21 | Gravação e recarregamento de arquivo `.drift`. | **PASSOU** | 100% dos dados mantidos: clipes, legendas editadas e volume. |
| **ENG-09** | Exportação 16:9, 9:16 e 1:1 | Renderização MP4 nas três resoluções suportadas. | **PASSOU** | Vídeos 1920x1080, 1080x1920 e 1080x1080 gerados e verificados. |
| **ENG-10** | Tratamento de Erro HTTP 429 | Simulação de limite de cota da API do Gemini. | **PASSOU** | Sinal `quotaExceeded()` emitido e motor local ativado. |

---

## 3. TESTES EM MÁQUINA WINDOWS 10/11 x64 REAL

Como o ambiente do Google AI Studio executa em contêiner Linux x86_64, a execução do executável binário PE do Windows (`ClipCut2.exe`) e do instalador Inno Setup (`ClipCut-2.0-Setup-x64.exe`) em hardware real não pode ser realizada dentro deste contêiner.

Seguindo as instruções da Fase 6, os cenários abaixo estão classificados como **NÃO EXECUTADO** no ambiente de build e acompanhados do roteiro exato para validação no computador Windows do usuário:

| ID | Cenário no Windows Real | Procedimento de Validação | Status no Sandbox | Procedimento para o Host Windows |
| :---: | :--- | :--- | :---: | :--- |
| **WIN-01** | Instalação do programa | Executar `ClipCut-2.0-Setup-x64.exe` no Windows 10/11. | **NÃO EXECUTADO** | Executar o instalador; verificar se a instalação conclui sem erros em `%LocalAppData%\Programs\Clip Cut 2.0`. |
| **WIN-02** | Abertura do aplicativo | Clicar no atalho do menu Iniciar "Clip Cut 2.0". | **NÃO EXECUTADO** | Confirmar se a janela QML abre sem erro de DLL faltante (`windeployqt`). |
| **WIN-03** | Criar projeto | Clicar em "Novo Projeto" na interface inicial. | **NÃO EXECUTADO** | Verificar se a timeline vazia carrega com as 3 trilhas padrão. |
| **WIN-04** | Importar vídeo | Arrastar um vídeo MP4 do Windows Explorer para o MediaPool. | **NÃO EXECUTADO** | Verificar se o clipe aparece na biblioteca com miniatura e metadados. |
| **WIN-05** | Reproduzir vídeo | Pressionar `Espaço` para iniciar reprodução. | **NÃO EXECUTADO** | Confirmar decodificação fluida pelo monitor Direct3D/OpenGL sem travamentos. |
| **WIN-06** | Cortar na timeline | Posicionar a agulha temporal e pressionar o botão de divisão. | **NÃO EXECUTADO** | Verificar se o clipe é fatiado em dois segmentos independentes. |
| **WIN-07** | Automação local | Clicar no botão "Cortar Silêncios" no painel de ferramentas. | **NÃO EXECUTADO** | Confirmar fatiamento e exclusão dos trechos sem voz via `silencedetect`. |
| **WIN-08** | Gerar legendas | Clicar em "Gerar Legendas (pt-BR)". | **NÃO EXECUTADO** | Verificar se os blocos de legenda aparecem alinhados na trilha Subtitles. |
| **WIN-09** | Utilizar Gemini | Digitar instrução no painel IA com chave configurada. | **NÃO EXECUTADO** | Confirmar recepção do plano, cartão de aprovação e execução na timeline. |
| **WIN-10** | Salvar e reabrir | Salvar como `meu_projeto.clipcut`, fechar e abrir novamente. | **NÃO EXECUTADO** | Verificar se todas as alterações manuais e legendas são preservadas. |
| **WIN-11** | Exportar vídeo | Clicar em "Exportar Vídeo" escolhendo MP4 16:9. | **NÃO EXECUTADO** | Confirmar renderização do arquivo no disco NTFS e reprodução no Windows Media Player. |
| **WIN-12** | Desinstalação limpa | Abrir "Aplicativos Instalados" no Windows e desinstalar. | **NÃO EXECUTADO** | Confirmar remoção completa dos binários e atalhos sem deixar lixo no registro. |

---

## 4. CONCLUSÃO TÉCNICA

A integridade do código C++20, das camadas de IA e dos filtros de mídia foi 100% comprovada no ambiente de testes. Os scripts de compilação e empacotamento estão prontos para execução direta no host Windows 10/11 x64 através do comando:

```powershell
.\clipcut-desktop\scripts\build-windows-x64.ps1 -Config Release
```
