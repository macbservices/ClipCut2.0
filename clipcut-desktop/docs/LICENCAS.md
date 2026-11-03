# CLIP CUT 2.0 — DECLARAÇÃO DE LICENÇAS E CONFORMIDADE DE CÓDIGO ABERTO

O **Clip Cut 2.0** é distribuído como software livre e de código aberto. Este documento audita todas as licenças aplicáveis ao código-fonte, bibliotecas embutidas, codecs e modelos de inteligência artificial.

---

## 1. LICENÇA DO PROJETO PRINCIPAL (GPLv3)

O Clip Cut 2.0 é um trabalho derivado construído sobre o projeto **Drift** (CutWire-Studios), licenciado sob os termos da:

> **GNU General Public License, Version 3 (GPLv3)**  
> Copyright (C) 2007 Free Software Foundation, Inc.

### Obrigações e Garantias da GPLv3:
1. **Disponibilização do Código-Fonte:** O código-fonte completo e modificações correspondentes estão integralmente disponíveis no repositório do projeto e empacotados no arquivo `dist/clipcut-desktop-src-v2.0.0.zip`.
2. **Direito de Modificação e Redistribuição:** Qualquer usuário tem a liberdade de inspecionar, modificar, recompilar e redistribuir o software, desde que as versões derivadas mantenham a mesma licença GPLv3.
3. **Sem Fechamento Proprietário:** O aplicativo não pode ser distribuído como software fechado comercial sem o cumprimento integral das cláusulas de disponibilização de código correspondente.

---

## 2. INVENTÁRIO DE DEPENDÊNCIAS DE TERCEIROS

| Componente / Biblioteca | Versão Auditada | Licença de Software | Tipo de Vinculação | Observações de Conformidade |
| :--- | :---: | :---: | :---: | :--- |
| **Drift Engine** | v2.0.0 (commit base) | **GPLv3** | Nativa / Código-fonte | Núcleo do editor de vídeo e modelos de timeline. |
| **Qt 6 Framework** | 6.10.x | **LGPLv3** | Dinâmica (DLLs) | `QtCore`, `QtGui`, `QtQuick`, `QtQml`, `QtConcurrent`. Vinculação dinâmica preservada via `windeployqt`. |
| **FFmpeg Backend** | n7.1 | **GPLv3 / LGPLv2.1+** | Dinâmica (DLLs) | Compilado com `--enable-gpl` para habilitar codificação H.264 (`libx264`). Sujeito aos termos GPLv3. |
| **Skia Graphics Engine** | m124 | **BSD 3-Clause** | Estática (via vcpkg) | Motor de renderização vetorial GPU (Ganesh). Licença permissiva compatível com GPLv3. |
| **ONNX Runtime** | 1.18.x | **MIT License** | Dinâmica | Motor de inferência para redes neurais locais. |
| **Silero VAD** | v4.0 | **Apache 2.0** | Modelo neural (.onnx) | Detecção de atividade de voz acústica. Permite distribuição e uso livre. |
| **Whisper (OpenAI)** | Tiny / Base / Small | **MIT License** | Modelo neural (.onnx) | Modelo acústico de transcrição de fala pt-BR. Permite distribuição e uso livre. |
| **OpenSSL** | 3.x | **Apache 2.0** | Dinâmica (DLLs) | Criptografia para transporte HTTPS seguro no `GeminiProvider`. |
| **Zstandard (zstd)** | 1.5.x | **BSD / GPLv2** | Dinâmica | Algoritmo de compressão de pacotes e cache. |
| **SoundTouch** | 2.3.x | **LGPLv2.1** | Dinâmica | Processamento de pitch e time-stretching de áudio. |
| **JUCE Core / Audio** | 7.x | **GPLv3 / ISC** | Código embutido | Suporte a drivers de áudio e processamento multicanal. |

---

## 3. NOTAS SOBRE CODECS E PATENTES

* **H.264 (AVC) e AAC:** O FFmpeg distribuído com o Clip Cut 2.0 inclui decodificadores e codificadores para os padrões H.264 e AAC. Usuários corporativos que utilizem o software para distribuição comercial em larga escala devem verificar as políticas de licenciamento do consórcio MPEG LA / Via Licensing.
* **Formatos Abertos Recomendados:** Para fluxos 100% isentos de royalties, o Clip Cut 2.0 oferece suporte nativo a contêineres e codecs abertos como **VP9**, **AV1** e **Opus**.

---

## 4. CONFORMIDADE COM A DISTRIBUIÇÃO DO INSTALADOR

O instalador Windows `ClipCut-2.0-Setup-x64.exe` (gerado via Inno Setup):
- Apresenta o texto completo da licença GPLv3 durante a instalação (`LicenseFile=LICENSE`).
- Inclui o arquivo `LICENSE.txt` na pasta de instalação do programa.
- Disponibiliza o código-fonte correspondente e scripts de compilação no pacote oficial.
