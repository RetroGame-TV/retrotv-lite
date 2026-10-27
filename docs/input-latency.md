# Entrada mais recente sem apagar toques curtos

## O que mudou

O motor espera o instante do próximo quadro **antes** de consultar o telefone, processar os eventos SDL e coletar os botões/eixos para o núcleo. Antes, coletava a entrada e só depois esperava. A sequência agora é espera → telefone → eventos/atualização SDL → mapeamento dos dois jogadores → `retro_run`. Não há segunda consulta da fila no mesmo quadro. Lançar/retomar pelo menu continua podendo executar o primeiro quadro imediatamente; o menu não recebe a espera do jogo.

O mapeamento existente foi extraído para `native/input/frame_input.hpp`, usado pelo laço real e pelos testes. Mantém os dois jogadores, remapeamento, gatilhos e deadzone de 8.000. A consulta/atualização do SDL continua na thread do jogo. A ordem importa porque [valores de joystick virtual só passam a valer após atualizar o SDL](https://wiki.libsdl.org/SDL2/SDL_JoystickSetVirtualButton).

`PhoneInputQueue` deixa de guardar quatro pacotes completos e apagar o mais antigo. Agora separa:

- **Analógicos:** o valor mais recente dos quatro eixos, sem fila de movimentos anteriores.
- **Botões/direcional:** cada mudança completa do estado digital em FIFO, uma por consulta de quadro. Pressionar e soltar entre quadros ocupa duas posições; o toque curto não desaparece. Diagonais, combinações, Start/Select, Home e Voltar permanecem representados.
- **Repetições:** pacotes com o mesmo estado digital não ocupam posições novas. Uma direção segurada continua ativa após a fila esvaziar.

Os eixos de cada quadro são os mais recentes; não se promete reproduzir a posição histórica do analógico associada a uma transição digital anterior da mesma rajada. Sequências genuínas de botões/direcional podem precisar de mais de um quadro para serem consumidas: isso preserva os comandos e não deve ser apresentado como latência zero.

A fila admite 64 transições digitais pendentes. Em saturação, a ponte fecha explicitamente a conexão responsável e neutraliza o controle, em vez de continuar jogando com uma soltura descartada. Não há garantia de preservar uma sequência ilimitada mais rápida que o consumo dos quadros. Esse caso extremo é testado; movimentos sozinhos não causam saturação. Troca de dono/desconexão limpa a sessão antiga; o marcador `off` não apaga um estado já recebido da conexão substituta. Um pacote lido após parar a ponte não volta a ativar o controle.

Wi-Fi, Bluetooth RFCOMM e navegador usam a mesma fila. O protocolo de pareamento, máscaras e formato das linhas não mudam. A decodificação passa a produzir um estado tipado e a saída para C++ usa dígitos ASCII independentemente do idioma do Android.

## Testes executados em 27/10/2026

- **21 testes JVM**, incluindo sete novos: 10.000 movimentos, 12 toques completos em meio a rajadas de movimentos, direção/combinação segurada, saturação explícita sem evicção silenciosa, reset/reconexão, produtor/consumidor concorrentes e localidade árabe.
- **4 suítes CTest**, com SDL e QuickNES reais. O teste novo injeta a entrada durante uma espera simulada e lê o SDL depois dela, sem depender de tempo de parede. Confere todos os botões do mapeamento, solturas, diagonal, dois jogadores, analógicos, gatilhos, remapeamento e desconexão. Depois leva a leitura do SDL ao host Libretro/QuickNES e executa 148 quadros da demo NES original CC0: a imagem muda com o direcional pressionado e fica parada após soltar.
- **8 testes Android** em Android TV 11/API 30 x86 virtual: três novos de entrada, um de metadados e quatro existentes de armazenamento/migração. O receptor TCP real recebe 12 toques completos em meio a 1.200 atualizações de eixos; todos os 24 estados de pressionar/soltar são observados em ordem. Também cobre todos os bits de botão, Home/Voltar, diagonais/combinações, fechamento por saturação e conexão substituta.
- Compilação debug ARMv7 e x86/APK de testes; lint com zero erros e 51 avisos. Nenhuma regra foi desativada nesta mudança.
- **Dentro do APK x86 com SDL/QuickNES ativos:** pareamento por código e canal TCP real no Android TV virtual. Uma rajada de 12 toques direita+A, 1.200 atualizações de eixos e as 12 solturas deslocou o quadrado da demo de x=535 para x=580 na tela (45 pixels, equivalentes aos 12 passos NES esperados, com escala horizontal 3,75). Após mais um segundo conectado e sem novos comandos, a imagem permaneceu idêntica pixel a pixel. Não foi apenas uma simulação da fila; o teste atravessou receptor, JNI, controle virtual SDL, mapeamento, núcleo e renderer da janela.

| Antes da rajada | Após os 12 toques e solturas |
|---|---|
| ![Demo original antes da entrada](images/input-before.png) | ![Demo original após 12 passos](images/input-after.png) |

Os testes de movimento usam apenas a demo original em `assets/demo/retrotv-pad-test.nes`; nenhum jogo comercial ou BIOS é necessário. Os testes não medem latência física nem afirmam compatibilidade de Bluetooth real ou desempenho dos outros núcleos.

## Repetir

```sh
git submodule update --init --recursive
cmake -S native -B build/native -DRETROTV_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build/native --parallel 2
ctest --test-dir build/native --output-on-failure
./gradlew :app:testDebugUnitTest :app:lintDebug :app:assembleDebug
./gradlew :app:assembleDebug :app:assembleDebugAndroidTest -PtestAbi=x86
adb -s SERIAL_DO_EMULADOR install -r app/build-emulator/outputs/apk/debug/app-debug.apk
adb -s SERIAL_DO_EMULADOR install -r app/build-emulator/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb -s SERIAL_DO_EMULADOR shell am instrument -w -e class org.retrotv.lite.PhoneInputAndroidTest,org.retrotv.lite.PhoneMetadataTest,org.retrotv.lite.StorageIntegrationTest org.retrotv.lite.test/androidx.test.runner.AndroidJUnitRunner
```

## APK local de atualização e estados salvos

Quick Resume verifica a identidade do núcleo, inclusive seu binário. Para uma atualização local que só altera o motor/ponte, a opção explícita `-PpackagedQuicknes=true` permite usar o mesmo `libquicknes_libretro.so` da instalação anterior em `.tools/preserved-cores/armeabi-v7a/`. O arquivo deve vir de um build conhecido da mesma revisão fixada e ser conferido por hash. Ele não é versionado nem baixado pelo app. A opção só muda o empacotamento; sem ela, o build padrão/CI continua compilando QuickNES das fontes fixadas.

Os demais núcleos continuam nos diretórios locais já usados por `-PprivateCores=true`, sob os mesmos limites de licença. O APK pessoal não deve ser confundido com o artefato público do CI. A validação dos hashes e da assinatura do APK local fica no relatório de entrega. Nenhum catálogo, preferência, SRAM ou save state é alterado pelo código destas otimizações.

No APK ARMv7 local desta entrega, os cinco binários Libretro (QuickNES, PCSX ReARMed, Snes9x 2005, PicoDrive e FBNeo) foram comparados com o APK pessoal anterior e são idênticos byte a byte. O certificado de assinatura também é o mesmo. O núcleo NES só é reutilizado nesse build local explícito; o CI continua exercitando sua compilação das fontes.

## Limites

Pendente: teste de celular e TV físicos por Bluetooth RFCOMM, latência total toque → imagem da TV e sessão longa em cada console. Não foi instalada uma atualização em aparelho físico nesta alteração. Não há promessa de ganho em milissegundos nem benchmark de FPS no hardware Lite.
