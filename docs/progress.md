# Progresso real

Não há TV conectada nem validação física. Fases são marcos de implementação, não certificação.

| Fase | Código | Verificação / pendências |
|---|---|---|
| F0 | Estrutura Gradle, Kotlin/SDL, GLES2, ARMv7 e manifesto TV | APK debug ARMv7 compilou; abertura na Philco pendente |
| F1 | UI original, lista virtual, detalhes, configurações, temas, D-pad/gamepad | 8 cenários C++ executados e aprovados no Windows; render/TV pendentes |
| F2 | SAF read-only persistente, scan cancelável, SQLite, favoritos/recentes, temas | Testes Kotlin preparados; USB/ROM OEM e migração física pendentes |
| F3 | Host Libretro + QuickNES real, áudio/vídeo/input, sessão, demo CC0 | CTest real QuickNES passou no Windows; build ARMv7 passou; 10 min/Philco pendentes |
| F4 | Saves/Quick Resume, backup, remapeamento, 2 slots; SNES/Pico/FBNeo/PCSX compilados ARMv7 | 7 testes Kotlin e 3 CTests passaram; jogos dos cores extras, Bluetooth e USB reais pendentes |
| F5 | Spike documentado, N64 oculto/desabilitado e gate de HW render | Sem integração GLES2 comprovada; não suportado nesta versão |
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

## F4

- Demo executou no APK Android TV 11 x86 virtual e foi capturada. Encontrado/corrigido DPAD_CENTER SDL SELECT (Enter não cobria o OK real).
- SRAM e estados separados; envelope/checksum/identidade do conteúdo e core, atomic rename/fsync, .prev e .rejected. ZIP export/import com manifest SHA-256, staging, confirmação e cópia de diretório anterior.
- Kotlin: 7 testes JUnit aprovados (3 catálogo, 1 caminho virtual, 3 backup). CTest: 3/3 aprovados, incluindo seek/read de arquivo esparso de 5 GiB no host Windows.
- NDK ARMv7: Snes9x2005, PicoDrive, PCSX ReARMed e FBNeo CPS12 compilaram. FBNeo precisou de caminhos de objeto relativos e regeneração de driverlist para subset; patch/script entregues.
- Padrão inclui QuickNES e PCSX quando construído pelo script. Cores não comerciais ficam em build privado separado. Não houve execução de jogo SNES/Mega Drive/Arcade/PS1 nem teste de BIOS/CHD na TV.
- Testes Android de provider SAF sintético/migração SQLite preparados. Lint encontrou permissões de APIs opcionais SDL e um flag SAF; flag corrigido e exceções SDL restritas a quatro arquivos, sem adicionar permissões.

## F5

Pesquisa upstream de Mupen64Plus-Next/ParaLLEl registrada em n64-spike.md. Faltam contexto HW Libretro/GLES2 integrado, plugin ARMv7 comprovado e medições na Mali. Não há atalho N64 ativo, nem promessa de desempenho. Gate automatizado rejeita habilitação incidental. Fase de investigação concluída; suporte não implementado.
