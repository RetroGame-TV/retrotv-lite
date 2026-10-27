# Progresso real

## Atualização de entrada · 27/10/2026

- Implementado e testado: esperar o próximo quadro antes de consultar o telefone/processar SDL/coletar botões e eixos. Menu e primeiro quadro ao lançar/retomar mantêm seu comportamento.
- Implementado e testado: analógicos usam o valor mais recente; cada mudança digital entra em FIFO, sem pacotes de movimento ocupando quadros adicionais. Toques curtos, diagonais, combinações, Home/Voltar e direção segurada são preservados. Saturação de 64 transições é explícita e neutraliza/desconecta em vez de descartar uma soltura silenciosamente; reset não apaga a conexão substituta.
- Executado: 21 testes JVM, 4 CTests e 8 testes Android em Android TV 11/API 30 x86 virtual. O novo CTest usa SDL real, dois controles virtuais e o host/QuickNES real por 148 quadros, conferindo movimento e parada após soltura. O Android usa receptor TCP real com 12 toques e 1.200 atualizações de eixos.
- Executado no APK x86 com jogo ativo: a mesma rajada de 12 toques pelo canal de celular produziu 12 passos na demo original. Captura passou de x=535 a x=580 (45 px de tela/12 px NES); o frame ficou idêntico após mais um segundo solto, ainda conectado. Capturas e detalhes em [entrada e latência](input-latency.md).
- Builds debug ARMv7 e x86/APK de testes passaram; lint com zero erros/51 avisos. APK ARMv7 pessoal mantém o certificado e os cinco núcleos idênticos aos da versão anterior, usando opção local explícita para preservar a identidade do QuickNES e seus estados salvos. Build padrão/CI continua compilando QuickNES das fontes fixadas.
- Pendente: Bluetooth RFCOMM entre aparelhos, latência física toque→imagem e sessão longa em todos os consoles. Não houve instalação em TV física nesta alteração.

## Histórico dos marcos

Não há TV conectada nem validação física. Fases são marcos de implementação, não certificação.

| Fase | Código | Verificação / pendências |
|---|---|---|
| F0 | Estrutura Gradle, Kotlin/SDL, GLES2, ARMv7 e manifesto TV | APK debug ARMv7 compilou; abertura na TV de referência pendente |
| F1 | UI original, lista virtual, detalhes, configurações, temas, D-pad/gamepad | 8 cenários C++ executados e aprovados no Windows; render/TV pendentes |
| F2 | SAF read-only persistente, scan cancelável, SQLite, favoritos/recentes, temas | Testes Kotlin preparados; USB/ROM OEM e migração física pendentes |
| F3 | Host Libretro + QuickNES real, áudio/vídeo/input, sessão, demo CC0 | CTest real QuickNES passou no Windows; build ARMv7 passou; 10 min/TV de referência pendentes |
| F4 | Saves/Quick Resume, backup, remapeamento, 2 slots; SNES/Pico/FBNeo/PCSX compilados ARMv7 | 7 testes Kotlin e 3 CTests passaram; jogos dos cores extras, Bluetooth e USB reais pendentes |
| F5 | Spike documentado, N64 oculto/desabilitado e gate de HW render | Sem integração GLES2 comprovada; não suportado nesta versão |
| F6 | Build debug/release, lint, CI preparado, scripts, documentação e pacote | Testes locais e Android virtual passaram; assinatura release/CI GitHub/TV pendentes |
| F7 | Adiado | Extras só após fluxo principal validado |

## F0

Pasta inicial vazia. Java 21 e Git disponíveis; SDK/NDK ausentes. Download oficial iniciado dentro de work, sem alterar instalação global. Dependências fixadas por gitlink. Testes físicos, PSS e FPS na TV de referência pendentes.

## F1

`native/ui/` contém modelo independente de SDL e renderer. `clang++ -std=c++17 -I native native/tests/policy_tests.cpp -o policy_tests.exe` seguido de `policy_tests.exe`: 8 cenários passaram (coleção vazia, detalhes/contexto, foco por aba, favoritos, recentes, catálogo removido, plataforma não comprovada e orçamento de CD/cache). Primeiro build Android ainda em execução. Sem inferência de teste físico.

## F2

Banco v1 com índices e atualização preservando metadados. Scan incremental de metadados: não reabre bytes de ROM no rescan, mas enumera diretórios para detectar mudanças. Cancelamento/erro preserva catálogo. Extensões ambíguas exigem pasta identificada. Primeiro build encontrou cast jobject faltante na ponte SDL; corrigido. Não houve teste físico USB.

## F3

