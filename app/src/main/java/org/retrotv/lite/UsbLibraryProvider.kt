package org.retrotv.lite

import android.content.ContentValues
import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract as DC
import java.io.File
import java.util.ArrayDeque
import java.util.UUID
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicLong

class UsbLibraryProvider(private val context: Context, private val db: LibraryDatabase) {
    private val executor = Executors.newSingleThreadExecutor()
    private val cancelled = AtomicBoolean(false)
    private val scanning = AtomicBoolean(false)
    val generation = AtomicLong(1)
    @Volatile var status = "Nenhuma pasta selecionada"
    fun cancel() { cancelled.set(true) }
    fun validate(tree: Uri) {
        executor.execute {
            try {
                val doc = DC.buildDocumentUriUsingTree(tree, DC.getTreeDocumentId(tree))
                context.contentResolver.query(doc, arrayOf(DC.Document.COLUMN_DOCUMENT_ID), null, null, null)?.use { require(it.moveToFirst()) { "USB ausente" } } ?: error("USB ausente")
                if (!scanning.get()) status = "USB disponível · atualize se alterou arquivos"
            } catch (e: Exception) {
                db.treeUnavailable(tree.toString())
                status = if (e is SecurityException) "Permissão perdida: selecione a pasta" else "USB ausente: reconecte e atualize a biblioteca"
                generation.incrementAndGet()
            }
        }
    }
    fun scan(tree: Uri) {
        if (!scanning.compareAndSet(false, true)) { cancel(); status = "Cancelando leitura USB"; return }
        cancelled.set(false)
        executor.execute {
            val scan = UUID.randomUUID().toString()
            var count = 0
            try {
                status = "Lendo biblioteca USB…"
                val queue = ArrayDeque<Pair<String, String>>()
                queue.add(DC.getTreeDocumentId(tree) to "")
                var rootName = ""
                context.contentResolver.query(DC.buildDocumentUriUsingTree(tree, DC.getTreeDocumentId(tree)), arrayOf(DC.Document.COLUMN_DISPLAY_NAME), null, null, null)?.use { if (it.moveToFirst()) rootName = it.getString(0) ?: "" }
                val visited = HashSet<String>()
                while (queue.isNotEmpty() && !cancelled.get()) {
                    val (parent, folder) = queue.removeFirst()
                    if (!visited.add(parent)) continue
                    require(visited.size <= 20000) { "Muitas pastas: escolha uma subpasta menor" }
                    val children = DC.buildChildDocumentsUriUsingTree(tree, parent)
                    val columns = arrayOf(DC.Document.COLUMN_DOCUMENT_ID, DC.Document.COLUMN_DISPLAY_NAME, DC.Document.COLUMN_MIME_TYPE, DC.Document.COLUMN_SIZE, DC.Document.COLUMN_LAST_MODIFIED)
                    val cursor = context.contentResolver.query(children, columns, null, null, null) ?: error("USB indisponível")
                    val covers = HashMap<String, String>()
                    // The cursor is rewindable: discover sidecar art before indexing ROMs.
                    cursor.use { c ->
                        while (c.moveToNext() && !cancelled.get()) {
                            val name = c.getString(1) ?: continue
                            if (name.substringAfterLast('.', "").lowercase() in setOf("png", "jpg", "jpeg", "webp") && c.getLong(3) in 1..12L*1024*1024)
                                covers[name.substringBeforeLast('.').lowercase()] = DC.buildDocumentUriUsingTree(tree, c.getString(0)).toString()
                        }
                        c.moveToPosition(-1)
                        while (c.moveToNext() && !cancelled.get()) {
                            val id = c.getString(0); val name = c.getString(1) ?: continue
                            if (c.getString(2) == DC.Document.MIME_TYPE_DIR) {
                                if (name != "Android" && name != "data") queue.add(id to "$folder/$name")
                                continue
                            }
                            val system = CatalogRules.system("$rootName/$folder", name) ?: continue
                            require(++count <= 10000) { "Limite de 10.000 jogos; escolha uma subpasta" }
                            db.upsert(ContentValues().apply {
                                put("id", CatalogRules.stableId(tree.toString(), id)); put("tree", tree.toString())
                                put("uri", DC.buildDocumentUriUsingTree(tree, id).toString())
                                put("title", CatalogRules.clean(name.substringBeforeLast('.'))); put("system", system)
                                put("core", CatalogRules.core(system)); put("scan", scan); put("available", 1)
                                put("size", c.getLong(3)); put("modified", c.getLong(4))
                                put("relative", "$folder/$name")
                                put("cover", covers[name.substringBeforeLast('.').lowercase()] ?: "")
                            })
                            if (count % 50 == 0) { status = "$count jogos encontrados"; generation.incrementAndGet() }
                        }
                    }
                }
                if (!cancelled.get()) { db.finishScan(tree.toString(), scan); status = "$count jogos · USB conectado" }
                else status = "Leitura cancelada · catálogo preservado"
            } catch (e: Exception) {
                status = if (e is SecurityException) "Permissão perdida: escolha a pasta novamente" else "USB: ${e.message ?: "erro de leitura"}"
            } finally { scanning.set(false); generation.incrementAndGet() }
        }
    }
    /**
     * App-private folder (files/Games): no picker and no permission, so it also works on
     * TVs whose folder picker cannot confirm a subfolder. Cartridge games only; disc images stay on the USB path.
     * Runs on the same single-thread executor as [scan], so the two never overlap.
     */
    fun scanLocal(dir: File) {
        executor.execute {
            val key = "local:" + dir.absolutePath
            val scan = UUID.randomUUID().toString()
            var count = 0
            try {
                dir.walkTopDown().maxDepth(6).filter { it.isFile }.forEach { file ->
                    val relative = file.relativeTo(dir).path.replace('\\', '/')
                    val system = CatalogRules.system("Games/" + relative.substringBeforeLast('/', ""), file.name) ?: return@forEach
                    if (system == "PlayStation" || count >= 10000) return@forEach
                    count++
                    val cover = listOf("png", "jpg", "jpeg", "webp").map { File(file.parentFile, file.nameWithoutExtension + "." + it) }
                        .firstOrNull { it.isFile && it.length() in 1..12L * 1024 * 1024 }
                    db.upsert(ContentValues().apply {
                        put("id", CatalogRules.stableId(key, relative)); put("tree", key)
                        put("uri", Uri.fromFile(file).toString())
                        put("title", CatalogRules.clean(file.nameWithoutExtension)); put("system", system)
                        put("core", CatalogRules.core(system)); put("scan", scan); put("available", 1)
                        put("size", file.length()); put("modified", file.lastModified())
                        put("relative", "/$relative"); put("cover", cover?.let { Uri.fromFile(it).toString() } ?: "")
                    })
                }
                db.finishScan(key, scan)
                // Replace the "choose a USB folder" prompts, never a real USB status.
                if (count > 0 && (status.startsWith("Escolha") || status.startsWith("Nenhuma") || status.contains("pasta do RetroTV"))) status = "$count jogos na pasta do RetroTV"
            } catch (e: Exception) {
                status = "Pasta do RetroTV: ${e.message ?: "erro de leitura"}"
            } finally { generation.incrementAndGet() }
        }
    }
    fun close() { cancel(); executor.shutdown() }
}
