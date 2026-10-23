package org.retrotv.lite

import org.libsdl.app.SDLActivity
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Typeface
import android.os.Bundle
import java.nio.ByteBuffer
import android.app.Activity
import android.content.Intent
import android.content.ActivityNotFoundException
import android.net.Uri
import android.os.Debug
import android.os.ParcelFileDescriptor
import android.system.Os
import android.system.OsConstants
import java.io.File
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean

class RetroActivity : SDLActivity() {
    private val prefs by lazy { getSharedPreferences("retrotv", MODE_PRIVATE) }
    private val database by lazy { LibraryDatabase(this) }
    private val library by lazy { UsbLibraryProvider(this, database) }
    private val transfers = Executors.newSingleThreadExecutor()
    private val transferBusy = AtomicBoolean(false)
    private val discAccess by lazy { DiscAccess(contentResolver) }
    override fun getLibraries(): Array<String> = arrayOf("SDL2", "main")
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.decorView.systemUiVisibility = 5894
        library.status = if (prefs.contains("tree")) "Catálogo salvo · USB a verificar" else "Escolha sua biblioteca USB"
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
    fun fontMetrics(value: String): String {
        val paint = Paint(Paint.ANTI_ALIAS_FLAG).apply { textSize = 30f; typeface = Typeface.create("sans-serif", Typeface.NORMAL) }
        return (32..255).joinToString(",") { kotlin.math.ceil(paint.measureText(it.toChar().toString()).toDouble()).toInt().toString() }
    }
    fun status(value: String): String = library.status
    fun catalog(value: String): String = database.snapshot()
    fun revision(value: String): String = library.generation.get().toString()
    fun setting(value: String): String {
        val parts = value.split('\t', limit = 2)
        if (parts.size == 2) { prefs.edit().putString(parts[0], parts[1]).apply(); return parts[1] }
        return prefs.getString(value, "") ?: ""
    }
    fun path(value: String): String = when (value) {
        "native" -> applicationInfo.nativeLibraryDir
        "saves" -> File(filesDir, "saves").apply { mkdirs() }.absolutePath
        "bios" -> File(filesDir, "bios").apply { mkdirs() }.absolutePath
        else -> filesDir.absolutePath
    }
    fun diagnostics(value: String): String = "Android ${android.os.Build.VERSION.RELEASE}\nABI: ${android.os.Build.SUPPORTED_ABIS.joinToString()}\nPSS local: ${Debug.getPss()} KiB\nSDL2 / OpenGL ES 2.0\nQuickNES integrado; demais cores condicionais\nN64: desabilitado, sem validação GLES2\n${library.status}"
    fun request(value: String): String { return try {
        val p = value.split('\t')
        when (p[0]) {
            "choose" -> runOnUiThread { pickTree() }
            "scan" -> prefs.getString("tree", null)?.let { library.scan(Uri.parse(it)) }
            "favorite" -> { database.favorite(p[1], p[2] == "1"); library.generation.incrementAndGet() }
            "record" -> { database.record(p[1], p[2].toLong()); library.generation.incrementAndGet() }
            "export" -> runOnUiThread {
                try { startActivityForResult(Intent(Intent.ACTION_CREATE_DOCUMENT).apply { type = "application/zip"; addCategory(Intent.CATEGORY_OPENABLE); putExtra(Intent.EXTRA_TITLE, "retrotv-saves.zip") }, 101) }
                catch (e: ActivityNotFoundException) { library.status = "Seletor SAF indisponível" }
            }
            "restore" -> runOnUiThread {
                try { startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT).apply { type = "application/zip"; addCategory(Intent.CATEGORY_OPENABLE) }, 102) }
                catch (e: ActivityNotFoundException) { library.status = "Seletor SAF indisponível" }
            }
            "bios" -> runOnUiThread {
                try { startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT).apply { type = "application/octet-stream"; addCategory(Intent.CATEGORY_OPENABLE) }, 103) }
                catch (e: ActivityNotFoundException) { library.status = "Seletor SAF indisponível" }
            }
            "disc" -> { val info = database.discInfo(p[1]); return discAccess.prepare(info[0], info[1]) + "\t" + info[2] + ":" + info[3] }
            "vfs" -> return discAccess.open(p[1]).toString()
            "open" -> {
                check(!transferBusy.get()) { "Aguarde a transferência de saves/BIOS" }
                if (p[1] == "demo:retrotv-pad-test.nes") {
                    val demo = File(cacheDir, "retrotv-pad-test.nes")
                    assets.open("demo/retrotv-pad-test.nes").use { input -> demo.outputStream().use { input.copyTo(it) } }
                    return ParcelFileDescriptor.open(demo, ParcelFileDescriptor.MODE_READ_ONLY).detachFd().toString()
                }
                val uri = Uri.parse(p[1])
                val fd = contentResolver.openFileDescriptor(uri, "r") ?: error("USB indisponível")
                try { Os.lseek(fd.fileDescriptor, 0, OsConstants.SEEK_SET) } catch (e: Exception) { fd.close(); error("Este provedor USB não permite seek") }
                return fd.detachFd().toString()
            }
        }
        "OK"
    } catch (e: Exception) { "ERR: ${e.message ?: "falha de acesso"}" } }
    private fun pickTree() {
        try {
            startActivityForResult(Intent(Intent.ACTION_OPEN_DOCUMENT_TREE).apply {
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION)
            }, 100)
        } catch (e: ActivityNotFoundException) { library.status = "TV sem seletor SAF. Consulte docs/usb-storage.md" }
    }
    @Deprecated("SDLActivity uses legacy Activity result dispatch")
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        if (requestCode == 100 && resultCode == Activity.RESULT_OK) {
            val uri = data?.data ?: return
            try {
                check(data.flags and Intent.FLAG_GRANT_READ_URI_PERMISSION != 0) { "Leitura não concedida" }
                contentResolver.takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION)
                prefs.edit().putString("tree", uri.toString()).apply(); library.scan(uri)
            } catch (e: Exception) { library.status = "Sem permissão persistente: ${e.message}" }
        }
        if (requestCode in 101..103 && resultCode == Activity.RESULT_OK) {
            val uri = data?.data ?: return
            if (!transferBusy.compareAndSet(false, true)) { library.status = "Aguarde a transferência atual"; return }
            transfers.execute {
                try {
                    val saves = File(filesDir, "saves")
                    when (requestCode) {
                        101 -> { val output = contentResolver.openOutputStream(uri, "wt") ?: error("Destino indisponível"); output.use { SaveArchive.export(saves, it) }; library.status = "Backup exportado com checksums" }
                        102 -> { val input = contentResolver.openInputStream(uri) ?: error("Backup indisponível"); input.use { SaveArchive.restore(saves, it) }; library.status = "Saves restaurados · cópia anterior preservada" }
                        103 -> {
                            val dir = File(filesDir, "bios").apply { mkdirs() }; val temp = File(dir, "bios.tmp")
                            contentResolver.openInputStream(uri)?.use { input -> temp.outputStream().use { out -> val b = ByteArray(65536); var size = 0; while (true) { val n = input.read(b); if (n < 0) break; size += n; require(size <= 524288) { "BIOS excede 512 KiB" }; out.write(b, 0, n) }; require(size == 524288) { "BIOS PS1 deve ter 512 KiB" }; out.fd.sync() } } ?: error("BIOS ilegível")
                            val dest = File(dir, "scph5501.bin"); if (dest.exists()) dest.copyTo(File(dir, "scph5501.bin.prev"), overwrite = true)
                            java.nio.file.Files.move(temp.toPath(), dest.toPath(), java.nio.file.StandardCopyOption.REPLACE_EXISTING, java.nio.file.StandardCopyOption.ATOMIC_MOVE)
                            library.status = "BIOS importada pelo usuário; região a confirmar"
                        }
                    }
                } catch (e: Exception) { library.status = "Falha: ${e.message}; saves anteriores preservados" }
                finally { transferBusy.set(false) }
            }
        }
    }
    override fun onDestroy() { library.close(); super.onDestroy(); transfers.shutdown(); database.close() }
}
