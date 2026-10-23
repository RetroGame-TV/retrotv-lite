package org.retrotv.lite

import android.content.ContentValues
import android.content.Context
import android.net.Uri
import android.provider.DocumentsContract as DC
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
                    cursor.use { c ->
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
    fun close() { cancel(); executor.shutdown() }
}
