# Arquitetura e decisões

2026-10-23. Código original. Não incorpora código nem tema Pegasus.

- Activity única deriva de SDLActivity 2.30.11. SDL possui a surface, loop e eventos de lifecycle/input. Kotlin cuida apenas de SAF, SQLite, preferências e diagnósticos; JNI cruza com strings/descritores limitados. Nenhum segundo loop Android.
- SDL_Renderer usa exclusivamente o backend opengles2 em 1280x720 lógico. Android armeabi-v7a, NEON, minSdk 26, target 32 (APK de sideload Android 11; publicação Play exigiria política de target atual). API 26 permite rename atômico com java.nio.file e offsets grandes no VFS.
- Versões escolhidas: Gradle 8.9, AGP 8.7.3, Kotlin 2.0.21, compileSdk/build-tools 35, NDK 26.3.11579264, CMake 3.22.1, Java 17 bytecode (JDK 21 no host). Resultado de compilação deve constar em progress.md, não é inferido destas escolhas.
- Uma instância Libretro por sessão, dlopen apenas de core empacotado em nativeLibraryDir. Memória de ROM limitada; CD nunca passa pelo carregador de ROM em RAM.
- SRAM e snapshots têm arquivos distintos, checksum e identidade de conteúdo/core. Metadados SQLite e preferências Android sobrevivem ao reinício.
- Sem internet no manifest, contas, ROMs comerciais, BIOS, serviços residentes ou launchers externos.

Referências verificadas: [SDL Android](https://github.com/libsdl-org/SDL/blob/release-2.30.11/docs/README-android.md), [AGP 8.7](https://developer.android.com/build/releases/agp-8-7-0-release-notes), [SAF](https://developer.android.com/training/data-storage/shared/documents-files), [Libretro](https://docs.libretro.com/development/cores/developing-cores/).
