# Matriz de cores

Versões exatas: cores/registry.json e gitlinks. Nenhuma ROM/BIOS comercial distribuída.

| Sistema | Core | ARMv7 | Execução comprovada | Política/pendência |
|---|---|---|---|---|
| NES | QuickNES 26bb785 | Compilado no APK | Demo original no host Windows e em Android TV virtual x86 | Philco, 10 min e Bluetooth físicos pendentes |
| SNES | Snes9x2005 a79dfe9 | Compilado | Não testado com jogo SNES | Build privado separado; licença mista/não comercial |
| Mega Drive / Master System | PicoDrive 1890c29 | Compilado | Não testado com jogo Mega Drive | Build privado separado; não comercial |
| CPS1/CPS2 | FBNeo 2b0c1c0, subset cps12 | Compilado | Não testado | ROM-set específico; licença customizada, build privado |
| PS1 | PCSX ReARMed c881679 | Compilado com VFS/CHD/NEON | Não testado com jogo PS1/BIOS | BIOS fornecida pelo usuário; CUE/CHD/M3U e discos condicionais à integração real |
| GB/GBC | Gambatte | Não empacotado | Não testado | Identificado no catálogo; erro claro ao abrir |
| N64 | Nenhum habilitado | Não integrado | Não testado | Spike separado; não suportado por esta versão |
| Wii/GameCube | Nenhum | Fora de escopo | Não testado | Não executar localmente |

PS1/arcade usam caminhos virtuais dentro da árvore SAF e VFS read-only de 64 bits. O teste automatizado prova seek/read de arquivo esparso de **5 GiB no Windows** com uso limitado de memória; não prova CHD nem provider USB Android. CUE/M3U resolvem referências relativas sem sair da pasta autorizada. Acesso por core que contorne VFS pode falhar e precisa de teste específico.

SRAM e estados dependem das funções expostas por cada core. Estados são vinculados ao binário do core (CRC do .so), à identidade do jogo e ao conteúdo. PS1 usa amostra início/fim + tamanho/mtime, não hash integral do CD. Memory cards são preservados após carregar snapshot, mas a combinação precisa de teste no PCSX real antes de confiar progresso importante.
