package org.retrotv.lite
import android.database.Cursor
import android.database.MatrixCursor
import android.os.CancellationSignal
import android.os.ParcelFileDescriptor
import android.provider.DocumentsContract.Document
import android.provider.DocumentsProvider
import java.io.File
class TestDocumentsProvider : DocumentsProvider() {
    companion object { @Volatile var scenario = "normal" }
    override fun onCreate() = true
    override fun queryRoots(projection: Array<out String>?): Cursor = MatrixCursor(arrayOf("root_id", "document_id", "title", "flags")).apply { addRow(arrayOf("root", "root", "Synthetic RetroTV test", 0)) }
    private val cols = arrayOf(Document.COLUMN_DOCUMENT_ID, Document.COLUMN_DISPLAY_NAME, Document.COLUMN_MIME_TYPE, Document.COLUMN_SIZE, Document.COLUMN_LAST_MODIFIED)
    override fun queryDocument(documentId: String, projection: Array<out String>?): Cursor = MatrixCursor(cols).apply {
        addRow(arrayOf(documentId, if (documentId == "root") "RetroTV test" else "pad.nes", if (documentId == "root") Document.MIME_TYPE_DIR else "application/octet-stream", 24592L, 1L))
    }
    override fun queryChildDocuments(parentDocumentId: String, projection: Array<out String>?, sortOrder: String?): Cursor {
        if (scenario == "denied") throw SecurityException("Synthetic permission denied")
        if (scenario == "removed") throw java.io.FileNotFoundException("Synthetic USB removed")
        return MatrixCursor(cols).apply { if (scenario != "empty") addRow(arrayOf("pad", "pad.nes", "application/octet-stream", 24592L, 1L)) }
    }
    override fun isChildDocument(parentDocumentId: String, documentId: String) = parentDocumentId == "root" && documentId == "pad"
    override fun openDocument(documentId: String, mode: String, signal: CancellationSignal?): ParcelFileDescriptor {
        require(mode == "r" && documentId == "pad")
        val file = File(context!!.cacheDir, "synthetic-pad.nes")
        context!!.assets.open("demo/retrotv-pad-test.nes").use { input -> file.outputStream().use { input.copyTo(it) } }
        return ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY)
    }
}
