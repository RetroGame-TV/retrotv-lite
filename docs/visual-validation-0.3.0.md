# Verificação da revisão 0.3.0-dev

Executada em 24/10/2026. Android TV 11 x86 virtual, SDL/GLES2, 1280×720. APK de destino compilado para ARMv7; nenhuma instalação na Philco real foi realizada.

## Mudanças verificadas

- Galeria com cinco capas maiores e próxima capa parcial, foco elevado sem corte, rolagem até o sexto item, fundo contextual, tipografia regular/negrito com alpha corrigido e formas com antialiasing.
- Detalhes com Jogar/Continuar lado a lado, opção de favoritos e acesso aos ajustes. Continuar recebe foco somente quando existe arquivo de snapshot; a compatibilidade é validada no carregamento.
- Ajustes e pausa com coluna de contexto, título do jogo e ações virtualizadas. Minimal monocromático e estado vazio com ilustração original.
- Coleção filtrada vazia permite Ver biblioteca. A coleção inicial vazia oferece seleção SAF e demonstração original.
- Perfis persistentes de proporção, filtro e volume por jogo/console. Alterado console NES para pixels nativos, suavização e 0%; o editor do jogo exibiu os valores herdados, e a partida iniciou. Restaurar padrões limpou apenas as preferências daquele escopo. Cálculo de escala/mute é coberto por teste nativo; não houve avaliação auditiva no emulador sem áudio.
- Save convencional com metadados da versão anterior lido pelo novo APK; snapshot do binário antigo recusado. Um snapshot da versão atual foi retomado, e salvar/sair retornou à biblioteca.

## Evidências automáticas

| Verificação | Resultado |
|---|---|
| CTest/LLVM-MinGW | 3/3: políticas de navegação/perfis/alpha; QuickNES real e saves; VFS esparso de 5 GiB |
| Host QuickNES | SRAM antiga da mesma família aceita após rebuild; snapshot de outro build e SRAM de outra família/tamanho recusados; CRC, backup e ciclos de sessão passaram |
| JUnit JVM | 7/7: catálogo, caminho virtual e backup |
| AndroidJUnitRunner | 4/4: provider SAF/capas, migrações SQLite e auxiliar de preview sem mutação na execução normal |
| Lint debug | 0 erros, 53 avisos, incluindo rotinas legadas SDL |
| APK ARMv7 pessoal debug | versionCode 3, 0.3.0-dev; oito bibliotecas ELF32 ARM, cinco núcleos no mesmo APK |
| Assinatura | v2 debug válida; mesma chave de desenvolvimento da 0.2, permite atualização preservando dados |
| Permissões | Nenhuma permissão INTERNET, armazenamento irrestrito ou serviço de fundo adicionado |

Comandos: gradlew.bat :app:assembleDebug :app:assembleDebugAndroidTest -PtestAbi=x86; adb -s emulator-5556 install -r; AndroidJUnitRunner; cmake --build e ctest --output-on-failure; Gradle ARMv7 :app:assembleDebug :app:testDebugUnitTest :app:lintDebug :app:assembleRelease -PprivateCores=true; audit-apk.ps1, apksigner verify --verbose e aapt dump permissions.

A primeira repetição dos testes do host precisou adicionar as DLLs LLVM-MinGW ao PATH. Três testes JVM inicialmente falharam porque o TEMP novo no D ainda não existia; criado o diretório, os sete passaram. Os logs finais registram a execução corrigida.

## Material visual e limites

Capturas reais do renderer em outputs/evidence/design-0.3.0/. Seis cartazes geométricos originais remetem ao mesmo homebrew CC0; o rodapé identifica a demonstração. Nenhum jogo, BIOS ou capa comercial foi adicionado. O preview foi removido do catálogo ao concluir a captura; saves foram preservados.

O APK pessoal reúne QuickNES, Snes9x2005, PicoDrive, FBNeo CPS12 e PCSX ReARMed. Só o NES foi executado; a presença dos demais binários não comprova seus jogos, BIOS ou desempenho. Esta variante tem licenças não comerciais/mistas e não é autorização de publicação/venda. Fontes e avisos acompanham a entrega.

Pendências: Philco/Mali-470, USB OEM físico, gamepads reais, jogos dos demais consoles, memória/FPS e distância de leitura. Game Boy não tem núcleo empacotado, N64 permanece desabilitado, Wii fora de escopo. Perfis de núcleo, frameskip e remapeamento por console ainda não foram implementados. A versão continua de desenvolvimento.
Acabamento final: adicionada margem ao atlas de formas para impedir linhas causadas pela interpolação entre regiões. Tema Minimal novamente capturado e inspecionado; builds x86 e ARMv7 debug/release repetidos com sucesso (refine-atlas-*.log). O release gerado permanece sem assinatura de distribuição e não é o APK indicado para instalar.
