package org.retrotv.lite

import android.content.ContentResolver
import android.net.Uri
import android.provider.DocumentsContract as DC
import android.system.Os
import android.system.OsConstants

/** Resolves CUE/M3U siblings inside the granted tree; never stages a CD to flash. */
class DiscAccess(private val resolver: ContentResolver) {
    private var tree: Uri? = null
    private val cache = object : LinkedHashMap<String, Uri>(128, 0.75f, true) {
        override fun removeEldestEntry(eldest: MutableMap.MutableEntry<String, Uri>?) = size > 128
    }
    @Synchronized fun prepare(treeUri: String, relative: String): String {
        tree = Uri.parse(treeUri); cache.clear()
        return "/__retrotv_saf__/" + normalize(relative).joinToString("/")
    }
    companion object {
        fun normalize(path: String): List<String> {
            require(!path.contains('\\') && !path.contains('\u0000') && !path.contains('\t') && !path.contains('\n')) { "Caminho inválido" }
            val parts = ArrayList<String>()
            for (part in path.removePrefix("/__retrotv_saf__/").removePrefix("saf:/").trimStart('/').split('/')) when (part) {
                "", "." -> Unit
                ".." -> { require(parts.isNotEmpty()) { "Referência fora da biblioteca" }; parts.removeAt(parts.lastIndex) }
                else -> parts.add(part)
            }
            require(parts.isNotEmpty()) { "Caminho vazio" }
            return parts
        }
    }
    @Synchronized fun open(path: String): Int {
        require(path.startsWith("/__retrotv_saf__/")) { "Somente árvore SAF autorizada" }
        val root = tree ?: error("Disco não preparado")
        val parts = normalize(path); val key = parts.joinToString("/")
        var uri = cache[key]
        if (uri == null) {
            var parent = DC.getTreeDocumentId(root)
            for (name in parts) {
                val children = DC.buildChildDocumentsUriUsingTree(root, parent)
                var found: String? = null
                resolver.query(children, arrayOf(DC.Document.COLUMN_DOCUMENT_ID, DC.Document.COLUMN_DISPLAY_NAME), null, null, null)?.use { c ->
                    while (c.moveToNext()) if (c.getString(1) == name) { found = c.getString(0); break }
                } ?: error("USB desconectado")
                parent = found ?: error("Faixa/disco ausente: $name")
            }
            uri = DC.buildDocumentUriUsingTree(root, parent); cache[key] = uri
        }
        val fd = resolver.openFileDescriptor(uri, "r") ?: error("Disco indisponível")
        try { Os.lseek(fd.fileDescriptor, 0L, OsConstants.SEEK_SET) } catch (e: Exception) { fd.close(); error("Provedor não permite seek") }
        return fd.detachFd()
    }
}
