# Controle pelo celular — 0.7.0-dev

RetroTV Pad é um APK separado para celular Android 9+. Ele chega à TV por um destes caminhos, nesta ordem:

| Caminho | Quando usar | O que controla |
|---|---|---|
| Bluetooth HID | celular que oferece o perfil HID Device (muitos fabricantes desligam) | qualquer app da TV |
| Canal RetroTV (Bluetooth RFCOMM ou Wi-Fi) | qualquer celular, com RetroTV Lite aberto | jogos e menus do RetroTV |
| Controle pelo navegador (Wi-Fi) | iPhone, iPad ou qualquer celular, sem instalar app | jogos e menus do RetroTV |
| Controle remoto Google TV (Wi-Fi) | navegar fora do RetroTV | direcional, OK, Voltar, Menu |

RetroTV Lite continua com frontend original e host Libretro, sem código Bluke ou Delta vinculado aos núcleos.

Uso:
1. Na TV, ative Ajustes > Controle pelo celular. A TV mostra o endereço Wi-Fi e passa a aceitar o Pad.
2. No celular, abra RetroTV Pad > Conectar e escolha a TV (achada no Wi-Fi, pareada por Bluetooth ou por endereço).
3. Pelo Wi-Fi, digite no celular o código de 4 dígitos que aparece no canto da TV. Só na primeira vez; depois o celular guarda um token.
4. Com AUTO ativo, o celular troca o layout conforme o console aberto na TV.
5. Para desativar, use a mesma opção dos Ajustes. A função vem desligada.

Protocolo (linhas de texto, até 256 bytes no sentido celular → TV):
- TV → celular, a cada 1 s: JSON UTF-8 `{"version":1,"system","title","mode"}`.
- Celular → TV: `I <botões hex> <direcional> <lx> <ly> <rx> <ry>` (eixos −127..127; botões nos bits 0–11, Home no bit 16 (Guide do controle virtual) e Retornar no bit 17 (MISC1)); `K` só mantém a conexão.
- Wi-Fi, antes de tudo: `H <id do celular> <token|->`; a TV responde `OK`, ou `C` e espera `P <código>` (3 tentativas, 90 s), respondendo `T <token>` ou `R`.

Bluetooth: RFCOMM seguro, apenas aparelhos pareados, UUID 8a421b90-d104-4ba2-8c72-50a34191ce01. Wi-Fi: TCP 47800 na rede local, anunciado por NSD `_retrotv._tcp.`; até 3 celulares, até 8 tokens guardados; celular sem enviar nada por 15 s é desconectado. A entrada vira um controle virtual SDL "RetroTV Pad", liberado quando o celular sai. Não há download, arquivos nem comandos de execução no protocolo.

Servidor pertence ao lifecycle da Activity e fecha sockets em onStop/onDestroy. Permissões: INTERNET e ACCESS_NETWORK_STATE para o Wi-Fi; BLUETOOTH_CONNECT pedido só ao ativar (Android 12+). Sem Bluetooth, o Wi-Fi continua funcionando.

Código original da ponte e protocolo sob MIT; não muda o licenciamento do frontend/núcleos. Skins e transporte do celular estão no projeto separado AGPLv3.

Verificado (0.5.1): 10 testes JVM, 5 testes Android, lint, build x86/ARMv7 e ponta a ponta por Wi-Fi entre dois emuladores (código, recusa, token, conexão parada estável, toques rápidos movendo o foco).
Pendente: RFCOMM entre aparelhos físicos, latência e reconexão na TV de referência, Wi-Fi em rede real com isolamento de clientes.

Controle pelo navegador (0.7.0): com "Controle pelo celular" ativo, o RetroTV também atende em `http://<IP da TV>:47801` (o endereço aparece ao ativar a opção). A página (`app/src/main/assets/web/pad.html`, um arquivo só, sem recursos externos) tem os layouts NES, SNES, Mega, PS1, GB, Arcade e TV, AUTO, Início e Retornar. Conversa por WebSocket em `/ws` com as mesmas linhas do canal TCP (`H`/`C`/`P`/`T`/`OK`, `I`, `K`) e o mesmo pareamento por código e token; o token fica no armazenamento do navegador. Implementação mínima de WebSocket (RFC 6455) em `WebPad.kt`: só frames de texto mascarados, sem fragmentação, até 1 KiB; ping respondido; cabeçalho HTTP limitado a 8 KiB; só GET. No iPhone: abrir o endereço no Safari e usar "Adicionar à Tela de Início" para abrir em tela cheia.
