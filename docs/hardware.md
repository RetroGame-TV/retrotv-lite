# Perfil alvo e limites

O RetroTV Lite mira TVs Android de entrada:
- Android TV 8.0+;
- `armeabi-v7a` (ARMv7 de 32 bits), 4 núcleos da classe Cortex-A53/A7;
- GPU OpenGL ES 2.0 (classe Mali-400/450/470);
- cerca de 1 GiB de RAM, dividido com o sistema da TV.

A TV de referência usada nos testes reais tem exatamente esse perfil (Android TV 11, Mali-470, 4 Cortex-A53, ~1 GiB).

App: UI lógica 1280x720, framebuffer de console limitado a 1024x1024, atlas de fonte ~3,5 MiB; somente uma sessão/core. ROMs simples limitadas a 32 MiB; discos usam VFS/seek e amostra de identidade de no máximo 128 KiB. Sem Vulkan, GLES3, WebView, Qt, Flutter ou serviços residentes.

Ambiente de validação disponível: Windows, Java 21, Gradle 8.9, Android SDK 35, NDK 26.3, CMake 3.22.1; Android TV 11 x86 virtual com WHPX/SwiftShader em 1280x720. Variante x86 só para teste (`-PtestAbi=x86`). APK entregue é ARMv7. Resultados de emulador não equivalem a uma TV real.
