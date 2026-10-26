# RetroTV Lite — Especificação mestre para implementação com Codex

**Versão:** 2.0 — 23/10/2026  
**Status:** planejamento; nenhum APK funcional ou teste físico foi demonstrado neste documento.  
**Produto:** RetroTV Lite, aplicativo Android TV com experiência de console retrô e interface original inspirada na organização do Pegasus Frontend.  
**Dispositivo de referência:** TV de referência (perfil Lite) com Android TV 11.  
**Objetivo de entrega:** código-fonte reproduzível, documentação, testes e APK ARM32 quando o SDK/NDK estiver disponível. Não declarar compatibilidade física antes de instalar e executar na TV.

## 0. Instruções fundamentais para o Codex

Este documento é a fonte de verdade. O arquivo `AGENTS.md` define as regras de execução do agente. Ler ambos antes de editar. **Implementar o projeto inteiro por fases e manter progresso rastreável**, sem parar apenas para solicitar permissão para avançar. No entanto, nunca afirmar que uma fase está validada na TV de referência sem evidência do dispositivo real. Quando o ambiente não permitir compilar, executar ou conectar a TV, concluir as partes independentes, criar os testes/scripts correspondentes, documentar o bloqueio e continuar com as demais tarefas possíveis.

Não tentar realizar tudo em um único commit. Trabalhar em mudanças pequenas, com compilação e testes após cada mudança, e registrar o resultado real. Ao final, entregar um estado honesto das funcionalidades: `implementada + validada`, `implementada + não validada em hardware`, `experimental` ou `bloqueada`.

**Prioridades:** (1) jogos funcionando e controle confiável; (2) interface responsiva e legível; (3) biblioteca e saves robustos; (4) acabamento visual. Não comprometer o desempenho para obter efeitos visuais.

## 1. Escopo e requisitos de produto

### 1.1 Experiência principal

1. Abrir o RetroTV Lite no launcher da Android TV, em tela cheia e modo paisagem.
2. Navegar por controle remoto D-pad (cima, baixo, esquerda, direita, OK, Voltar); adicionar suporte a gamepad Bluetooth/USB.
3. Explorar a biblioteca com destaque para jogo selecionado, coleções por plataforma, favoritos, jogos recentes e continuar jogando.
4. Identificar e indexar jogos autorizados pelo usuário em uma pasta USB selecionada via mecanismo compatível com Android 11.
5. Pressionar `Jogar`: carregar o núcleo adequado, jogar, acessar menu rápido, salvar/carregar estado quando suportado e voltar exatamente à posição anterior da biblioteca.
6. Preservar saves de maneira confiável, oferecer backup/restore manual e lidar com desligamento, perda de foco, remoção de USB e controle desconectado.
7. Operar offline, sem conta obrigatória, telemetria invasiva nem dependência de servidor.

### 1.2 Sistemas

