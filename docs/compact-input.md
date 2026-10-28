# Entrada compacta em RFCOMM e Wi-Fi

TV 0.7.5-dev e Pad 0.3.6-dev reduzem trabalho e tráfego no canal de jogo existente. Não dependem de Bluetooth HID: o moto g55 do usuário não oferece esse recurso. Não alteram protocolo de pareamento, UUID, portas, layouts ou identidades de save.

## Formato e compatibilidade

A TV acrescenta `inputBinary: 1` aos metadados JSON de versão 1. Só depois de receber essa capacidade o Pad usa o formato binário. O estado inicial continua podendo chegar em texto. Pad novo com TV antiga mantém texto; TV nova aceita ambos, intercalados no mesmo fluxo confiável. Navegador continua usando suas linhas habituais.

Cada estado completo ocupa **9 bytes**:

| Byte | Conteúdo |
|---|---|
| 0 | Marcador 0xF1 |
| 1–3 | Máscara de botões em little endian; mesmos bits 0–11, 16/Home e 17/Voltar |
| 4 | Direcional, quatro bits existentes |
| 5–8 | LX, LY, RX, RY, inteiros com sinal de -127 a 127 |

Texto ocupava de 14 a 31 bytes por estado válido; a redução de payload é de aproximadamente 36% a 71%. Isso não implica a mesma redução no tempo de rádio, que também tem cabeçalhos e agendamento. Um byte por ação isolada não representa todo o estado, combinações e quatro eixos, e exigiria outro mecanismo de ressincronização.

O leitor consome exatamente oito bytes depois do marcador, inclusive quando um eixo contém 0x0A/0x0D. Testes cobrem fragmentação de um byte por leitura, múltiplos pacotes na mesma leitura, texto/keepalive/binário intercalados, máscaras inválidas e truncamento. Pacote binário inválido encerra a conexão e neutraliza o controle. O buffer de leitura de 8 KiB continua: ele lê os bytes já disponíveis e não espera encher 8 KiB para entregá-los.

O Pad descarta estados iguais **após a mesma quantização usada no envio**, antes de acordar o escritor. Só estados equivalentes são descartados; mudanças digitais continuam em FIFO. A TV não aloca objetos Digital para cada movimento de eixo e mantém a política de uma transição por quadro, sem fundir pressionar/soltar.

## Ponte até o motor

O caminho usado pelo aplicativo agora passa um `long` por JNI em vez de criar uma string, copiá-la e interpretar seis números com sscanf a cada quadro. Zero significa ausência de entrada; -1 significa desconexão; bit 54 indica estado presente. Bits 0–17 carregam botões, 18–21 direcional e 22–53 os quatro eixos em bytes com sinal. Um neutro presente não se confunde com ausência de controle. A API de texto permanece para compatibilidade/testes, mas não participa do loop do motor atualizado.

O mapeamento SDL, deadzones, gatilhos e dois jogadores continuam no caminho existente. O teste C++ verifica todos os bits de botão, 16 estados de direcional, 255 valores de eixo, neutro, desconexão e dados inválidos.

## Tráfego de retorno e falhas

Metadados são enviados imediatamente ao conectar e quando o contexto realmente muda, com notificação entre threads; não há mais repetição de JSON a cada segundo. O keepalive do Pad continua a cada cinco segundos. O teste Android observa ausência de repetição e envio de um novo contexto ao publicar SNES.

Quando a fila digital satura, a recepção agora para imediatamente. Antes, bytes já presentes no buffer poderiam continuar sendo processados após fechar a conexão, reativando um estado antigo. O teste envia 120 transições, observa fechamento e neutralização, reconecta e confere que o controle novo não recebe a cauda antiga. Saturação continua sendo uma falha explícita, não uma promessa de armazenar comandos ilimitados.

## Sugestões de transporte

