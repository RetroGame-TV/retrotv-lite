# Saves e recuperação

Atualização SNES 0.7.4: [seleção de núcleo por snapshot e cópias permanentes antes de LAGFIX](snes-latency.md). Jogar agora usa a variante rápida; Continuar partida mantém o binário exato do estado existente. Não há conversão automática nem relaxamento da validação de snapshots.

files/saves/<id>/progress.sram é progresso convencional; resume.state é snapshot. Envelope RTVSAVE2 inclui identidade do conteúdo, identidade do core, tipo, tamanho e CRC32. Snapshot exige versão e hash binário exatos do núcleo. SRAM usa família do núcleo (por exemplo QuickNES), permitindo recompilar/atualizar o mesmo núcleo; o host ainda exige tamanho exato. Metadados antigos de SRAM com versão/hash são aceitos na mesma família. Troca de família, conteúdo, tamanho ou CRC inválido é recusada. CRC detecta corrupção acidental, não autentica terceiros. Rom simples usa CRC de todos os bytes; discos usam amostras início/fim + tamanho/mtime para evitar leitura integral.

Gravação: temporário, flush, fsync, rename, cópia .prev e sincronização do diretório no Android. Save inválido anterior é preservado como .rejected, sem destruir .prev válido. Limite de snapshot: 32 MiB. SRAM é persistida periodicamente a cada 10s de execução, ao salvar, pausar e sair quando o lifecycle permite. Corte de energia pode perder alterações desde o último flush.

Quick Resume está disponível somente se retro_serialize/retro_unserialize funcionarem. Ao sair, tenta snapshot; quando não há suporte, SRAM e histórico continuam separados. Carregar snapshot faz backup da SRAM atual e reaplica a SRAM depois, para evitar reverter memory card. Isso requer validação específica por core (especialmente PS1).

Menu confirma sobrescrita de estado, carregamento, reinício e saída. Quando conteúdo ou família de core muda, a SRAM é recusada; snapshots também recusam outra versão/binário. Os arquivos são preservados. Não se apagam saves automaticamente.

Backup ZIP manual via SAF: exporta arquivos de saves com manifest SHA-256; restauração valida nomes, checksum, limites, duplicatas e caminhos antes de trocar diretório. Limites: 4.096 arquivos, 34 MiB por arquivo e 128 MiB total. saves.previous guarda uma cópia completa anterior. A operação exige confirmação; não há sincronização em nuvem. Enquanto há transferência, abrir jogo é bloqueado para não concorrer com a troca dos saves.
