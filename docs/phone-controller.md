# Controle pelo celular — 0.4.0-dev

RetroTV Pad é um APK separado para instalar no celular Android9+. O input é Bluetooth HID padrão, com descritor/report Android do Bluke no app do celular. RetroTV Lite continua com seu frontend original e host Libretro, sem código Bluke ou Delta vinculado aos núcleos.

Para trocar o layout automaticamente:
1. Atualize a TV para RetroTV Lite0.4 preservando os dados.
2. Ative Ajustes > Controle pelo celular.
3. No celular, abra RetroTV Pad, pareie por Bluetooth com a TV e conecte. Deixe AUTO ativo.
4. Selecione/abra um jogo. A TV informa o console; o celular escolhe a skin correspondente.
5. Para desativar, use a mesma opção dos Ajustes. A função fica desligada por padrão em uma instalação normal.

Canal de sincronização: RFCOMM seguro, apenas aparelhos pareados, até dois clientes e pacote JSON UTF-8 com version/system/title/mode. UUID 8a421b90-d104-4ba2-8c72-50a34191ce01. Há heartbeat de 1s; leitura da seleção no frontend só publica via JNI quando muda. Título limitado160 caracteres e linha2048 bytes. Não há input remoto, download, arquivos ou comandos de execução nesse protocolo. Sem rede Wi-Fi/INTERNET.

Servidor pertence ao lifecycle da Activity e fecha sockets em onStop/onDestroy. No Android12+ a permissão BLUETOOTH_CONNECT é solicitada somente ao ativar. No Android11 usa a permissão Bluetooth legada. Sem adaptador/permissão/serviço, a interface apresenta o erro e os controles USB/remoto continuam disponíveis.

Código original da ponte e protocolo sob MIT; não muda o licenciamento do frontend/núcleos. Skins e transporte do celular estão no projeto separado AGPLv3. O gamepad pode ser usado manualmente no RetroTV0.3 e em outros emuladores; AUTO exige esta ponte.

Verificado: build x86/ARMv7, 7 testes JVM, lint, 5 testes Android (incluindo metadata com UTF-8/escape/limites), 3 CTests e navegação do menu.
Pendente: Bluetooth entre celular/TV físicos, mapeamento SDL, canais HID/RFCOMM simultâneos, latência e reconexão na Philco. Nenhum teste virtual comprova esses itens.