# F5: Nintendo 64 isolado e desabilitado

Pesquisa em 2026-10-23: [Mupen64Plus-Next](https://docs.libretro.com/library/mupen64plus/), [fontes oficiais](https://github.com/libretro/mupen64plus-libretro-nx) e [ParaLLEl N64](https://github.com/libretro/parallel-n64). Existem variantes e plugins gráficos diferentes; a existência do core não comprova um caminho ARMv7/GLES2 adequado à Mali-470.

O host atual aceita somente frames de software. Recusa SET_HW_RENDER com diagnóstico, sem fingir suporte a contexto GL do core. Para viabilizar N64 seriam necessários: versão/plugin GLES2 fixados; contexto HW Libretro compartilhado e restauração de estado SDL; APIs FBO/proc resolver; compilação ARMv7; medição PSS/áudio/frame pacing na Philco e teste de dump legal do proprietário. Essas evidências não estão disponíveis.

Decisão: sem core N64, opção oculta, extensões N64 não importadas, teste de política impede habilitação acidental. Isto não afirma impossibilidade absoluta do hardware; informa ausência de suporte comprovado nesta implementação. A versão PS1 é candidata alternativa, também dependente de validação real. Wii/GameCube seguem fora de escopo local.
