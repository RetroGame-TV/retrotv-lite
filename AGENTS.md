# Instruções para Codex — RetroTV Lite

## Missão
Implemente integralmente o plano `RETROTV_LITE_PLANO_CODEX.md` por marcos pequenos, com código real, testes e documentação. A aplicação deve ser original e inspirada na experiência UX do Pegasus, sem copiar seu código ou assets. Principal alvo: Philco PTV40G71AGBL, Android TV 11, `armeabi-v7a`, Mali-470 / GLES2, ~1 GB de RAM.

## Regras obrigatórias
1. Leia primeiro o plano completo. Crie `docs/progress.md` e atualize em cada marco.
2. Faça commits pequenos e testáveis; implemente F0→F7, sem bloquear trabalhos independentes por não ter a TV.
3. Nunca afirme que houve build, teste físico, FPS, PSS ou APK gerado sem evidência. Liste o que realmente executou.
4. Compile para `armeabi-v7a`, OpenGL ES 2.0 e Android TV. Não usar Vulkan, GLES3 obrigatório, Qt/QML, WebView, Flutter ou serviços de fundo pesados no produto final.
5. Frontend nativo C++/SDL2; Kotlin para Android/SAF/lifecycle; host Libretro para jogos. Priorize menor RAM e baixo armazenamento.
6. Navegação completa por D-pad/OK/Voltar; menus recuperáveis por controle remoto. Gamepad com desconexão/reconexão e player 2 onde suportado.
7. Jogos somente do acervo legal do usuário. Não incluir ROMs comerciais, BIOS proprietárias ou imagens/capas protegidas. Testes usam homebrew e dados fictícios.
8. Cada core/biblioteca possui suas próprias licenças; documente antes de distribuição. Não copiar código/temas do Pegasus sem revisão jurídica e licença correspondente.
9. Use SAF Android 11, teste/planeje VFS ou ponte adequada para URI USB; nunca carregar CD de PS1 inteiro na RAM por padrão.
10. Recursos arriscados, como N64, devem ficar opcionais e não impedir release dos consoles comprovados.
11. Preserve SRAM/memory cards e save states separadamente, faça gravação segura e backup antes de sobrescrever.
12. A experiência é um APK único para os consoles principais; não substitua furtivamente a solução por um launcher que apenas abre apps externos.

## Saída a cada etapa
- Arquivos alterados e por quê
- Comandos de build/instalação/teste
- Testes executados, com resultados reais
- Status: implementado/testado, implementado/pendente de hardware, experimental ou bloqueado
- Riscos e próximo marco

## Conduta ante bloqueios
Se SDK/NDK, binários de core, compatibilidade de libretro ou Philco real não estiverem disponíveis: complete a parte independente, use mocks apenas onde apropriado, documente uma reprodução clara e não falsifique sucesso. Continue nas tarefas que não dependem desse bloqueio.
