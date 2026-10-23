# Saves e recuperação

files/saves/<id>/progress.sram é progresso convencional; resume.state é snapshot. Envelope RTVSAVE2 inclui identidade, versão/binário do core, tipo, tamanho e CRC32. CRC detecta corrupção acidental, não autentica terceiros. Rom simples usa CRC de todos os bytes; discos usam amostras início/fim + tamanho/mtime para evitar leitura integral.

Gravação: temporário, flush, fsync, rename, cópia .prev e sincronização do diretório no Android. Save inválido anterior é preservado como .rejected, sem destruir .prev válido. Limite de snapshot: 32 MiB. SRAM é persistida periodicamente a cada 10s de execução, ao salvar, pausar e sair quando o lifecycle permite. Corte de energia pode perder alterações desde o último flush.

Quick Resume está disponível somente se retro_serialize/retro_unserialize funcionarem. Ao sair, tenta snapshot; quando não há suporte, SRAM e histórico continuam separados. Carregar snapshot faz backup da SRAM atual e reaplica a SRAM depois, para evitar reverter memory card. Isso requer validação específica por core (especialmente PS1).

Menu confirma sobrescrita de estado, carregamento, reinício e saída. Quando conteúdo/core muda, restauração é recusada, preservando arquivos. Não se apagam saves automaticamente.

Backup ZIP manual via SAF: exporta arquivos de saves com manifest SHA-256; restauração valida nomes, checksum, limites, duplicatas e caminhos antes de trocar diretório. Limites: 4.096 arquivos, 34 MiB por arquivo e 128 MiB total. saves.previous guarda uma cópia completa anterior. A operação exige confirmação; não há sincronização em nuvem. Enquanto há transferência, abrir jogo é bloqueado para não concorrer com a troca dos saves.
