# Menor atraso no SNES sem converter partidas antigas

O build pessoal 0.7.4-dev adiciona uma variante de Snes9x 2005 com a opção upstream `LAGFIX`. A revisão continua a79dfe9047e7fec58808aefe48ad2bf499c7af11. O Makefile geral já oferece essa opção, mas o Android.mk upstream não a repassa. O wrapper local em `cores/snes9x2005/Android.mk` a habilita explicitamente; não modifica fontes nem o gitlink upstream.

## Saves e atualização

- O pacote mantém **o binário SNES anterior, byte a byte**, em `libsnes9x2005_libretro.so`. O novo usa `libsnes9x2005_lowlatency_libretro.so`.
- **Jogar agora** usa o novo, quando presente. SRAM continua sob a mesma identidade de jogo e família de núcleo; o tamanho exigido pelo host não muda.
- **Continuar partida** seleciona o binário cujo CRC consta no snapshot. Depois de abrir, a identidade completa, conteúdo, checksum e tamanho continuam validados pelo SaveStore/Host. Não se aceitam snapshots de outro build como se fossem compatíveis.
- Carregar pelo menu rápido também pode reabrir o núcleo anterior, preservando a SRAM atual. Estados novos continuam com o novo núcleo.
- Uma partida retomada de um snapshot antigo **continua com o comportamento de atraso antigo**. Para aproveitar a redução, usar Jogar agora. Não convertemos a posição exata de uma partida antiga para o novo núcleo sem comprovação de compatibilidade.
- Antes da primeira execução acelerada, saves existentes e `.prev` recebem cópias permanentes com sufixo `.before-snes-lagfix`. Essas cópias não são substituídas pelas gravações seguintes e entram no backup ZIP normal. Falha na cópia aborta a abertura antes da execução do jogo. Os `.prev` continuam funcionando separadamente.
- Núcleo desconhecido ou save corrompido é recusado com os arquivos preservados. Nenhuma exclusão automática ou migração de banco é feita. Os demais núcleos permanecem iguais no APK pessoal de atualização.

Instalar sobre o app existente, com a mesma assinatura e applicationId; não desinstalar nem limpar dados. A validação desta entrega ocorreu apenas na TV virtual. Nenhum save da TV física foi acessado ou alterado.

## Build pessoal

Preservar os núcleos conhecidos do APK anterior nos diretórios privados locais antes de compilar. Não recompilar o arquivo legado por cima: a identidade binária é necessária para os snapshots existentes.

```powershell
./scripts/build-cores.ps1 -Core snes9x2005 -PrivateCores -SnesLagFix
./gradlew.bat :app:testDebugUnitTest :app:lintDebug :app:assembleDebug -PprivateCores=true -PpackagedQuicknes=true
# Apenas a variante da TV virtual:
./scripts/build-cores.ps1 -Core snes9x2005 -PrivateCores -SnesLagFix -Abi x86
./gradlew.bat :app:assembleDebug :app:assembleDebugAndroidTest -PtestAbi=x86 -PprivateCores=true
```

Sem `-SnesLagFix`, o script mantém o build sem essa opção. A variante acelerada tem diretório de objetos e nome de saída separados; sua compilação não troca o legado. Os limites de licença privada de SNES/PicoDrive/FBNeo continuam em vigor; os binários e o APK pessoal não são artefatos públicos do CI.

## Verificação local em 27/10/2026

Usando a cópia de Street Fighter II (USA) do usuário, um executável de teste Android com o Host real carregou os dois `.so` x86. Para cada comando, restaurou o mesmo estado, comparou uma sequência neutra repetida e detectou a primeira diferença visual ao pressionar. Foram 12 comandos × 5 repetições × 2 builds = 120 comparações determinísticas.

| Entrada | Núcleo anterior | LAGFIX |
|---|---:|---:|
| Quatro direções e duas diagonais | 3 quadros | 2 quadros |
| Seis botões de golpe | 4 quadros | 3 quadros |

O ganho interno é um quadro, nominalmente 16,69 ms a 59,9227 Hz. Isso não mede toque físico, rádio Bluetooth, compositor ou tela da TV. Não se deduz FPS da TV dessa medição. O APK de destino contém ARMv7 compilado com a mesma opção, mas não houve execução ARMv7 em aparelho físico.

Na TV virtual, instalação `-r` 0.7.3 → 0.7.4 manteve os quatro arquivos de save de teste idênticos por SHA-256. A retomada real do SF2 abriu com o `.so` anterior; Jogar agora carregou o `.so` acelerado, criou a cópia permanente idêntica e Carregar pelo menu reabriu o anterior. A escolha foi conferida também nos mapas das bibliotecas carregadas do processo. Um novo estado salvo pelo menu do próprio app reabriu no núcleo acelerado após reiniciar o processo, com o backup antigo ainda idêntico. Direções, diagonal e seis golpes foram enviados pelo receptor TCP real durante a partida; CPU adversária/animações impedem usar essa verificação visual como contagem estatística de golpes.

Cinco suítes CTest passam: políticas, QuickNES real/SRAM/estados, VFS, leitura de entrada SDL e a nova proteção de saves SNES. Esta última cobre seleção por binário, SRAM entre builds, recusa de snapshot incompatível, cópias imutáveis após oito gravações, dados de outro jogo, núcleo ausente/desconhecido e cabeçalhos/checksums corrompidos. JVM mantém 22 testes; os oito testes Android de entrada/metadados/armazenamento passaram novamente, incluindo rajadas de 12 toques/24 transições entre 1.200 movimentos de eixos. Lint tem zero erros e 51 avisos anteriores. Nenhuma ROM ou save do usuário é versionado.
