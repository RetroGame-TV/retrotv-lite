package org.retrotv.lite

import org.libsdl.app.SDLActivity
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import android.os.Bundle
import java.nio.ByteBuffer

class RetroActivity : SDLActivity() {
    override fun getLibraries(): Array<String> = arrayOf("SDL2", "main")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.decorView.systemUiVisibility = 5894
    }
    fun fontAtlas(): ByteArray {
        val bitmap = Bitmap.createBitmap(1024, 896, Bitmap.Config.ARGB_8888)
        val canvas = Canvas(bitmap)
        val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
            color = Color.WHITE; textSize = 30f; typeface = Typeface.create("sans-serif", Typeface.NORMAL)
        }
        for (code in 32..255) {
            val i = code - 32
            canvas.drawText(code.toChar().toString(), (i % 16 * 64).toFloat(), (i / 16 * 64 + 36).toFloat(), paint)
        }
        val bytes = ByteBuffer.allocate(bitmap.byteCount)
        bitmap.copyPixelsToBuffer(bytes); bitmap.recycle()
        return bytes.array()
    }
    fun status(value: String): String = "Nenhuma pasta selecionada"
}
