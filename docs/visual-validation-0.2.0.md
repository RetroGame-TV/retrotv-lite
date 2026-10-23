# Verificação da revisão visual 0.2.0-dev

Executada em 23/10/2026. Android TV 11 x86, viewport 1280×720, SDL/OpenGL ES 2 via emulador. Nenhuma instalação ou medição na Philco real foi realizada.

| Verificação | Resultado |
|---|---|
| Build ARMv7 debug e release unsigned | Gradle BUILD SUCCESSFUL; versionCode 2 / 0.2.0-dev |
| Lint debug | 0 erros; 53 avisos, incluindo código SDL legado |
| Assinatura debug | apksigner: assinatura v2 válida |
| Auditoria APK | Cinco bibliotecas ELF32 ARM; somente armeabi-v7a; sem permissão INTERNET |
| JUnit JVM | 7 testes sem falhas: catálogo, caminho virtual e backup |
| CTest host | 3/3 suítes passaram: navegação/política, QuickNES real/saves e VFS esparso de 5 GiB |
| AndroidJUnitRunner | 4/4 métodos passaram: SAF/capa assíncrona/remoção/retorno, migração v1→v3, migração v2→v3 e auxiliar opcional de preview (sem mutação na execução normal) |
| Galeria | D-pad percorreu capas; OK abriu detalhes; fundo contextual e foco capturados no renderer |
| Favorito | Ação acionada em detalhes; metadado atualizado sem alterar o jogo |
| Partida/pausa | Homebrew original iniciado com OK; Menu abriu pausa; salvar e voltar à biblioteca executado pelo remoto |
| Ajustes/Minimal | Alternância por D-pad/OK observada; lista monocromática capturada |
| Estado vazio | Ação Testar controle iniciou o homebrew com OK; pausa e salvar/voltar executados; captura da biblioteca vazia refeita após inicialização |

As seis capas das capturas são cartazes geométricos originais de teste. Correspondem a entradas do mesmo programa homebrew CC0, não a seis jogos comerciais emulados. O preview é montado somente por teste explícito em catálogo vazio do emulador; não aparece em uma instalação normal. Não há ROM, BIOS ou capa comercial incluída.

Arquivos de evidência e APKs ficam ao lado do diretório do projeto, em `outputs/evidence/design-0.2.0/` e `outputs/RetroTV-Lite-0.2.0-*`. O pacote release continua sem assinatura de distribuição; usar o APK debug para instalar a versão de desenvolvimento.

Continuam pendentes: leitura do USB no provider OEM, distância real de visualização, limites de memória/FPS da Mali-470, gamepads físicos e validação dos demais consoles. A revisão não muda o status experimental desses caminhos.