Build Gradle `:app:assembleDebug :app:testDebugUnitTest` passou em 25s após corrigir retorno Kotlin em função de corpo expressão. Host nativo e QuickNES compilados também para Windows com LLVM-MinGW; CTest 2/2 passou (políticas + execução real da demo original). Exercitados 241+ frames, áudio, input alterando frame, pause/resume, reset, serialize/unserialize, CRC/identidade/backup de saves e 10 ciclos abrir/fechar. Esses são testes em host, não Android/TV de referência. Programa NES de 24.592 bytes gerado no próprio projeto sob CC0. Não mede desempenho real da TV.

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
| TV de referência / USB físico / gamepads físicos | Não disponíveis; sem instalação, FPS/PSS ou compatibilidade afirmados |

A revalidação do USB no resume é uma consulta leve ao documento raiz, sem rescan completo. Eventos de montagem iniciam scan; remoção revalida a árvore e marca indisponibilidade. O provider OEM real ainda precisa ser testado. Lint passou com exceções **somente nas APIs opcionais de quatro arquivos SDL**, descritas em licenses.md. Nenhuma permissão INTERNET, microfone, Bluetooth scanning ou armazenamento irrestrito foi adicionada.

Entrega fora da árvore do código: APK debug instalável, APK release sem assinatura, ZIP de fontes com dependências, screenshot e logs/checksums em `outputs/`. Documentação de build não depende dos caminhos temporários desta máquina. Gitlinks permanecem nas revisões upstream; alterações de build necessárias são reproduzidas por scripts e patches (submódulos podem aparecer modificados por isso).

## F7 e escopo restante

Adiado conforme gate do plano: scraper opt-in, perfis de usuário, previews em vídeo e personalização avançada exigem primeiro fluxo principal comprovado no hardware. Também não foram concluídos editor avançado de perfil por jogo/console, reconhecimento universal de gamepads nem teste físico dos consoles adicionais. A versão de desenvolvimento não é 1.0 completa/validada. Prioridade seguinte é executar `docs/test-on-tv.md` com arquivos legais do proprietário.

## Revisão visual · 0.2.0-dev

Solicitação posterior do usuário antecipou a direção visual. Barra lateral e listas sólidas substituídas por navegação superior, galeria de seis capas, foco elevado, fundo contextual discreto e detalhes com capa. Minimal recebe lista monocromática e desenho original de console. Ajustes, pausa, confirmação, diagnóstico e estado vazio seguem a mesma linguagem. Banner Android atualizado.

Capas locais PNG/JPEG/WebP via SAF, worker e caches limitados implementados. Banco migra v1/v2→v3 sem substituir linhas, favoritos, histórico ou caminhos. Núcleo, SRAM e protocolo de save state permanecem os mesmos. Ilustrações e cartazes de teste são originais; referência Delta usada apenas para análise visual.

Comandos desta revisão: Gradle x86 `:app:assembleDebug :app:assembleDebugAndroidTest -PtestAbi=x86`; AndroidJUnitRunner; host `cmake --build`/`ctest`; Gradle ARMv7 `:app:assembleDebug :app:testDebugUnitTest :app:lintDebug :app:assembleRelease`. Builds passaram, lint 0 erros, JUnit 7/7, CTest 3/3 e Android 4/4. Auditoria ARMv7 e assinatura debug v2 passaram. Capturas reais incluem Galeria, Minimal, detalhes, ajustes e pausa. Veja `visual-validation-0.2.0.md`; logs e screenshots acompanham `outputs/evidence/design-0.2.0/` na entrega. A TV de referência física continua pendente.


## Refinamento e perfis · 0.3.0-dev

Direção 0.2 mantida e refinada: proporções das capas, tipografia-base de 40 px no mesmo atlas, correção de alpha Android→SDL, formas com antialiasing, foco sem corte e rolagem de cinco capas com próxima parcial. Detalhes distinguem início/continuação; pausa identifica a sessão real. Estados vazios oferecem recuperação contextual. Nenhum asset de Delta/RetroArch foi copiado.

Editor de proporção/filtro/volume por jogo e console implementado, persistente e aplicado ao retomar. Demais opções avançadas do perfil (núcleo/frameskip/remapeamento por console) continuam pendentes. SRAM passou a validar família do núcleo e tamanho, preservando compatibilidade após recompilação; snapshots mantêm identidade exata do binário. Testes usam metadados antigos reais do envelope RTVSAVE2.

