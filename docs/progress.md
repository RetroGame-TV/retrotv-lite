# Progresso real

Não há TV conectada nem validação física. Fases são marcos de implementação, não certificação.

| Fase | Código | Verificação / pendências |
|---|---|---|
| F0 | Estrutura Gradle, Kotlin/SDL, GLES2, ARMv7 e manifesto TV | APK debug ARMv7 compilou; abertura na Philco pendente |
| F1 | UI original, lista virtual, detalhes, configurações, temas, D-pad/gamepad | 8 cenários C++ executados e aprovados no Windows; render/TV pendentes |
| F2 | SAF read-only persistente, scan cancelável, SQLite, favoritos/recentes, temas | Testes Kotlin preparados; USB/ROM OEM e migração física pendentes |
| F3 | Host Libretro + QuickNES real, áudio/vídeo/input, sessão, demo CC0 | CTest real QuickNES passou no Windows; build ARMv7 passou; 10 min/Philco pendentes |
| F4 | Pendente | Saves, controles, cores adicionais |
| F5 | Pendente | Spike N64 isolado |
| F6 | Pendente | CI, testes e pacote |
| F7 | Adiado | Extras só após fluxo principal validado |

## F0

Pasta inicial vazia. Java 21 e Git disponíveis; SDK/NDK ausentes. Download oficial iniciado dentro de work, sem alterar instalação global. Dependências fixadas por gitlink. Testes físicos, PSS e FPS na Philco pendentes.

## F1

`native/ui/` contém modelo independente de SDL e renderer. `clang++ -std=c++17 -I native native/tests/policy_tests.cpp -o policy_tests.exe` seguido de `policy_tests.exe`: 8 cenários passaram (coleção vazia, detalhes/contexto, foco por aba, favoritos, recentes, catálogo removido, plataforma não comprovada e orçamento de CD/cache). Primeiro build Android ainda em execução. Sem inferência de teste físico.

## F2

Banco v1 com índices e atualização preservando metadados. Scan incremental de metadados: não reabre bytes de ROM no rescan, mas enumera diretórios para detectar mudanças. Cancelamento/erro preserva catálogo. Extensões ambíguas exigem pasta identificada. Primeiro build encontrou cast jobject faltante na ponte SDL; corrigido. Não houve teste físico USB.

## F3

Build Gradle `:app:assembleDebug :app:testDebugUnitTest` passou em 25s após corrigir retorno Kotlin em função de corpo expressão. Host nativo e QuickNES compilados também para Windows com LLVM-MinGW; CTest 2/2 passou (políticas + execução real da demo original). Exercitados 241+ frames, áudio, input alterando frame, pause/resume, reset, serialize/unserialize, CRC/identidade/backup de saves e 10 ciclos abrir/fechar. Esses são testes em host, não Android/Philco. Programa NES de 24.592 bytes gerado no próprio projeto sob CC0. Não mede desempenho real da TV.
