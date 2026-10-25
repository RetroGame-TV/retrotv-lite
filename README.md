# RetroTV Lite

App Android TV nativo C++17/SDL2/GLES2 + Kotlin/SAF, destinado à Philco PTV40G71AGBL (Android 11, **ARMv7 de 32 bits**). Interface original, offline e navegável por controle remoto. Versão de desenvolvimento **0.3.0-dev**; não é release 1.0 certificada.

Inclui biblioteca SQLite, pastas SAF, favoritos/recentes, Galeria com capas locais/Minimal monocromático, modo performance, host Libretro (frontend próprio, sem incorporar a interface do RetroArch), perfis persistentes de proporção/filtro/volume, menu rápido, SRAM, save states/Quick Resume condicionais, backup/restore validado e controles com dois slots/remapeamento. Demo NES original CC0 permite testar o fluxo sem obter ROM comercial. Para usar capas: `Nome do jogo.png` (ou jpg/jpeg/webp) ao lado da ROM com o mesmo nome base; atualizar biblioteca após mudanças. Veja [especificação visual](docs/ui-spec.md).

**Validação e limitações:** leia [docs/progress.md](docs/progress.md) e [matriz dos cores](docs/core-compatibility.md). O APK ARMv7 compila; demo NES foi executada no Windows e Android TV virtual **x86**. Não houve instalação na Philco, teste de Bluetooth/USB físico nem execução de jogos SNES/Mega Drive/Arcade/PS1. N64 está desabilitado; Wii está fora do escopo. Perfis de imagem/áudio por jogo e console estão disponíveis. Scraper, perfis de usuário e editor de núcleo/frameskip/remapeamento por console continuam pendentes.

## Compilar no Windows

Java 17+ (usado JDK 21), Gradle wrapper 8.9, AGP 8.7.3, Kotlin 2.0.21. Instale os componentes oficiais:

```powershell
git submodule update --init --recursive
sdkmanager 'platforms;android-35' 'build-tools;35.0.0' 'ndk;26.3.11579264' 'cmake;3.22.1' 'platform-tools'
$env:ANDROID_HOME = 'C:\Android\Sdk' # ajuste ao seu SDK
.\scripts\build.ps1 -Sdk $env:ANDROID_HOME -Release
```

No ZIP de fontes as dependências já acompanham o pacote; o primeiro comando é necessário para um clone Git, não para o ZIP. Caminho curto é recomendado no Windows para builds NDK. Não há ferramentas globais obrigatoriamente instaladas pelo projeto.

Saídas: `app/build/outputs/apk/debug/app-debug.apk` (assinado com chave **debug de desenvolvimento** gerada pelo Android); `app/build/outputs/apk/release/app-release-unsigned.apk` (sem assinatura release). Não há credenciais/chaves release no fonte. Gere e guarde sua chave fora do repositório para distribuição real.

QuickNES compila pelo CMake; PCSX ReARMed compila pelo script a partir da revisão fixa, sem download de .so em runtime. Para **experimento privado** dos cores com restrições de licença:

```powershell
.\scripts\build.ps1 -Sdk $env:ANDROID_HOME -PrivateCores
```

Essa variante reúne os cores principais no mesmo APK, mas **não está liberada para distribuição**: veja [licenças](docs/licenses.md). Não é um launcher de aplicativos externos.

Linux/CI: `pwsh ./scripts/build-cores.ps1 -Sdk "$ANDROID_HOME"` e `bash ./gradlew :app:testDebugUnitTest :app:lintDebug :app:assembleDebug`. O workflow em `.github/workflows/` está preparado, mas não foi executado no GitHub nesta entrega.

## Instalar e usar

Habilite depuração Android TV e conecte ADB pelo método suportado pela TV. Consulte `adb devices` e use explicitamente o serial desejado:

```powershell
.\scripts\adb-install.ps1 -Serial 'SERIAL_DA_TV' -Apk '.\app\build\outputs\apk\debug\app-debug.apk'
```

Abra RetroTV Lite pelo launcher. Use D-pad/OK/Voltar; Menu abre configurações. Primeiro teste: **Ajustes > Testar controle**. Depois escolha a subpasta USB contendo seu acervo legal. Para PS1, importe sua BIOS; formatos e VFS ainda dependem de validação. Nunca inclua jogos/BIOS sem direitos de uso.

Antes de usar progresso importante, teste save convencional, snapshot, exportação e restauração com arquivos descartáveis. [Manual de saves](docs/saves.md), [controles](docs/controls.md), [USB](docs/usb-storage.md), [testar na Philco](docs/test-on-philco.md) e [troubleshooting](docs/troubleshooting.md).

## Testes

```powershell
.\gradlew.bat :app:testDebugUnitTest :app:lintDebug
cmake -S native -B build/native -DRETROTV_TESTS=ON
cmake --build build/native
ctest --test-dir build/native --output-on-failure
```

São testes reais de QuickNES e políticas de navegação/saves/VFS. Testes Android adicionais usam provider sintético apenas no debug. Variante exclusiva para emulador: `-PtestAbi=x86` (saídas em `app/build-emulator/`; não entregar para a TV ARM).