CTest 3/3, JUnit JVM 7/7, Android 4/4 e lint 0 erros/53 avisos. Navegação, tema Minimal, perfil herdado/reset, partida, Quick Resume e rejeição de snapshot antigo observados no Android TV virtual. APK debug ARMv7 pessoal reúne os cinco núcleos já compilados; auditoria ELF32/assinatura v2/permissões passou. Apenas NES executado; outros consoles e TV física continuam pendentes. Consulte visual-validation-0.3.0.md para comandos e limites. Projeto, SDK e build permanecem no disco D.

## Controle pelo celular · 0.4.0-dev
Ponte de metadados RFCOMM original implementada com opt-in, pareamento autenticado, encerramento no lifecycle e até dois clientes. Native envia contexto da seleção/sessão quando muda. O app RetroTV Pad usa HID padrão para input e escolhe layouts por console; o frontend/núcleos da TV não incorporam código Bluke/Delta.

Build x86 e ARMv7 debug pessoal passou; JUnit7/7, Android5/5 (incluindo protocolo UTF-8/escape/limites), lint sem erros e CTest3/3. Navegação D-pad até Ajustes > Controle pelo celular e ativação capturadas. Bluetooth entre aparelhos físicos permanece pendente. Ver docs/phone-controller.md. Biblioteca, saves e cores mantidos; não há novos jogos ou nova validação dos demais consoles.
## Home, busca e consoles isométricos · 0.5.0-dev
Home reorganizada em prateleiras verticais (Continuar, Consoles, Biblioteca); cabeçalho reduzido a Início, Favoritos, o console aberto, Buscar e Ajustes. As abas Continuar/Recentes saíram do cabeçalho (a lógica de coleção permanece). Rodapé de teclas removido da biblioteca, busca e detalhes; detalhes sem textos redundantes.

Busca com campo, teclado navegável por D-pad (letras, números, espaço, apagar, limpar) e resultados ao vivo; ignora acentos e prioriza títulos que começam pelo termo. OK abre detalhes e Voltar retorna à busca.

Ilustrações de console originais em `native/ui/iso.hpp`: projeção isométrica 30°, faces sólidas por orientação, um acento por figura, supersampling em textura alvo e cache limitado (6 MiB, liberado ao abrir jogo, pouca memória ou reset de alvo). Usadas no cartaz sem capa, prateleira de consoles, Minimal, estados vazios, Ajustes e pausa.

Executado: CTest de políticas no host com 13 novos cenários de home/busca; build x86 e AndroidJUnitRunner 5/5 no Android TV 11 virtual; navegação e capturas reais da home, busca, detalhes e coleção vazia. ARMv7, TV de referência e jogos dos demais núcleos continuam como nas versões anteriores.

## Controle pelo celular sem HID · 0.5.1-dev
A ponte do celular agora também recebe o controle, não só envia metadados: Bluetooth RFCOMM (aparelhos pareados) e Wi-Fi local (TCP 47800, anunciado por NSD `_retrotv._tcp.`). No Wi-Fi o celular é aceito após digitar o código de 4 dígitos que a TV mostra por 90 s no canto da tela; depois guarda um token aleatório e reconecta sem código. Linhas `I <botões> <dpad> <eixos>` alimentam um controle virtual SDL ("RetroTV Pad"), liberado quando o celular sai. Linhas `K` só mantêm a conexão (a TV derruba celular mudo após 15 s). Estados que chegam juntos esperam numa fila de até 4, para um toque rápido durar pelo menos um quadro.

Executado: JUnit JVM 10/10 (inclui fila de entrada), AndroidJUnitRunner 5/5 no Android TV 11 virtual, lint 0 erros, build ARMv7 pessoal. Ponta a ponta com dois emuladores (TV e "celular") via Wi-Fi: código exibido, código errado recusado, código certo aceito, reconexão por token sem código, conexão parada estável por mais de 37 s, toques rápidos de direcional movendo o foco da TV, primeiro toque após reconectar sem perda. RFCOMM entre aparelhos e TV de referência física continuam pendentes.

## Ajustes de desempenho do controle pelo celular · 0.5.2-dev
A TV só consulta a entrada do celular a cada quadro enquanto há um celular conectado (o estado é lido junto da atualização de 500 ms que já existia); antes a chamada JNI rodava todo quadro com a opção apenas ligada. `Platform::call` guarda os `jmethodID` já resolvidos (com trava, porque núcleos podem chamar a VFS de outra thread).

Executado: testes nativos de política no host, JUnit JVM 10/10, build x86 e ARMv7, lint 0 erros. Ponta a ponta com dois emuladores: novo pareamento por código, reconexão por token, conexão parada estável, toques rápidos e arrastos de analógico intercalados movendo o foco. Uma sequência logo após reiniciar o Pad perdeu três toques e não se repetiu em três tentativas seguintes.

