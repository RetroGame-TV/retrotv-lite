package org.retrotv.lite

import org.libsdl.app.SDLActivity

class RetroActivity : SDLActivity() {
    override fun getLibraries(): Array<String> = arrayOf("SDL2", "main")
}
