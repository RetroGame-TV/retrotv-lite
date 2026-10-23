# Controles

Controle remoto: D-pad navega, OK abre, Voltar retorna, Menu abre configurações ou menu rápido. Android DPAD_CENTER mapeia SDL SELECT (diferente de Enter); ambos são tratados. Na demonstração, D-pad move o quadrado. Jogos completos normalmente precisam de gamepad; o menu rápido continua acessível pelo remoto.

Gamepad: A confirma, B retorna na interface. No jogo, mapa Libretro padrão: A/B/X/Y, ombros, gatilhos, SELECT/START, direcionais, cliques e analógicos com deadzone de 8.000. START+SELECT abre pausa independentemente do remapeamento. HOME continua reservado ao Android. Dois slots preservam P1/P2 ao desconectar; reconectar ocupa vaga livre. Core deve suportar dois jogadores; não implica multiplayer em qualquer jogo.

Configurações > Controles permite selecionar comando com D-pad, OK e pressionar botão do gamepad. Preferência salva por GUID/modelo. Gatilhos analógicos têm atribuição padrão; captura de remapeamento atualmente usa botões digitais. A configuração vale por modelo de controle, não por exemplar físico ou console. Perfis por console usam os defaults em ui/model.hpp; editor avançado por jogo ainda não foi feito.

Desconexão pausa a sessão e limpa entradas. Voltar/OK recupera o menu. Sem gamepad, emulação aceita D-pad e OK/A limitados; teclado de desenvolvimento: X/A, Z/B, Espaço/START, Tab/SELECT, Esc/menu.

Pendente: dois gamepads Bluetooth reais na Philco, layouts Android não padronizados, latência e reatribuição por dispositivo específico.
