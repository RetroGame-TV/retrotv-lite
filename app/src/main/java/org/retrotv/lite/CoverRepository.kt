package org.retrotv.lite

import android.content.ContentResolver
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.graphics.Canvas
import android.graphics.Paint
import android.graphics.RectF
import android.net.Uri
import java.nio.ByteBuffer
import java.util.concurrent.Executors

/** Small asynchronous staging cache. Native UI owns at most six 192x256 textures. */
class CoverRepository(private val resolver: ContentResolver) {
    private val worker = Executors.newSingleThreadExecutor()
    private val pending = HashSet<String>()
    private val failed = LinkedHashSet<String>()
    private val ready = LinkedHashMap<String, ByteArray>()
    @Volatile private var closed = false
    private var generation = 0
    @Synchronized fun invalidate() { generation++; ready.clear(); failed.clear() }
    @Synchronized fun poll(value: String): ByteArray? {
        ready.remove(value)?.let { return it }
        if (closed || value.isBlank() || value in pending || value in failed || pending.size >= 8) return null
        val uri = Uri.parse(value)
        if (uri.scheme != "content") return null
        pending.add(value)
        val token = generation
        worker.execute {
            val pixels = runCatching { decode(uri) }.getOrNull()
            synchronized(this) {
                pending.remove(value)
                if (!closed && token == generation) {
                    if (pixels != null) { ready[value] = pixels; while (ready.size > 8) ready.remove(ready.keys.first()) }
                    else { failed.add(value); while (failed.size > 128) failed.remove(failed.first()) }
                }
            }
        }
        return null
    }
    private fun decode(uri: Uri): ByteArray {
        resolver.openAssetFileDescriptor(uri, "r")?.use { require(it.length in -1L..12L * 1024 * 1024) }
        val bounds = BitmapFactory.Options().apply { inJustDecodeBounds = true }
        resolver.openInputStream(uri)?.use { BitmapFactory.decodeStream(it, null, bounds) }
        require(bounds.outWidth > 0 && bounds.outHeight > 0 && bounds.outWidth.toLong() * bounds.outHeight <= 32000000)
        var sample = 1
        while (bounds.outWidth / sample > 512 || bounds.outHeight / sample > 512) sample *= 2
        val options = BitmapFactory.Options().apply { inSampleSize = sample; inPreferredConfig = Bitmap.Config.ARGB_8888 }
        val source = resolver.openInputStream(uri)?.use { BitmapFactory.decodeStream(it, null, options) } ?: error("Invalid cover")
        val target = Bitmap.createBitmap(192, 256, Bitmap.Config.ARGB_8888)
        try {
            val canvas = Canvas(target); canvas.drawColor(0xff25232b.toInt())
            val scale = minOf(192f / source.width, 256f / source.height)
            val width = source.width * scale; val height = source.height * scale
            canvas.drawBitmap(source, null, RectF((192-width)/2, (256-height)/2, (192+width)/2, (256+height)/2), Paint(Paint.FILTER_BITMAP_FLAG))
            return ByteBuffer.allocate(target.byteCount).also { target.copyPixelsToBuffer(it) }.array()
        } finally { source.recycle(); target.recycle() }
    }
    @Synchronized fun close() { closed = true; ready.clear(); failed.clear(); worker.shutdown() }
}