| Fase | Plataforma | Opção inicial a investigar | Política |
|---|---|---|---|
| Principal | NES / Game Boy / Master System | FCEUmm ou QuickNES / mGBA ou Gambatte / Gearsystem conforme core ARM32 disponível | Testar núcleo real por sistema; não prometer desempenho antes de medir |
| Principal | SNES | Snes9x 2005 (ou variante ARM32 compatível) | Sem filtros pesados |
| Principal | Mega Drive / Genesis | PicoDrive / Genesis Plus GX | Preferir performance estável |
| Principal | Arcade CPS1/CPS2 / Street Fighter | FinalBurn Neo ou core FB Alpha de versão compatível | Verificar ROM-set exigido e licenças; evitar alegar compatibilidade universal |
| Principal | PS1 (Tony Hawk, Tekken, etc.) | PCSX ReARMed | Software rendering, resolução nativa, BIOS fornecida pelo usuário quando necessária |
| Experimental | Nintendo 64 (Tony Hawk's Pro Skater) | Investigação de core/plugin ARMv7 + GLES2 comprovadamente compatível | Isolar em módulo; ocultar/desabilitar se não funcionar |
| Fora de escopo local | Wii / GameCube | Dolphin não é opção nesse Android 32 bits/GLES2 | Não instalar, portar nem prometer Vulkan para contornar restrição |

Não distribuir ROMs comerciais, BIOS proprietárias, capas protegidas ou arquivos obtidos sem autorização. Usar ROMs de demonstração homebrew licenciadas ou dados de teste próprios durante o desenvolvimento.

### 1.3 Funcionalidades — ordem de prioridade

**P0 / indispensável à versão 1.0:**
- Interface original baseada nos padrões UX de Pegasus: painel lateral, carrosséis/listas, foco claro, capa e informações do jogo, categorias e coleção.
- Controle remoto D-pad em 100% dos fluxos; suporte gamepad com detecção/recuperação após desconexão e ao menos 2 controles onde o core suportar.
- Importação e indexação incremental de biblioteca USB com acesso persistente e detecção de perda de permissão/pendrive.
- Catálogo persistente leve (`SQLite`) com plataforma, caminho/URI, favoritos, recentes, progresso e perfil do núcleo.
- Host Libretro para pelo menos um console simples, depois ampliar a SNES/Mega Drive/arcade/PS1.
- Menu rápido: retomar, pausar, reiniciar com confirmação, salvar/carregar estado, controles, proporção, sair.
- Quick Resume com **save state por jogo e compatibilidade verificada**; fallback para último jogo/sessão quando indisponível.
- Perfis por plataforma e por jogo para resolução, áudio, núcleo, frameskip opcional, controles e proporção 4:3.
- Salvar progresso convencional (SRAM/memory card), backup e recuperação sem sobrescrever inadvertidamente.
- Uma experiência unificada: nenhum salto manual entre vários apps para os consoles principais.

**P1 / depois do fluxo principal estável:**
- Modo performance/minimalista (sem fundos complexos, imagens reduzidas, transições instantâneas).
- Duas opções de tema original: Pegasus-inspired visual e Minimal, selecionáveis sem reiniciar.
- Suporte a PS1 `.chd`, `.cue + .bin` e `.m3u` (multi-CD), condicionado ao core e ao mecanismo real de leitura USB; troca de disco no menu rápido.
- Tela de diagnóstico com RAM/PSS, FPS/frametime, núcleo, tempo de carregamento e avisos de compatibilidade.
- Perfis de usuários locais opcionais (sem nuvem), se o orçamento de RAM/armazenamento permitir.

**P2 / opcional e DESLIGADO por padrão:**
- Scraper online de capas e metadados, com autorização do usuário, limites de rede/cache, atribuições e direitos de uso.
- Busca por texto com teclado da TV, coleções inteligentes e filtros adicionais.
- Pré-visualizações animadas leves somente se demonstrado que não impactam a interface; evitar vídeo de fundo.

## 2. Restrições de hardware conhecidas

Os dados foram relatados/obtidos em capturas do AIDA64 nas interações do proprietário; trate-os como perfil inicial, não como benchmark do aplicativo.

| Recurso | Valor observado / a confirmar em runtime |
|---|---|
| Fabricante / modelo | TV de referência (perfil Lite) |
| SO | Android TV 11 |
| SoC/plataforma | MStar/MediaTek `m5621`, conforme identificação anterior |
| CPU | 4 × ARM Cortex-A53 até ~1,3 GHz |
| Sistema/ABI | Android 32 bits; `armeabi-v7a`, ARM NEON |
| GPU | Mali-470 MP |
| API gráfica | OpenGL ES 2.0 (NÃO Vulkan, NÃO exigir GLES3) |
| Tamanho reportado de saída Android | 1280 × 720 / 60 Hz |
| RAM utilizável | ~907 MB; amostra pontual de ~273 MB disponível |
| Armazenamento livre | amostra pontual de ~1,37 GB |

**Regras inegociáveis:** compilar o APK de destino em `armeabi-v7a`; não exigir `arm64-v8a`, Vulkan, GLES 3+, shaders complexos ou GPU moderna; não requerer root, firmware alternativo ou trocar o launcher padrão da TV; não guardar mídias grandes e jogos no armazenamento interno como padrão. O conteúdo dinâmico deve se adaptar à memória livre medida, sem manter várias imagens texturizadas ou caches sem limite.

## 3. Decisão técnica e alternativas

### 3.1 Prova inicial com Pegasus 32 bits (benchmark opcional, sem dependência do projeto)

Antes de escrever o frontend completo, documentar um **experimento A/B de hardware** com o APK Pegasus Android 32 bits oficial (disponível em https://pegasus-frontend.org/), se a TV estiver disponível. Medir tempo de abertura, PSS, navegação D-pad e estabilidade com um tema básico. O Pegasus é GPLv3; analisar código apenas como referência e observar suas licenças. **Não reutilizar código/temas/assets do Pegasus** no app próprio sem decisão expressa de licenciamento e atribuição.

Se não houver acesso à TV, registrar o experimento como pendente e continuar. O experimento **não é um requisito bloqueante** para o código original.

### 3.2 Arquitetura definida para o produto

```text
retro-tv-lite/
├── app/ Android TV (Kotlin)
│   ├── AndroidManifest (Leanback, sem touchscreen obrigatório)
│   ├── Activity única / lifecycle (definir integração SDL2 documentada)
│   ├── UsbLibraryProvider (SAF / ContentResolver / DocumentFile)
│   ├── Preferences / permissões e indicadores do sistema
│   └── ponte JNI restrita e testada
├── native/ C++17 + SDL2 + CMake / NDK armeabi-v7a
│   ├── application/ estados e ciclo de vida
│   ├── ui/ componentes 720p e layout focado em TV
│   ├── graphics/ renderer GLES2, atlas e textura/cache LRU
│   ├── input/ mapeamento D-pad/gamepad e foco
│   ├── catalog/ modelos, indexação e banco SQLite ou wrapper Kotlin
│   ├── emulation/
│   │   ├── libretro_host/ callbacks environment/video/audio/input
│   │   ├── sessions/ GameSessionManager
│   │   ├── profiles/ perfis por sistema/jogo
│   │   ├── saves/ SRAM, memcard, estados e backup
│   │   └── storage/ ponte para conteúdo SAF e arquivos grandes
│   └── diagnostics/ FPS, frametime, memória, logging
├── cores/ código/configuração de build dos núcleos permitidos
├── assets/ ícones e conteúdo original de teste, com licenças
├── docs/
│   ├── architecture.md
│   ├── hardware.md
│   ├── ui-spec.md
│   ├── controls.md
│   ├── core-compatibility.md
│   ├── usb-storage.md
│   ├── benchmark.md
│   ├── licenses.md
│   └── test-on-tv.md
├── scripts/ build, adb-install, log, benchmark
└── .github/workflows/ build-and-test.yml
```

- Kotlin restrito a integração Android, ciclo de vida, SAF/USB e APIs de plataforma; o desenho principal é SDL2/C++ com renderização OpenGL ES 2.0.
- Implementar **uma estratégia de Activity SDL** coerente. Não duplicar ciclos de vida ou rotas de entrada entre Kotlin, SDLActivity e JNI; documentar a decisão e testes de pause/resume.
- O Libretro host gerencia somente um núcleo/jogo por vez. Para abrir um jogo, descarregar imagens, caches e elementos não utilizados do menu. Ao sair, descarregar o núcleo (se seguro) e restaurar a posição anterior na biblioteca.
- **Não** pressupor que qualquer core `.so` de download será carregável com `dlopen()` em qualquer pasta no Android. Empacotar componentes nativos compatíveis no APK e testar `nativeLibraryDir`, linker/ABI e dependências. Respeitar a licença de cada core.
- Para um PS1 ou ROM grande: verificar `need_fullpath`, VFS, formatos e seek aleatório. **Não** carregar imagem completa de CD na RAM. `content://` SAF pode não ser interpretado por kernels nativos; projetar ponte compatível com core, arquivo descritor/`lseek` quando viável ou camada VFS, com testes reais.
- Não adicionar serviços residentes, boot receiver, internet obrigatória nem permissões desnecessárias.

## 4. Especificação detalhada da UI (desenvolver em código, não gerar imagem)

### 4.1 Inspiração visual

Pegasus inspira a estrutura visual (jogo em evidência, galeria, categoria, navegação previsível), mas a identidade, os componentes e os assets devem ser próprios. Não copiar tema específico pixel a pixel.

**Layout de referência 1280 × 720, 16:9:**

```text
+------------------------------------------------------------------------------+
| RETROTV LITE         [PS1] [ARCADE] [SNES]      [controle] [configurações]     |
+-----------------+------------------------------------------------------------+
|  INÍCIO         |  DESTAQUE: TONY HAWK (dados do acervo do usuário)           |
|  CONTINUAR      |  título + plataforma + informação rápida    [ JOGAR ]      |
|  RECENTES       +------------------------------------------------------------+
|  FAVORITOS      |  CONTINUAR JOGANDO                                         |
|  ARCADE         |  [jogo foco]   [jogo]   [jogo]   [jogo]                      |
|  SNES           +------------------------------------------------------------+
|  MEGA DRIVE     |  COLEÇÕES / JOGOS                                          |
|  PS1            |  [Arcade] [SNES] [Mega Drive] [PlayStation] [Nintendo 64]   |
|  N64 (beta)     |                                                            |
+-----------------+------------------------------------------------------------+
|  A/OK Selecionar       B/Voltar        START Opções       Estado do USB       |
+------------------------------------------------------------------------------+
```

Tela 'Detalhes' traz capa, título, sistema, tempo jogado, favorito, botão Jogar/Continuar, estado de compatibilidade e opções. Tela 'Configurações' tem controles, pastas, temas, áudio, vídeo, BIOS, backup, diagnóstico e sobre/licenças. N64 aparece como `Experimental` apenas se habilitado em diagnóstico/perfil e houver implementação concreta.

### 4.2 Tokens e desempenho visual

- Fundo principal escuro, cartões sólidos/sem degradê complexo; no máximo 1 cor de acento; tipografia limpa (usar fonte livre com licença permitida, sem redistribuir fontes proprietárias).
- `design tokens` em estrutura local (`Theme` / JSON validado), incluindo cores, espaçamentos, tamanhos, foco, tipografia e densidade. Tema 'Classic' e 'Minimal' como presets originais.
- Área segura de TV (overscan): margens reservadas; textos acessíveis a distância; alto contraste; toda seleção deve ter estado de foco inequívoco.
- Transições opcionais de 80–160 ms e sem blur em tempo real; modo performance remove transições. Considerar UI a 30 FPS se permitir menos CPU sem prejudicar responsividade, com meta de 60 FPS em navegação se possível.
- Capas locais reduzidas, carregamento incremental, placeholders originais e cache LRU em limites baixos. Não carregar centenas de capas no início ou guardar frames de vídeo.
- Catálogo pode ter centenas de jogos sem travar a render thread; usar paginação/virtualização.
- Estados vazios, erro e recuperação desenhados desde o início: nenhum jogo, USB ausente, pendrive removido, biblioteca sem permissão, core indisponível, save incompatível, BIOS ausente, controle desconectado.

### 4.3 Comportamento e navegação

- Foco persistente por aba, rolagem horizontal por categorias, retorno contextual ao item antes selecionado e Voltar com comportamento previsível.
- Um controle remoto simples deve conseguir: abrir jogo, fechar/sair (via menu rápido possível), abrir Configurações, voltar, recuperar-se de erro. Jogos que exigem botões não presentes no controle remoto devem pedir gamepad sem prender o usuário em um modal irrecuperável.
- Gamepads: detecção de conexão, mapeamento e remapeamento por controle e console, deadzone/analógicos quando existirem, hotplug e preferência por player 1/2.
- Confirmar ações destrutivas (reiniciar sessão, excluir save, sobrescrever estado, restaurar backup).
- Persistir última aba/posição de seleção e preferências; reinício de aplicativo não pode perder biblioteca.

## 5. Infraestrutura de jogos e saves

### 5.1 Catálogo e importação USB

Usar `ACTION_OPEN_DOCUMENT_TREE` quando disponível, permissão persistente e escolha de subpasta, por exemplo `USB/RetroTV/Games`; o Android 11 pode bloquear seleção da raiz de certos volumes e Downloads. Tratar ausência de DocumentsUI/SAF da ROM OEM com diagnóstico e oferecer alternativa segura quando suportada. Indexar fora da render thread, em lotes, com cancelamento, deduplicação, detecção incremental e armazenamento de URI/identificadores estáveis quando possível. Não escanear diretorias de `Android/data` nem pedir acesso irrestrito por padrão.

**Layout sugerido no pendrive:**

```text
RetroTV/
  Games/
    NES/
    SNES/
    Genesis/
    Arcade/
    PS1/
    N64/
  Covers/        # opcional; somente conteúdo legal
  Backups/       # exportado explicitamente
```

Identificar ROM pelo sistema e extensão com detecção de ambiguidades; exibir erros úteis. Catálogo guarda metadados mínimos: ID estável, nome, plataforma, URI, núcleo, favorito, data de acesso, tempo jogado, save resumido, caminho de capa opcional e flags de compatibilidade. Não guardar jogos inteiros no banco.

### 5.2 PS1, CHD e multidisco

- Planejar `.cue/.bin` e `.chd`; `.m3u` para múltiplos discos se o core oferecer Disc Control.
- **CHD economiza espaço, não garante maior velocidade de leitura.** Testar carregamento e seek pela ponte SAF; ideal não decomprimir integralmente na RAM ou no armazenamento interno.
- BIOS é fornecida pelo usuário por fluxo próprio; alguns cores têm HLE limitado. Mostrar diagnóstico claro sobre ausência de BIOS e compatibilidade.
- Cuidado com controles analógicos, CD audio, cache de leitura, frame pacing e compilação NEON do PCSX ReARMed.

### 5.3 Saves, Quick Resume e proteção contra perda

- Separar: SRAM/memory cards (progresso nativo do jogo), save states (snapshot dependente de core/versão) e metadados de última sessão.
- Gravar atomicamente via arquivo temporário + rename/fsync quando possível; validar arquivos antes de aplicar restore; backup rotativo pequeno; nunca sobrescrever um save válido sem mecanismo de recuperação.
- Quick Resume: ao sair do jogo, oferecer salvamento de estado se suportado; ao reabrir, mostrar `Continuar` e `Iniciar normalmente`. Se core mudou ou state incompatível, informar e manter save original.
- Exibir claramente o risco específico de alguns cores PS1 de regravar memory cards ao carregar save states antigos; testar salvamento convencional e snapshots em conjunto.
- Exportar/importar saves manualmente no USB com nomenclatura estável e confirmação para conflitos; **não** prometer sincronização em nuvem.

### 5.4 Menu rápido dentro do jogo

Acesso por botão apropriado (ex.: START+SELECT se disponíveis) e alternativa recuperável pelo controle remoto; não depender exclusivamente de HOME do Android, que é reservado pelo sistema. Itens: `Continuar`, `Salvar estado`, `Carregar estado`, `Reiniciar`, `Controles`, `Proporção`, `Trocar disco` (PS1 suportado), `Sair para biblioteca`. Menu simples sem suspender threads indefinidamente.

## 6. Modo performance e diagnóstico

### 6.1 Orçamento inicial (hipóteses; medir na TV)

| Indicador | Meta de engenharia | Método / ressalva |
|---|---|---|
| PSS do frontend em idle | objetivo <80 MB, meta estendida <50 MB | `adb shell dumpsys meminfo <package>`; não se comprometer com resultado sem medir |
| Resolução da UI | 1280 × 720, escalável | validar com `wm size` e viewport real |
| Navegação | fluida, sem congelamentos perceptíveis | FPS, frametime p50/p95 e input latency estimada |
| Tempo de boot | reduzir ao essencial; meta inicial a definir por baseline | medição cold-start real |
| Emulação | áudio sem interrupções, frame pacing estável | 10+ minutos/teste por core e jogo |
| RAM após voltar ao menu | sem crescimento contínuo em 10 ciclos | PSS antes/depois, acompanhamento de leak |
| Armazenamento | ROMs e capas grandes no USB | espaço antes/depois da instalação |

O `modo performance` desativa animações, reduz cache/capas, remove efeitos, mantém tipografia legível e permite mudar para listas simples. Não usar efeitos com shaders, blur, vídeos de fundo, upscaling ou Vulkan. Renderizar console em sua resolução nativa, escalando por inteiro ou proporcionalmente para 720p. Testar aspect 4:3 com barras laterais; nunca distorcer automaticamente.

### 6.2 Diagnóstico e métricas

Registrar FPS do jogo, frametime, underruns de áudio se disponíveis, uso de RAM/PSS (quando consultável), tempo de scan USB, tempo de inicialização e falhas do core. Painel local sem upload automático. Incluir scripts de ADB para `logcat`, `dumpsys meminfo`, `top`, `getprop ro.product.cpu.abilist`, `getprop ro.opengles.version` e `dumpsys SurfaceFlinger`/`gfxinfo` quando aplicáveis.

## 7. Plano de implementação (todas as fases, com gates)

O Codex deve executar todas as fases **na ordem**, mas pode construir testes/mocks de partes futuras enquanto um teste em hardware estiver pendente. Cada fase: código + testes + documentação + status de aceitação.

### F0 — Repositório, documentação e build Android (P0)

- Criar Gradle, manifest TV com Leanback launcher, banner original, orientação landscape, `uses-feature touchscreen=false`, GLES 2.0, `abiFilters=armeabi-v7a`, CMake/NDK e integração SDL2.
- Determinar e fixar versões de build/SDK/NDK que compilam; documentar.
- Implementar app fullscreen com tela inicial mínima, foco e D-pad OK/Voltar; tratamento de `onPause`/`onResume` e eventos de sistema.
- Configurar testes unitários e CI, sem dependência de APK compilado somente no computador do desenvolvedor.

**Aceite:** projeto compila debug ARMv7 (quando SDK/NDK disponíveis), APK abre no launcher Android TV, controle funciona; se TV inacessível, marcar teste físico pendente.

### F1 — Prova do Pegasus e núcleo de UI (P0)

- Criar roteiro opcional para medir Pegasus oficial Android 32 bits na TV de referência como baseline separado, não como dependência de runtime.
- Implementar layout, componentes e tokens originais; Home/Recentes/Favoritos/Sistemas/Detalhes/Configurações; dados mockados criados no projeto, sem capas comerciais.
- Suporte a D-pad e gamepads, restauração de foco, estados vazios e modo minimalista.
- Adicionar testes de navegação/estado.

**Aceite:** todas as telas navegáveis sem touch; botão Voltar não trava e os elementos fora da tela não retêm grandes texturas.

### F2 — Catálogo/USB/temas (P0)

- SAF via Kotlin e JNI, persistência quando suportada, indexação incremental e cancelável, banco local pequeno; detectar remoção e reconexão USB.
- Biblioteca real, favoritos/recentes, busca local opcional, metadados e ícones originais.
- Dois temas originais, `Classic` e `Minimal`, com modo performance.

**Aceite:** catálogo reinicia sem rescan completo injustificado, não crasha na remoção de USB, memory budget medido quando possível.

### F3 — Emulação vertical slice (P0)

- Implementar Libretro host documentando ciclo de vida: `retro_init`, environment, input/video/audio callbacks, carregar conteúdo, `retro_run`, pause, save, `retro_unload_game`, `retro_deinit`.
- Rodar **um core pequeno de NES** com jogo homebrew autorizado. Validar audio sync, controles, 10 minutos de jogo, sair e voltar à posição anterior.
- Implementar `GameSessionManager`, perfis e tratamento de falhas.

**Aceite:** jogo inicia no próprio APK, sem alternar para app externo; retorno limpo, sem crescimento contínuo de RAM ou travamento conhecido.

### F4 — Consoles centrais, saves e controles (P0)

- Acrescentar SNES, Mega Drive, arcade CPS1/CPS2 e PS1 um sistema de cada vez; compilar/distribuir somente cores com licença atendida e ARM32 compatível.
- Criar mapeamento/remapeamento de controles e player 2; fluxo de save convencional e save states; Quick Resume; backup export/import e menu rápido.
- PS1: primeiro formato simplificado que o core lê; depois CHD e M3U/Disc Control; teste de leituras grandes por SAF e bridge.
- Criar matriz de compatibilidade documentada por core, versão, extensões, BIOS e perfil padrão.

**Aceite:** teste autorizado de cada console disponível, áudio/controle/save/volta ao menu, diagnóstico de falhas; nenhum jogo comercial no repositório.

### F5 — Nintendo 64 como spike isolado (P1, experimental)

- Investigar disponibilidade real de Mupen64Plus ou alternativa ARMv7 com renderizador **GLES2**, dependências, tamanho, licença e custo RAM.
- Se viável, integrar de forma opcional e testar Tony Hawk's Pro Skater N64 somente com dump legalmente obtido pelo proprietário.
- Se inviável, manter a plataforma marcada como `não suportado neste hardware` sem travar release; indicar jogar a versão PS1 como alternativa.

**Aceite:** incluir N64 somente mediante dados medidos; não escrever suporte "funcional" baseado em suposição.

### F6 — Endurecimento, UX e lançamento (P0/P1)

- Corrigir crashes, vazamentos, buffer de áudio, controle desconectado, perda de focus, baixo armazenamento, hotplug USB e arquivos corrompidos.
- Benchmarks e logs, backup de saves, manual do usuário, script ADB, troubleshooting e licença de cada dependência.
- CI em GitHub Actions que execute verificações, build APK quando ambiente permitir e publique artifacts.
- Criar `docs/release-checklist.md`, changelog, APK debug/release e instruções para sideload; não assinar com credenciais embutidas.

**Aceite:** build reproduzível, testes automáticos passam, limitações conhecidas documentadas e nenhuma afirmação de teste físico não realizado.

### F7 — Extras opcionais (P2)

- Scraper de metadados e capas somente por escolha explícita, respeitando direitos/licenças, com cache/limites.
- Personalização avançada, filtros de coleção, melhorias de acessibilidade e diagnósticos.
- Não iniciar extras antes do fluxo principal ser confiável.

## 8. Qualidade, testes e definição de pronto

### 8.1 Matriz de validação na TV de referência (manual)

- [ ] Instalar APK ARMv7 com ADB, abrir pela Android TV com o controle original.
- [ ] Percorrer todo o menu só com D-pad/OK/Voltar, sem pointer/mouse.
- [ ] Pausar app, abrir Home do Android, voltar e continuar sem perder foco.
- [ ] Conectar/desconectar controle Bluetooth durante a navegação e durante jogo.
- [ ] Selecionar subpasta USB, indexar ROMs legais, reiniciar e recuperar catálogo.
- [ ] Retirar/reinserir USB com app aberto sem crash ou perda da base.
- [ ] Jogar 10 minutos num core simples e voltar exatamente ao jogo selecionado.
- [ ] Testar 10 ciclos abrir jogo/sair e comparar PSS para detectar vazamento.
- [ ] Usar save convencional e estado; reiniciar app, carregar e verificar integridade.
- [ ] Exportar backup de save, restaurar com confirmação e conferir integridade.
- [ ] Testar PS1 em CHD (se suportado) sem copiar disco inteiro para RAM/flash.
- [ ] Testar dois gamepads e atribuição correta de jogador, onde aplicável.
- [ ] Perfil minimalista: comparar memória, tempo de resposta e estabilidade.
- [ ] Verificar armazenamento livre antes/depois, aquecimento e estabilidade após sessão prolongada.
- [ ] N64 somente se core ARM32 + GLES2 compilou e passou pelo teste; caso contrário, documentar ausência.

### 8.2 Testes automatizados

- Unit tests de foco/navegação, seleção de core, perfil por console, URI persistida, migração de banco, filtros e desduplicação.
- Tests com mocks do SAF para permissão negada, diretório vazio, desconexão, conteúdo ilegível e arquivos grandes.
- Mock/test doubles do Libretro host para transição Menu→Launching→Running→Pause→Exit→Menu, erro de core, erro de ROM.
- Testes de serialização e verificação de integridade de saves/backup (sem ROMs comerciais).
- Lint estático, build ARM32 em CI e scripts de captura de métricas.

### 8.3 Entregáveis de cada fase

1. Alterações em arquivos rastreáveis e commits pequenos.
2. Resumo do que foi implementado e o que ficou pendente.
3. Comandos exatos para compilar, instalar e testar.
4. Lista de testes efetivamente executados e resultados; distinguir testes automatizados de testes físicos.
5. Riscos/limitações e decisão técnica tomada.
6. Atualização de `docs/progress.md` com checklist de F0–F7 e status real.

## 9. Licenciamento, direitos e segurança

- Pegasus é GPLv3; sua UX geral pode inspirar um design original, mas copiar código, temas ou arte requer revisão de licença e atribuição. Não incorporar Pegasus como dependência do aplicativo sem plano explícito de compliance.
- Libretro é uma API; **cada core tem licença própria**. PCSX ReARMed é GPLv2; verificar combinação/licenciamento do host e demais componentes, distribuição de binários, código-fonte correspondente e avisos. Usar licenças compatíveis e documentar obrigações antes de distribuir.
- SDL2, bibliotecas e fontes: registrar versões, URLs, licença e avisos. Não usar fonte comercial sem direitos.
- Não embutir jogos, BIOS proprietárias, chaves nem credenciais no APK ou repositório.
- Não fazer download automático de código nativo executável, `.so` ou firmware de terceiros fora do sistema de build/conferência de licença.
- Não pedir root nem permissões de armazenamento indiscriminadas; nunca sobrescrever firmware da TV.
- Ser explícito: o projeto é para execução de arquivos fornecidos legalmente pelo usuário e não oferece pirataria de conteúdo.

## 10. Padrão de resposta e tomada de decisões do Codex

Ao começar: inspecionar repositório e ferramentas disponíveis. Criar `docs/progress.md` e checklist de milestones. Priorizar código de verdade (sem simular `build passed`, screenshots falsos, benchmarks inventados ou declarar APK sem arquivo).

No fim de cada fase: atualizar estado, rodar build/testes reais, anotar comandos, logs e caminhos dos artefatos. Se o hardware físico não estiver disponível, **seguir para o próximo trabalho independente**, mas manter marcados todos os critérios que exigem TV de referência real; nunca marcar a fase como validada no dispositivo.

As decisões que faltarem (ex.: versão do NDK, formato exato de tema, core da primeira plataforma) devem ser resolvidas com pesquisa das APIs oficiais e justificadas em `docs/architecture.md`, favorecendo simplicidade, suporte ARMv7/GLES2 e menor memória. Não interromper com perguntas triviais ou depender de imagens geradas. Caso uma incompatibilidade estrutural impossibilite determinada funcionalidade, registrar o bloqueio e oferecer uma implementação degradada explícita em vez de quebrar o aplicativo.

## 11. Referências técnicas

- Pegasus e downloads Android 32-bit: https://pegasus-frontend.org/
- Pegasus fonte/licença GPLv3: https://github.com/mmatyas/pegasus-frontend
- SDL2 Android: https://wiki.libsdl.org/SDL2/Android
- Android TV e controles: https://developer.android.com/training/tv/get-started/controllers
- Android SAF: https://developer.android.com/training/data-storage/shared/documents-files
- API Libretro: https://docs.libretro.com/development/cores/developing-cores/
- PCSX ReARMed: https://docs.libretro.com/library/pcsx_rearmed/
- Imagens de disco CHD/M3U: https://docs.libretro.com/guides/disc-images/
- Dolphin (fora do escopo): https://dolphin-emu.org/docs/faq/
