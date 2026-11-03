# REQUISITOS — CLIP CUT 2.0
Versão: 1.0 | Destino: Windows 10/11 x64 | Estado: especificação, não implementação

## 1. Objetivo
Construir um editor desktop nativo de vídeo com edição híbrida: IA faz análise e primeira edição; usuário revisa e modifica tudo manualmente. Fluxo obrigatório: criar projeto → importar mídia → analisar → editar automaticamente → revisar na timeline → ajustar → salvar → exportar.

## 2. Base e arquitetura
Auditar https://github.com/CutWire-Studios/Drift antes de programar. Confirmar licença, commit, dependências, build Windows, Qt/QML, timeline, seleção, projetos, Undo/Redo, FFmpeg, IA local e MCP; não inventar APIs. Priorizar reutilização do Drift. Stack preferencial C++/Qt 6/QML/FFmpeg/CMake conforme arquitetura real. Não substituir por site React/HTML, nem criar Android. Examinar obrigações GPLv3 e licenças de modelos e codecs antes de distribuir.

## 3. IA e custos
Gemini para interpretar instruções, planejar edições e gerar comandos estruturados. Modelos locais gratuitos com licenças compatíveis para transcrição, legendas, análise de áudio, remoção de fundo e voz, conforme recursos efetivamente disponíveis no Drift e hardware. FFmpeg para análise, filtros, decodificação e exportação. Ollama opcional. Recursos ByteDance/CapCut apenas com acesso oficial, licença compatível e custo previamente autorizado; não usar endpoints privados nem extrair modelos proprietários. Assinaturas de apps não equivalem automaticamente a créditos de API. Não ativar cobrança. Respeitar cotas e permitir edição manual offline.

## 4. Interface
Cinco áreas: monitor de prévia; timeline com trilhas de vídeo/áudio/legenda, cortes e transições; biblioteca de mídia; painel de propriedades; painel IA com chat, plano, progresso, erros e cancelamento. Interface nativa QML responsiva, sem travar durante operações longas.

## 5. Recursos mínimos (MVP)
- Importar vídeos, áudios e imagens; reproduzir e mostrar metadados.
- Cortar, dividir, mover, aparar e excluir clipes manualmente.
- Identificar clipe por ID; não modificar trilhas não selecionadas indevidamente.
- Detectar silêncio via análise real do áudio (limiar dB, duração mínima e margens configuráveis); gerar cortes não destrutivos e revisáveis.
- Transcrever fala pt-BR com mecanismo local viável; legendas temporizadas editáveis, estilos básicos, exportação SRT quando aplicável.
- Ajustar volume, normalizar, fade e aplicar transições básicas efetivas.
- Gemini recebe apenas contexto necessário, produz plano e comandos JSON validados; motor nativo executa, com confirmação para operações destrutivas.
- Salvar, reabrir, autosave/recuperação, Undo/Redo e preservar mídia original.
- Exportar vídeo real MP4 horizontal 16:9, vertical 9:16 e quadrado 1:1, conforme codecs/licenças disponíveis; progresso, cancelamento e verificação de áudio/vídeo/legendas.

## 6. Contrato de comandos
JSON versionado com request_id, project_id, actions, requires_confirmation. Permitir apenas comandos explicitamente implementados e validados. Verificar IDs, intervalos, tipos, estado, dependências e permissões. Não executar shell arbitrário, mouse simulado, Selenium ou PyAutoGUI. Reaproveitar MCP do Drift se realmente existente e apropriado. Registrar alterações no Undo/Redo.

## 7. Segurança, confiabilidade e desempenho
Proteger credenciais com armazenamento seguro; HTTPS; sem chaves em código/logs; não enviar mídia privada à nuvem sem autorização. Timeouts, erro 429, cancelamento, falha de rede e fallback local. Processamento assíncrono, cache, uso eficiente de memória, fallback CPU e detecção de hardware. Não declarar progresso ou teste fictício.

## 8. Expansão futura (não bloquear MVP)
Multicâmera, melhores momentos, reenquadramento, presets avançados, contas, sincronização, assinaturas e processamento em nuvem. Preparar interfaces modulares, sem construir SaaS agora.

## 9. AVNS
Se pacote AVNS oficial for anexado, inspecionar e aplicar regras reais. Sem pacote, usar boas práticas documentadas, sem inventar convenções AVNS.

## 10. Fases e critérios de aceite
F1 Auditoria: evidência de arquivos/classes reais, licenças, build e riscos.
F2 Editor base: importar, reproduzir, cortar, Undo/Redo, salvar, exportar.
F3 IA local: silêncio, cortes, transcrição, legendas, áudio, transições.
F4 Gemini: chat QML, contexto, plano, JSON, validação, execução real, erros.
F5 Fluxo completo: importar → editar automaticamente → revisar → salvar → reabrir → exportar.
F6 Windows: Release x64, dependências, instalador, testes em Windows real.
Cada fase informa: arquivos alterados, testes executados, evidências, pendências e limitações.

## 11. Testes
Vídeo único e múltiplas trilhas; seleção e corte por ID; silêncio e continuidade; pt-BR e sincronização; edição manual de legendas; transições; áudio; Undo/Redo; projeto reaberto; erro Gemini/sem internet; exportação; instalação Windows. Nunca declarar sucesso sem executar.

## 12. Entregáveis
Código completo versionado, documentação técnica, inventário de licenças, testes, scripts CMake/build, ZIP, e instalador Windows somente se efetivamente compilado e testado. AI Studio não deve alegar compilação nativa quando indisponível.
