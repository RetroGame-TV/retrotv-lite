# Diagnóstico e recuperação

**APK não instala:** confirme armeabi-v7a e Android 8+ (alvo é TV 11). APK release-unsigned precisa ser assinado; para teste use debug. Não desinstale para resolver erro de assinatura sem antes exportar saves, pois desinstalar apaga dados privados.

**Sem seletor USB:** ROM OEM pode não incluir DocumentsUI. Não há acesso irrestrito/root alternativo no app. Use provider oficialmente suportado pelo fabricante. A raiz de USB/Downloads pode ser vedada no Android 11; escolha subpasta.

**Jogo não abre:** confira formato e core na matriz. Cores ausentes retornam à biblioteca. SNES/PicoDrive/FBNeo exigem build privado opcional e gate de licença; N64 está indisponível. Arcade precisa ROM-set correspondente ao FBNeo fixado, sem prometer suporte universal.

**PS1:** forneça BIOS de 512 KiB do seu acervo; importação atual nomeia scph5501.bin e exige verificar a região/conteúdo correto (não valida autenticidade da BIOS). Referências CUE/M3U devem existir na árvore autorizada. Provider sem seek ou core que ignore VFS pode falhar. Não se copia disco inteiro para armazenamento interno.

**Save incompatível:** não apague original. Confira versão/core/conteúdo; mantenha .prev e .rejected, exporte backup e recupere uma cópia compatível. Snapshot de core diferente não é portável por definição. saves.previous contém a pasta inteira anterior ao último restore.

**Gamepad desconectado:** OK/Voltar abre pausa; reconecte. START+SELECT ou Menu/Voltar recuperam menu rápido. Layouts não reconhecidos pelo SDL precisam de mapeamento SDL adicional; não há suporte universal testado.

**Áudio/performance:** compare modo Performance, jogos leves e resolução nativa. Painel mostra custo retro_run, carga e contadores locais; não é benchmark da TV de referência até medir no aparelho. Não habilitar filtros, Vulkan/GLES3 ou cores sem evidência.

**Build Windows:** use Java 17+, SDK/NDK/CMake fixados e caminho curto. `build-cores.ps1` usa caminhos absolutos e corrige objetos FBNeo longos. Perl é necessário para lista CPS1/CPS2 (Git para Windows o inclui). Nada depende de uma pasta temporária desta máquina.
