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
| F6 | Build debug/release, lint, CI preparado, scripts, documentação e pacote | Testes locais e Android virtual passaram; assinatura release/CI GitHub/TV pendentes |
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

## F6: evidências finais

| Verificação executada | Resultado observado |
|---|---|
| Gradle `:app:assembleDebug :app:testDebugUnitTest :app:lintDebug :app:assembleRelease` | BUILD SUCCESSFUL; APK ARMv7 debug e release **não assinado** gerados |
| Auditoria ZIP/ELF do APK debug | 5 libs exclusivamente ELF32 ARM: SDL2, libc++, main, QuickNES e PCSX ReARMed; demo e licenças presentes |
| `apksigner verify --verbose` | Assinatura debug v2 verificada; não é assinatura release |
| JUnit JVM | 7 testes, zero falhas: catálogo, paths e backup |
| CTest Windows/LLVM-MinGW | 3/3 suítes: 9 cenários de navegação/política, core QuickNES real, VFS esparso de 5 GiB |
| Host QuickNES real | Vídeo/input/áudio, pausa/reset, SRAM com bateria preservada ao reabrir, state, corrupção, proteção do .prev e 10 ciclos de sessão passaram |
| Android TV 11 x86 virtual / AndroidJUnitRunner | 2 testes passaram: provider SAF sintético com falhas/remoção/retorno e migração v1→v2 com favoritos e URI >500 caracteres |
| Navegação Android virtual | Demo iniciou com DPAD_CENTER/OK; menu rápido, salvar/carregar estado, saída à biblioteca e Home/retorno observados; captura real do renderer |
| Build de cores adicionais | ARMv7 SNES/PicoDrive/PCSX/FBNeo CPS12 compilados; execução com jogos desses sistemas permanece não validada |
| CI GitHub Actions | Workflow criado; não executado remotamente |
| Philco / USB físico / gamepads físicos | Não disponíveis; sem instalação, FPS/PSS ou compatibilidade afirmados |

A revalidação do USB no resume é uma consulta leve ao documento raiz, sem rescan completo. Eventos de montagem iniciam scan; remoção revalida a árvore e marca indisponibilidade. O provider OEM real ainda precisa ser testado. Lint passou com exceções **somente nas APIs opcionais de quatro arquivos SDL**, descritas em licenses.md. Nenhuma permissão INTERNET, microfone, Bluetooth scanning ou armazenamento irrestrito foi adicionada.

Entrega fora da árvore do código: APK debug instalável, APK release sem assinatura, ZIP de fontes com dependências, screenshot e logs/checksums em `outputs/`. Documentação de build não depende dos caminhos temporários desta máquina. Gitlinks permanecem nas revisões upstream; alterações de build necessárias são reproduzidas por scripts e patches (submódulos podem aparecer modificados por isso).

## F7 e escopo restante

Adiado conforme gate do plano: scraper opt-in, perfis de usuário, previews em vídeo e personalização avançada exigem primeiro fluxo principal comprovado no hardware. Também não foram concluídos editor avançado de perfil por jogo/console, reconhecimento universal de gamepads nem teste físico dos consoles adicionais. A versão de desenvolvimento não é 1.0 completa/validada. Prioridade seguinte é executar `docs/test-on-philco.md` com arquivos legais do proprietário.

## Revisão visual · 0.2.0-dev

Solicitação posterior do usuário antecipou a direção visual. Barra lateral e listas sólidas substituídas por navegação superior, galeria de seis capas, foco elevado, fundo contextual discreto e detalhes com capa. Minimal recebe lista monocromática e desenho original de console. Ajustes, pausa, confirmação, diagnóstico e estado vazio seguem a mesma linguagem. Banner Android atualizado.

Capas locais PNG/JPEG/WebP via SAF, worker e caches limitados implementados. Banco migra v1/v2→v3 sem substituir linhas, favoritos, histórico ou caminhos. Núcleo, SRAM e protocolo de save state permanecem os mesmos. Ilustrações e cartazes de teste são originais; referência Delta usada apenas para análise visual.

Comandos desta revisão: Gradle x86 `:app:assembleDebug :app:assembleDebugAndroidTest -PtestAbi=x86`; AndroidJUnitRunner; host `cmake --build`/`ctest`; Gradle ARMv7 `:app:assembleDebug :app:testDebugUnitTest :app:lintDebug :app:assembleRelease`. Builds passaram, lint 0 erros, JUnit 7/7, CTest 3/3 e Android 4/4. Auditoria ARMv7 e assinatura debug v2 passaram. Capturas reais incluem Galeria, Minimal, detalhes, ajustes e pausa. Veja `visual-validation-0.2.0.md`; logs e screenshots acompanham `outputs/evidence/design-0.2.0/` na entrega. A Philco física continua pendente.


## Refinamento e perfis · 0.3.0-dev

Direção 0.2 mantida e refinada: proporções das capas, tipografia-base de 40 px no mesmo atlas, correção de alpha Android→SDL, formas com antialiasing, foco sem corte e rolagem de cinco capas com próxima parcial. Detalhes distinguem início/continuação; pausa identifica a sessão real. Estados vazios oferecem recuperação contextual. Nenhum asset de Delta/RetroArch foi copiado.

Editor de proporção/filtro/volume por jogo e console implementado, persistente e aplicado ao retomar. Demais opções avançadas do perfil (núcleo/frameskip/remapeamento por console) continuam pendentes. SRAM passou a validar família do núcleo e tamanho, preservando compatibilidade após recompilação; snapshots mantêm identidade exata do binário. Testes usam metadados antigos reais do envelope RTVSAVE2.

CTest 3/3, JUnit JVM 7/7, Android 4/4 e lint 0 erros/53 avisos. Navegação, tema Minimal, perfil herdado/reset, partida, Quick Resume e rejeição de snapshot antigo observados no Android TV virtual. APK debug ARMv7 pessoal reúne os cinco núcleos já compilados; auditoria ELF32/assinatura v2/permissões passou. Apenas NES executado; outros consoles e TV física continuam pendentes. Consulte visual-validation-0.3.0.md para comandos e limites. Projeto, SDK e build permanecem no disco D.