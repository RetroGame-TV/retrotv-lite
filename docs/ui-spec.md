# Interface original · 0.3.0

Direção revisada a pedido do usuário: biblioteca centrada nas capas, inspirada na organização e sobriedade do [Delta](https://github.com/rileytestut/Delta) e em seu [press kit oficial](https://faq.deltaemulator.com/about/press-kit). As duas referências fornecidas também orientaram o carrossel e o modo monocromático. Nenhum código, screenshot, fonte, marca de console ou asset desses produtos foi incorporado ao aplicativo.

1280×720 lógico, margens horizontais de 54–60 px. Fundo carvão, texto branco, acento violeta e alvos arredondados. Barra superior reúne coleções e sistemas. A capa selecionada sobe, ganha contorno contrastante e um símbolo de reprodução; a mesma textura recebe tratamento escurecido no fundo. A rolagem interpola a posição apenas no modo Galeria. Menu de detalhes mantém a capa visível e separa ações. Ajustes e pausa usam uma coluna de contexto e outra de ações.

Minimal troca a galeria por lista monocromática e ilustração original de console/controle. Não carrega imagens nem anima o carrossel; Performance usa essa mesma apresentação econômica. O estado vazio contém duas ações reais: selecionar a pasta SAF e executar o teste original de controle. Não há títulos comerciais fictícios apresentados como biblioteca instalada.

## Navegação

- Barra superior: esquerda/direita muda coleção; baixo ou OK entra no conteúdo. Menu abre ajustes em qualquer coleção.
- Galeria: esquerda/direita percorre capas; cima ou Voltar retorna à barra; OK abre o jogo.
- Minimal: cima/baixo percorre os jogos; cima no primeiro item, esquerda ou Voltar retorna à barra; OK abre o jogo.
- Ajustes e pausa: cima/baixo e OK. Detalhes: esquerda/direita entre Jogar e Continuar (quando há snapshot); baixo alcança favoritos, imagem/áudio e retorno. Voltar recupera o contexto.
- A posição de cada coleção, favoritos, saves e histórico permanecem preservados.

## Capas locais e limites

Na mesma pasta: `Meu jogo.nes` + `Meu jogo.png` (também jpg/jpeg/webp, extensão e nome sem distinção de maiúsculas). A leitura SAF associa o nome base ao jogo; não baixa imagens e não abre ROMs para procurar arte. Após alterar a capa, atualizar a biblioteca.

Decodificação em worker único, amostragem máxima aproximada de 512 px, saída fixa 240×320 RGBA com proporção preservada. Limites: arquivo até 12 MiB, imagem até 32 milhões de pixels, oito tarefas pendentes, oito buffers prontos (~2,34 MiB), seis texturas de capa (~1,76 MiB). Erros limitados a 128 URIs e invalidados no próximo reload do catálogo. Imagens inválidas ou indisponíveis usam cartaz original com ícone vetorial. Minimal/Performance liberam as texturas; evento de pouca memória também limpa o cache.

Atlas de fonte Android regular/negrito: 1024×1792 RGBA (~7 MiB), glifos-base de 40 px e alpha corrigido para SDL, criado uma vez, sem arquivo de fonte redistribuído. Renderização SDL2/OpenGL ES 2; sem WebView, vídeo de fundo, shader novo ou dependência de rede.

## Evidência visual

Capturas da revisão são produzidas pelo renderer do APK em Android TV 11 x86 virtual. Para mostrar uma biblioteca preenchida, um teste opcional protegido por argumento `preview=1` cria seis cartazes geométricos originais associados ao mesmo homebrew de controle CC0. O rodapé identifica esse acervo como demonstração. O teste recusa catálogos com jogos preexistentes. Esses cartazes não são ROMs comerciais e o catálogo de preview não acompanha o APK instalado normalmente.

Legibilidade a distância, fluidez e consumo na Philco real ainda precisam de validação física.

## Refinamento 0.3

Cinco capas de 195×260 px e a próxima parcialmente visível, foco elevado em 120 ms, contorno completo e rolagem sem salto na troca da janela. Bordas e ícones usam um pequeno atlas de formas com antialiasing (64×128 RGBA, 32 KiB); o orçamento do atlas de fonte permanece igual. Capas são normalizadas com canto transparente e proporção preservada. Textos truncados reservam espaço para reticências.

Detalhes têm ações primárias lado a lado; Continuar fica desabilitado sem arquivo de snapshot. A presença do arquivo não garante compatibilidade: o carregador valida conteúdo, versão/hash e CRC antes de restaurar. Pausa identifica o jogo e reaproveita o frame já existente como fundo discreto. Coleção filtrada vazia permite voltar ao acervo sem abrir o seletor de pastas.

Imagem e áudio: escopo jogo/console, proporção 4:3/pixels nativos/tela cheia, pixels nítidos/suave e volume 0–100% em passos de 25. Primeiro aplica defaults, depois console, depois sobrescritas do jogo. Restaurar padrões remove as sobrescritas somente do escopo selecionado; jogo volta a herdar console. Ajustes em sessão pausada valem ao retomar. O filtro é interpolação SDL; não há shaders.