# Licenças e distribuição

Código original do RetroTV Lite: GPL-2.0-or-later, conforme LICENSE. Nenhum código/tema/asset Pegasus foi reutilizado. A API Libretro (`libretro.h`) tem licença MIT preservada no cabeçalho.

| Dependência | Versão | Avisos no fonte | Empacotamento |
|---|---|---|---|
| SDL2 | 2.30.11 / fa24d868 | third_party/SDL/LICENSE.txt (zlib), demais notices do projeto | Padrão |
| QuickNES | 26bb785 | LICENSE GPL2, avisos por arquivo em nes_emu/ e libretro/ | Padrão; entregar fontes correspondentes |
| PCSX ReARMed | c881679 | COPYING GPL2, avisos dos arquivos, libchdr BSD, LZMA/public domain, zstd BSD e libretro-common | Build GPL opcional; fontes e dependências fixadas acompanham |
| Snes9x2005 | a79dfe9 | copyright: Snes9x pessoal/não comercial + componentes GPL/MIT | Binário não incluído no APK padrão |
| PicoDrive | 1890c29 | COPYING não comercial; demais cabeçalhos e submódulos | Binário não incluído no APK padrão |
| FBNeo CPS12 | 2b0c1c0 | src/license.txt: licença customizada, MAME legado e demais componentes | Binário não incluído no APK padrão |
| Demo NES original | gerada por scripts/generate-demo.mjs | assets/demo/LICENSE.txt (CC0) | Padrão, 24.592 bytes |
| Tipografia | fonte do dispositivo | Android Canvas / Typeface do sistema | Nenhum arquivo de fonte redistribuído |

As restrições não comerciais de Snes9x/PicoDrive/FBNeo exigem decisão de distribuição separada da base GPL. Compilar localmente não torna a combinação automaticamente distribuível. `-PrivateCores` e `-PprivateCores=true` preparam experimento pessoal; não são autorização para publicar ou vender esse APK. O pacote padrão não contém esses três binários.

Qualquer distribuição de APK deve acompanhar o código correspondente exato, este documento, LICENSE e os avisos das dependências. A fonte no ZIP e os gitlinks/patches identificam as revisões usadas. O subset FBNeo possui patch público de build e lista de drivers regenerada por ferramenta upstream; nenhuma lógica de emulação foi alterada.

Nenhuma BIOS, ROM comercial, imagem de capa protegida, credencial ou chave de assinatura release é incluída. O arquivo BIOS selecionado é armazenado somente no dispositivo do proprietário. Scraper/rede desabilitados e sem permissão INTERNET no app.

Lint mantém validações no código do app. Quatro arquivos Java SDL têm exceções específicas MissingPermission para rotinas opcionais de Steam BLE, captura de microfone e vibração que RetroTV não invoca; não se adicionaram permissões desnecessárias para calar esses avisos. Gamepads pareados são recebidos pelo sistema de input Android.

A entrega 0.3.0-armv7-pessoal é a variante privada: inclui também Snes9x2005, PicoDrive e FBNeo CPS12. Os avisos integrais principais acompanham este APK em Snes9x2005.txt, PicoDrive.txt e FBNeo.txt; os demais avisos por componente estão no ZIP de fontes correspondente. Uso pessoal de desenvolvimento; nenhuma autorização de publicação/comercialização é concedida por esta entrega.