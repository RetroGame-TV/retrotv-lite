# USB e SAF

Selecione uma subpasta em Configurações > Biblioteca USB. Android 11 pode recusar raiz do volume e Downloads. A permissão é somente leitura e persistida por ContentResolver. Não se pede MANAGE_EXTERNAL_STORAGE, root ou acesso a Android/data.

Scan é cancelável, fora da render thread, sem carregar ROMs. IDs SHA-256 de árvore + document ID; metadados são atualizados sem REPLACE para preservar favoritos/recentes/tempo. Limites explícitos: 20.000 diretórios, 10.000 jogos; selecione subpasta se excedidos. Scan interrompido não invalida itens não vistos. Ao concluir, arquivos ausentes ficam indisponíveis sem apagar histórico.

Persistir URI não prova presença do USB: cada abertura revalida e retorna erro recuperável. Providers precisam permitir seek no descritor. Não se copiam CDs para RAM/flash. Provedores sem seek e TV sem DocumentsUI ficam bloqueados com orientação, sem contornar permissões. Em OEM sem seletor SAF, habilitar/instalar DocumentsUI por procedimento oficial do fabricante ou usar volume/provider suportado; o app não instala componentes de terceiros.

Ao retomar o app, consulta somente o documento raiz para revalidar presença/permissão. Broadcasts de mídia disparam revalidação na remoção e scan na montagem. Essa integração foi testada com falhas de provider sintético; broadcasts e volumes físicos da TV de referência continuam pendentes. URIs longas não são truncadas pelo protocolo de catálogo.

Layout: Games/NES/*.nes, Games/SNES/*.sfc, Games/Genesis/*.md, Games/Arcade/*.zip, Games/PS1/*.chd ou *.cue/*.m3u. BIN isolado é ambíguo e só é jogo Mega Drive em pasta Genesis/Mega. Faixas BIN de PS1 não viram jogos duplicados. Suporte efetivo depende do core empacotado.

Pendências físicas: retirada/reinserção de USB, revogação de permissão, provider OEM, diretórios grandes e PS1 aleatório via VFS.
