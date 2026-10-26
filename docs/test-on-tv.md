# Roteiro de aceitação física

Todos os itens abaixo estão pendentes na TV. Emulador e testes no host não os substituem.

- [ ] Registrar ABI, API GLES, resolução, espaço livre e modelo com scripts/benchmark.ps1.
- [ ] Instalar APK ARMv7, abrir pelo launcher Leanback e navegar todos os fluxos com D-pad/OK/Voltar/Menu.
- [ ] Rodar demonstração original, mover quadrado e conferir áudio; Home Android/retorno sem travar ou perder foco.
- [ ] Parear 2 gamepads, verificar player 1/2, remapear, desligar/reconectar durante jogo e menu.
- [ ] Selecionar subpasta USB, indexar, reiniciar app, conferir favoritos e posição; revogar permissão e escolher novamente.
- [ ] Retirar/reinserir USB durante navegação, load e jogo; não apagar catálogo/saves e apresentar erro útil.
- [ ] Jogar 10+ min por core com arquivo legal autorizado; verificar áudio e entrada.
- [ ] Executar 10 ciclos abrir/sair, medir PSS antes/depois, carregar SRAM/state e comparar integridade.
- [ ] Exportar/restaurar ZIP, testar corrupção e conflitos somente em cópia descartável.
- [ ] PS1: BIOS correta, CUE+BIN, CHD, M3U, seek e troca de disco; verificar se core usa VFS em todos os acessos.
- [ ] Comparar Classic/Minimal/Performance; registrar frametime, PSS, espaço e estabilidade prolongada.
- [ ] N64 continua desabilitado até integração/medição concreta ARMv7/GLES2.

Não testar progresso importante antes de confirmar saves convencionais e estados em conjunto. Relatórios devem trazer arquivo/jogo e core/commit, versão do APK, data, comando, logs e resultado observado. Não incluir ROM/BIOS no relatório.