## Refinos de interface · 0.5.3-dev
O app abre com o foco na ação principal ("Adicionar jogos" ou a primeira linha), não numa aba do cabeçalho. O cartão "Código do RetroTV Pad" passou para o canto superior direito, sob o cabeçalho, e não cobre mais a ilustração do console nem os botões.

Executado: testes nativos de política, JUnit JVM 10/10, lint 0 erros, build x86 e ARMv7; capturas no Android TV virtual da abertura com foco em "Adicionar jogos" e do cartão do código na posição nova; controle pelo celular conferido depois da atualização.

## Botão Início · 0.6.0-dev
Home/Guide (do celular ou de um controle físico) leva ao menu principal: fora de um jogo, volta a Início com o foco na ação principal; dentro de um jogo, pausa já em "Salvar e voltar à biblioteca", para um toque acidental não perder o progresso. O protocolo do celular aceita o bit 16 (Home) e continua recusando os bits 12–15.

Executado: testes nativos de política, JUnit JVM 10/10, lint 0 erros, build x86 e ARMv7. No Android TV virtual: Menu do celular abre Ajustes, Início volta a Início; no jogo, Início abre a pausa em "Salvar e voltar à biblioteca". Instalado na TV de referência real (0.6.0, abriu sem erro).

## Pasta interna de jogos e Retornar do celular · 0.6.1-dev
Na TV de referência, o seletor de pastas do Android (DocumentsUI) não mostra "Usar esta pasta" dentro de subpastas e bloqueia a raiz e Download; não havia como escolher uma pasta. O RetroTV agora também lê `files/Games`, a pasta privada do app (sem seletor nem permissão), ao voltar ao app e em "Atualizar biblioteca". Só jogos de cartucho; imagens de CD continuam pela pasta USB. Em build de depuração, jogos entram nela com `adb push` para `/data/local/tmp` e `run-as org.retrotv.lite cp`.

Celular: "Retornar" (bit 17 → botão MISC1 do controle virtual) volta um passo em qualquer tela; no jogo abre a pausa. Não fecha o RetroTV a partir do cabeçalho da home, para não derrubar a própria conexão.

Executado: testes nativos de política, JUnit JVM 10/10, lint 0 erros, build x86 e ARMv7. No Android TV virtual: Thwaite apareceu e abriu pela pasta interna; Retornar abriu e fechou a pausa. Na TV de referência real (0.6.1): os 7 jogos homebrew aparecem na biblioteca e Concentration Room abriu e rodou.

## Jogador 1 do celular · 0.6.2-dev
Na TV de referência, com o celular também conectado como controle remoto do Google TV, o serviço cria um dispositivo "virtual-remote" que se declara gamepad; ele virava jogador 1 e o celular (canal RetroTV) ficava como jogador 2, então os botões não agiam no jogo. Agora o controle do celular é sempre o jogador 1 e o "virtual-remote" só navega menus, sem ocupar vaga. Executado: testes nativos de política, JUnit JVM 10/10, lint 0 erros, builds; no emulador, o Start do celular avançou o Thwaite e Retornar voltou um passo. Instalado na TV de referência (0.6.2); teste com o moto g55 pendente.

## Setas seguradas e controle remoto como reserva · 0.6.3-dev
Teclas do controle remoto (o da TV ou o do Google TV) passam a valer enquanto estão seguradas durante o jogo; antes cada toque virava um pulso de um quadro, então segurar andava só um passo. O "virtual-remote" do Google TV volta a ocupar uma vaga livre de jogador como reserva; o celular pelo canal RetroTV continua sempre como jogador 1.

Executado: testes nativos de política, JUnit JVM 10/10, lint 0 erros, builds x86 e ARMv7. No Android TV virtual, na demo: toque rápido moveu 4 px e seta segurada (~0,5 s) moveu 52 px. Instalação na TV de referência pendente: a TV pediu nova autorização de ADB.

## Controle pelo navegador (iPhone) · 0.7.0-dev
A ponte do celular agora também serve, na porta 47801, uma página de controle que funciona em qualquer navegador (iPhone, iPad, Android), sem app. Ela fala o mesmo protocolo do RetroTV Pad por WebSocket, com o mesmo pareamento por código de 4 dígitos e token. Layouts NES, SNES, Mega, PS1, GB, Arcade e TV; AUTO, Início e Retornar; multitoque e escolha do botão mais próximo.

