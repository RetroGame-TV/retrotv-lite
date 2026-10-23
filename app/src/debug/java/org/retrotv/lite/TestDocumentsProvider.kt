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
        if (scenario == "denied") throw SecurityException("Synthetic permission denied")
        if (scenario == "removed") throw java.io.FileNotFoundException("Synthetic USB removed")
        addRow(arrayOf(documentId, if (documentId == "root") "RetroTV test" else "pad.nes", if (documentId == "root") Document.MIME_TYPE_DIR else "application/octet-stream", 24592L, 1L))
    }
    override fun queryChildDocuments(parentDocumentId: String, projection: Array<out String>?, sortOrder: String?): Cursor {
        if (scenario == "denied") throw SecurityException("Synthetic permission denied")
        if (scenario == "removed") throw java.io.FileNotFoundException("Synthetic USB removed")
        return MatrixCursor(cols).apply { if (scenario != "empty") { addRow(arrayOf("pad", "pad.nes", "application/octet-stream", 24592L, 1L)); addRow(arrayOf("cover", "pad.png", "image/png", 2000L, 1L)) } }
    }
    override fun isChildDocument(parentDocumentId: String, documentId: String) = parentDocumentId == "root" && (documentId == "pad" || documentId == "cover" || documentId.startsWith("preview"))
    override fun openDocument(documentId: String, mode: String, signal: CancellationSignal?): ParcelFileDescriptor {
        require(mode == "r" && (documentId == "pad" || documentId == "cover" || documentId.startsWith("preview")))
        if(documentId == "cover" || documentId.startsWith("preview")) {
            val index = documentId.removePrefix("preview").toIntOrNull()?.coerceIn(0,5) ?: 0
            val file=File(context!!.cacheDir,"original-cover-$index.png")
            if(!file.exists()) createCover(file,index)
            return ParcelFileDescriptor.open(file,ParcelFileDescriptor.MODE_READ_ONLY)
        }
        val file = File(context!!.cacheDir, "synthetic-pad.nes")
        context!!.assets.open("demo/retrotv-pad-test.nes").use { input -> file.outputStream().use { input.copyTo(it) } }
        return ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY)
    }
    private fun createCover(file: File,index: Int) {
        // Original geometric posters for renderer tests. No commercial assets.
        val colors=intArrayOf(0xffD87F55.toInt(),0xff8B8ED0.toInt(),0xff4C9B99.toInt(),0xffB99A4F.toInt(),0xffA66D9B.toInt(),0xff668AC3.toInt())
        val titles=arrayOf("ORBIT","PULSE","DRIFT","SIGNAL","VECTOR","ECHO")
        val bitmap=android.graphics.Bitmap.createBitmap(384,512,android.graphics.Bitmap.Config.ARGB_8888)
        val canvas=android.graphics.Canvas(bitmap);val paint=android.graphics.Paint(android.graphics.Paint.ANTI_ALIAS_FLAG)
        canvas.drawColor(colors[index]);paint.color=0xff20202A.toInt()
        if(index%3==0){for(i in 0..3){paint.style=android.graphics.Paint.Style.STROKE;paint.strokeWidth=13f;canvas.drawCircle(192f,243f,49f+i*35,paint)};paint.style=android.graphics.Paint.Style.FILL;canvas.drawCircle(275f,173f,29f,paint)}
        if(index%3==1){paint.strokeWidth=18f;val path=android.graphics.Path();path.moveTo(42f,272f);path.lineTo(105f,272f);path.lineTo(145f,156f);path.lineTo(206f,346f);path.lineTo(252f,225f);path.lineTo(343f,225f);paint.style=android.graphics.Paint.Style.STROKE;canvas.drawPath(path,paint);paint.style=android.graphics.Paint.Style.FILL}
        if(index%3==2){canvas.save();canvas.rotate(-24f,192f,250f);for(i in 0..3)canvas.drawRoundRect(55f+i*63,115f,92f+i*63,361f,18f,18f,paint);canvas.restore()}
        paint.typeface=android.graphics.Typeface.create("sans-serif-medium",android.graphics.Typeface.NORMAL);paint.textSize=21f;canvas.drawText("RETROTV  /  ORIGINALS",27f,44f,paint)
        paint.textSize=51f;canvas.drawText(titles[index],26f,445f,paint);paint.textSize=17f;canvas.drawText("HOME BREW  ·  VISUAL TEST",29f,482f,paint)
        file.outputStream().use{bitmap.compress(android.graphics.Bitmap.CompressFormat.PNG,100,it)};bitmap.recycle()
    }
}
