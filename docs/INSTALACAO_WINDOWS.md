# CLIP CUT 2.0 — GUIA DE INSTALAÇÃO NO WINDOWS 10/11 x64

Este documento descreve os procedimentos para instalar, configurar e desinstalar o **Clip Cut 2.0** no **Windows 10/11 x64**.

---

## 1. REQUISITOS DO SISTEMA

### Requisitos Mínimos
- **Sistema Operacional:** Windows 10 x64 (versão 21H2 ou superior) ou Windows 11 x64.
- **Processador:** Intel Core i3 / AMD Ryzen 3 (64 bits com suporte a AVX2).
- **Memória RAM:** 8 GB.
- **Armazenamento:** 2 GB de espaço livre em disco (recomenda-se SSD).
- **Placa de Vídeo:** Compatível com OpenGL 3.3 / DirectX 11 (GPU integrada Intel HD Graphics 620 ou superior).
- **Resolução de Tela:** 1366x768.

### Requisitos Recomendados
- **Processador:** Intel Core i7 / AMD Ryzen 7 (8 núcleos ou superior).
- **Memória RAM:** 16 GB ou 32 GB (para edição de vídeos 4K e renderizações pesadas).
- **Placa de Vídeo:** GPU dedicada NVIDIA GeForce RTX 2060+ ou AMD Radeon RX 5600+ com 6 GB+ VRAM.
- **Resolução de Tela:** 1920x1080 (Full HD) ou superior com DPI escalável.

---

## 2. MODALIDADES DE DISTRIBUIÇÃO

O Clip Cut 2.0 está disponível em dois formatos de distribuição para Windows:

1. **Instalador Oficial (`ClipCut-2.0-Setup-x64.exe`):** Recomendado para usuários finais. Cria atalhos, associa os formatos de projeto (`.clipcut` e `.drift`) e suporta desinstalação automática.
2. **Pacote Portátil (`ClipCut-2.0-Windows-x64-Portable.zip`):** Não requer instalação. Basta extrair e rodar `ClipCut2.exe`. Ideal para pen drives ou ambientes corporativos com políticas de permissão restritivas.

---

## 3. INSTALAÇÃO VIA INSTALADOR OFICIAL

1. **Download do Instalador:** Obtenha o arquivo `ClipCut-2.0-Setup-x64.exe`.
2. **Execução:** Dê um duplo clique sobre o executável.
   > **Nota sobre Permissões:** O instalador foi configurado com `PrivilegesRequired=lowest`. Por padrão, ele instala no diretório do usuário local (`%LocalAppData%\Programs\Clip Cut 2.0`), **sem exigir privilégios de Administrador**. Se você preferir instalar para todos os usuários (`C:\Program Files\Clip Cut 2.0`), execute o instalador como Administrador.
3. **Contrato de Licença:** O instalador apresentará os termos da licença **GNU General Public License v3 (GPLv3)**. Leia e clique em **Avançar**.
4. **Pasta de Destino:** Selecione a pasta onde o programa será instalado (padrão recomendado).
5. **Opções Adicionais:**
   - [x] Criar atalho na Área de Trabalho (Desktop).
   - [x] Associar arquivos de projeto `.clipcut` e `.drift`.
6. **Conclusão:** Clique em **Instalar**. Ao final, marque **Executar o Clip Cut 2.0** e clique em **Concluir**.

---

## 4. INSTALAÇÃO VIA PACOTE PORTÁTIL (ZIP)

1. Baixe o arquivo `ClipCut-2.0-Windows-x64-Portable.zip`.
2. Clique com o botão direito no arquivo e selecione **Extrair Tudo...**.
3. Escolha uma pasta de destino (exemplo: `C:\ClipCut2\`).
4. Abra a pasta extraída e localize o executável `ClipCut2.exe`.
5. Dê um duplo clique em `ClipCut2.exe` para iniciar o editor.

---

## 5. EXPERIÊNCIA DA PRIMEIRA EXECUÇÃO

Ao abrir o Clip Cut 2.0 pela primeira vez:

1. **Verificação Automática de Dependências:**
   - O aplicativo verifica se as bibliotecas do FFmpeg 7.1 (`avcodec-61.dll`, `avformat-61.dll`, `avutil-59.dll`) estão presentes na pasta de binários.
   - Testa a aceleração de renderização gráfica (OpenGL/Direct3D via Qt RHI e Skia).
   - Detecta os modelos ONNX locais (`silero_vad.onnx` e `whisper-small`).
2. **Tela Inicial do Editor:**
   - Permite clicar em **Novo Projeto** ou **Abrir Projeto Existente** (`.clipcut` / `.drift`).
   - Apresenta as 5 áreas de trabalho nativas: Monitor de Vídeo, Timeline de Trilhas, Biblioteca de Mídia, Painel de Propriedades e Painel de IA.
3. **Configuração Opcional do Google Gemini:**
   - No painel lateral de IA, clique no ícone ⚙ (Configurações).
   - Insira sua chave de API oficial do Google Gemini ou confirme o uso da variável de ambiente `GEMINI_API_KEY`.
   - **Uso 100% Offline:** Se você não configurar a chave do Gemini ou não tiver conexão com a internet, o Clip Cut 2.0 funcionará normalmente utilizando seus motores locais de corte de silêncio, normalização de áudio e legendas automáticas.

---

## 6. DESINSTALAÇÃO

Para remover o Clip Cut 2.0 do computador:

1. Abra o menu **Iniciar** do Windows e clique em **Configurações** (ou pressione `Win + I`).
2. Vá em **Aplicativos** > **Aplicativos Instalados**.
3. Localize **Clip Cut 2.0** na lista.
4. Clique nos três pontos (`...`) e selecione **Desinstalar**.
5. O assistente de desinstalação removerá todos os arquivos do aplicativo, atalhos do menu Iniciar e associações de arquivos registradas.
6. Seus projetos (`.clipcut` / `.drift`) e arquivos de vídeo exportados em pastas pessoais **não serão apagados**.