Executado: JUnit JVM 14/14 (4 novos: chave de aceite da RFC 6455, frames mascarados e recusas, linhas sobre frames com ping/pong, cabeçalho HTTP limitado), lint 0 erros, builds x86 e ARMv7. No Android TV virtual, pelo navegador embutido: página servida (200; 404 e 405 nos demais casos), código pedido e aceito, AUTO trocando para NES, setas movendo o foco, Início voltando à home, A abrindo o Thwaite, START passando a tela de título e Retornar abrindo a pausa. Na TV de referência real (0.7.0): página respondendo pela rede local em `http://192.168.1.112:47801`. Teste no iPhone pendente.

## Revisão de alinhamento da interface · 0.7.1-dev
- **Pôster:**
  - a faixa clara que sobrava sob o título foi removida (a metade de baixo já é pintada na cor final do degradê);
  - o título deixou de aparecer dentro do pôster, porque todas as telas já o mostram ao lado ou embaixo; com isso a arte ficou maior.
- **Home (Galeria):**
  - a fileira Biblioteca termina alinhada com a fileira Consoles (1196 px), em vez de passar da margem;
  - o conteúdo rolado esmaece sob o cabeçalho, em vez de ser cortado numa linha seca.
- **Detalhes:**
  - pôster na margem de 64 px;
  - o tempo jogado (ou "Ainda não jogado") preenche o vão sob o título;
  - sem partida salva, "Jogar agora" ocupa a largura toda, no lugar do botão morto "Sem partida salva".
- **Listas de ajustes:** os valores ("Minimal", "Ativo", "Por jogo / console") ficam alinhados à direita, junto da seta.
- **Minimal:** o nome do console e a contagem ficam centralizados na mesma caixa da arte.
- **Rodapé:** o status mostra os jogos da pasta interna, em vez de "Escolha sua biblioteca USB".

Executado: testes nativos de política, JUnit JVM 14/14, lint 0 erros, builds x86 e ARMv7, capturas antes e depois no Android TV virtual (Galeria e Minimal: home, biblioteca, detalhes, ajustes). Ainda não instalado na TV de referência.

## Capas dos jogos da pasta interna · 0.7.2-dev
A pasta interna (`files/Games`) já procurava uma imagem com o mesmo nome do jogo (`Thwaite.nes` → `Thwaite.png`, também jpg/jpeg/webp até 12 MB), mas o carregador de capas só aceitava `content://` (USB pelo seletor), então essas capas nunca apareciam. Agora `CoverRepository` aceita `file://` apenas dentro de `files/Games` (caminho canônico, sem sair da pasta); qualquer outro `file://` continua recusado.

Capas geradas a partir da tela de título de cada jogo, capturada no Android TV virtual: 480x640 (3:4), fundo com a própria tela desfocada e escurecida, tela nítida no centro e o nome do console no topo, como nos pôsteres sem capa. Ficam fora do projeto (`work/covers/capas`) e vão só para a pasta do app na TV. Sete jogos com capa; Break An Egg (Mega Drive) ficou sem, porque o núcleo picodrive compilado para x86 recusou a ROM no emulador.

Executado: JUnit JVM 14/14, lint 0 erros, builds x86 e ARMv7. No Android TV virtual: capas na Biblioteca, em Continuar e na tela de detalhes. Na TV de referência: capas copiadas para `files/Games`; 0.7.2 instalado em seguida e capas conferidas na Biblioteca e em Continuar. Na TV de referência, o Break An Egg também é recusado: o picodrive pede caminho de arquivo (`need_fullpath`) e recebe a URI do jogo; correção pendente.

## Mega Drive volta a abrir · 0.7.3-dev
O picodrive declara `need_fullpath` (abre o arquivo por conta própria), mas o RetroTV passava a ele a URI do jogo (`file://…` ou `content://…`), que ele não consegue abrir: todo jogo de Mega Drive dava "Núcleo recusou jogo". Agora, para núcleos que pedem caminho, o `Host::open` grava os bytes já lidos em `cache/content/content.<ext>` (extensão original tirada da URI, com reserva por console: `md`, `sms`) e passa esse caminho. NES e SNES continuam recebendo a ROM em memória; PlayStation e Arcade continuam no caminho por streaming, sem cópia.

Executado: testes nativos (host com QuickNES real + 3 casos de extensão, política), JUnit JVM 14/14, lint 0 erros, builds x86 e ARMv7. No Android TV virtual (picodrive x86 só para teste) e na TV de referência (0.7.3 instalada): Break An Egg abre até a tela de título. Com isso o Break An Egg ganhou capa, e os 8 jogos da pasta interna têm capa na TV de referência.
