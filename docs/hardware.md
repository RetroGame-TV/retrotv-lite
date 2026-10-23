# Perfil alvo e limites

Philco PTV40G71AGBL, Android TV 11, armeabi-v7a, Mali-470/GLES2, 4 Cortex-A53, aproximadamente 1 GiB de RAM. Dados fornecidos pelo plano; não houve conexão à TV neste desenvolvimento.

App: UI lógica 1280x720, framebuffer de console limitado a 1024x1024, atlas de fonte ~3,5 MiB; somente uma sessão/core. ROMs simples limitadas a 32 MiB; discos usam VFS/seek e amostra de identidade de no máximo 128 KiB. Sem Vulkan, GLES3, WebView, Qt, Flutter ou serviços residentes.

Ambiente de validação disponível: Windows, Java 21, Gradle 8.9, Android SDK 35, NDK 26.3, CMake 3.22.1; Android TV 11 x86 virtual com WHPX/SwiftShader em 1280x720. Variante x86 só para teste (`-PtestAbi=x86`). APK entregue é ARMv7. Resultados de emulador não equivalem à Philco.