`flush()` já existia a cada envio e continua presente. Nagle é uma opção de **TCP**, já desabilitada no Wi-Fi com TCP_NODELAY; não é uma configuração RFCOMM. [Java Socket](https://docs.oracle.com/en/java/javase/21/docs/api/java.base/java/net/Socket.html#setTcpNoDelay(boolean)). A prioridade de conexão de BluetoothGatt pertence ao **BLE**, não ao socket RFCOMM clássico. [Android BluetoothGatt](https://developer.android.com/reference/android/bluetooth/BluetoothGatt#requestConnectionPriority(int)). Não se aplicam ajustes de COM do Windows ou Serial.readString a esta ponte Android–Android.

Bluetooth e Wi-Fi usam o novo formato, como rotas alternativas. BLE não foi introduzido. Usar duas rotas simultaneamente exigiria identidade de sessão, sequência comum, deduplicação e regras para pacotes atrasados/desconexão. Não basta copiar o mesmo comando nas duas conexões: o receptor atual possui um dono ativo. Tal modo não está habilitado nesta entrega e não há alegação de que duplicar tráfego reduziria a latência no rádio compartilhado.

## Validação local

- **26 testes JVM na TV e 47 no Pad**. Incluem dez mil estados binários equivalentes às linhas anteriores e dez mil pequenas variações de analógico que não geram novo envio, seguidas de 12 toques completos preservados.
- **6 CTests** aprovados, com a nova ponte compacta, SDL/QuickNES reais e as proteções de saves existentes.
- **11 testes Android da TV**: recepção compacta real com 12 toques/24 transições entre 12 mil atualizações de eixos, saturação/reconexão, formato legado, metadados e armazenamento.
- **24 testes do runner Android do Pad**: inclui negociação por TCP real, todos os botões, direções, Home/Voltar, solturas, multitoque e regressões de AUTO/controles. O teste opcional de skin Delta retorna sem carregar uma referência quando ela não foi fornecida.
- **1 teste adicional entre os dois APKs**, com Street Fighter II do usuário ativo no Android TV 11 x86 virtual: RetroTvLink real do Pad → TCP local → parser/queue → JNI compacta → SDL → núcleo SNES LAGFIX/renderer. Envia seis direções/diagonais, seis botões, sequência de quarto de círculo e rajada de 12 mil movimentos; conexão permanece ativa. Capturas mostram a partida antes/depois. Não é contagem estatística de golpes da luta, pois há CPU adversária e animações.
- Atualização `install -r` preservou os sete arquivos de save do cenário de teste por SHA-256. Os seis binários de núcleo do APK pessoal anterior são mantidos; não houve alteração em SaveStore, SRAM, estados ou compatibilidade do SNES. Instalar por cima com o mesmo certificado, sem desinstalar ou limpar dados.

Microbenchmarks informativos em Android virtual, mediana de sete rodadas de 5.000 conversões/consultas após aquecimento:

| Custo de CPU por estado | Texto anterior | Compacto |
|---|---:|---:|
| Pad: serialização até bytes | 13.067 ns | 589 ns |
| TV: leitura/decodificação do pacote | 10.108 ns | 639 ns |
| TV: consulta da fila para JNI | 2.148 ns | 105 ns |

Sem limiar de tempo nos testes. Esses custos não medem rádio, JNI nativa completa, compositor ou latência de tela e variam conforme carga/JIT/GC. Não houve teste de RFCOMM físico entre moto g55 e Philco nem medição toque→imagem. Não atribuir ganho adicional de milissegundos no Street Fighter ao transporte com base nesses microbenchmarks. O ganho de um quadro no núcleo SNES é o teste separado da entrega 0.7.4, preservado aqui.

## Repetir

```sh
./gradlew :app:testDebugUnitTest :app:lintDebug :app:assembleDebug
./gradlew :app:assembleDebug :app:assembleDebugAndroidTest -PtestAbi=x86 -PprivateCores=true
adb -s SERIAL_DO_EMULADOR install -r app/build-emulator/outputs/apk/debug/app-debug.apk
adb -s SERIAL_DO_EMULADOR install -r app/build-emulator/outputs/apk/androidTest/debug/app-debug-androidTest.apk
adb -s SERIAL_DO_EMULADOR shell am instrument -w -e class org.retrotv.lite.CompactInputAndroidTest,org.retrotv.lite.PhoneInputAndroidTest,org.retrotv.lite.PhoneMetadataTest,org.retrotv.lite.StorageIntegrationTest org.retrotv.lite.test/androidx.test.runner.AndroidJUnitRunner
```

No Pad, rodar `:retropad:testDebugUnitTest :retropad:assembleDebug :retropad:assembleDebugAndroidTest :retropad:lintDebug` e o runner normal. `LiveTvInputTest` é explicitamente opt-in (`liveTv=1`, `tvHost`, `tvToken`), exige jogo SNES ativo e autorização de teste já preparada na TV virtual. Não publicar token real nem ROM/estado do jogo. APK pessoal privado não é artefato público do CI.
