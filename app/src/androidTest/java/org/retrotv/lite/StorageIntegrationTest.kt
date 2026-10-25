package org.retrotv.lite
import android.net.Uri
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith

@RunWith(AndroidJUnit4::class)
class StorageIntegrationTest {
    private val context get() = InstrumentationRegistry.getInstrumentation().targetContext
    @Test fun optionalVisualPreviewFixture() {
        if (InstrumentationRegistry.getArguments().getString("preview") == "clear") {
            LibraryDatabase(context).use { it.writableDatabase.delete("games","id LIKE ?",arrayOf("visual-preview-%")) }
            val prefs=context.getSharedPreferences("retrotv",0)
            if(prefs.getString("tree", "")?.contains("org.retrotv.lite.testdocs") == true) prefs.edit().remove("tree").putString("tab","0").putString("positions","0,0,0,0,0,0,0,0,0,0").putString("minimal","0").commit()
            return
        }
        if (InstrumentationRegistry.getArguments().getString("preview") != "1") return
        val db = LibraryDatabase(context)
        try {
            require(db.snapshot().lines().filter { it.isNotBlank() }.all { it.startsWith("visual-preview-") }) { "Preview requires an empty test catalog; user games are preserved" }
            val titles=arrayOf("Orbit","Pulse","Drift","Signal","Vector","Echo")
            for (i in titles.indices) db.upsert(android.content.ContentValues().apply {
                put("id","visual-preview-$i");put("tree","content://org.retrotv.lite.testdocs/tree/root")
                put("uri","demo:retrotv-pad-test.nes");put("title",titles[i]);put("system", "NES");put("core","quicknes")
                put("scan","preview");put("cover","content://org.retrotv.lite.testdocs/tree/root/document/preview$i")
            })
            context.getSharedPreferences("retrotv",0).edit().putString("tree","content://org.retrotv.lite.testdocs/tree/root").putString("tab","0").putString("positions","0,0,0,0,0,0,0,0,0,0").putString("minimal","0").commit()
        } finally { db.close() }
    }
    @Test fun safScanDedupFavoriteAndDisconnectedRecovery() {
        context.deleteDatabase("storage-test.db")
        val db = LibraryDatabase(context, "storage-test.db")
        val library = UsbLibraryProvider(context, db)
        val tree = Uri.parse("content://org.retrotv.lite.testdocs/tree/root")
        fun scan(scenario: String) {
            TestDocumentsProvider.scenario = scenario
            val before = library.generation.get(); library.scan(tree)
            val deadline = System.currentTimeMillis() + 10000
            while (library.generation.get() == before && System.currentTimeMillis() < deadline) Thread.sleep(10)
            assertTrue("scan timed out: $scenario", library.generation.get() > before)
        }
        try {
            scan("normal"); assertEquals(1, db.snapshot().trim().lines().size); assertTrue(db.snapshot().trim().split('\t')[9].contains("cover"))
            val art = CoverRepository(context.contentResolver)
            try { val uri = db.snapshot().trim().split('\t')[9]; var pixels: ByteArray? = null; val deadline = System.currentTimeMillis() + 5000
                while (pixels == null && System.currentTimeMillis() < deadline) { pixels = art.poll(uri); Thread.sleep(20) }; assertEquals(240*320*4, pixels?.size)
            } finally { art.close() }
            val id = db.snapshot().substringBefore('\t'); db.favorite(id, true); db.record(id, 61)
            scan("normal"); val row = db.snapshot().trim().split('\t'); assertEquals("1", row[5]); assertEquals("61", row[7]); assertEquals(1, db.snapshot().trim().lines().size)
            scan("denied"); assertTrue(library.status.contains("Permissão")); assertTrue(db.snapshot().contains(id))
            scan("removed"); assertTrue(db.snapshot().contains(id))
            scan("empty"); assertEquals("0", db.snapshot().trim().split('\t')[8])
            scan("normal"); assertEquals("1", db.snapshot().trim().split('\t')[8])
            val before = library.generation.get(); TestDocumentsProvider.scenario = "removed"; library.validate(tree)
            val deadline = System.currentTimeMillis() + 10000
            while (library.generation.get() == before && System.currentTimeMillis() < deadline) Thread.sleep(10)
            assertEquals("0", db.snapshot().trim().split('\t')[8]); assertTrue(library.status.contains("USB ausente"))
        } finally { library.close(); db.close(); context.deleteDatabase("storage-test.db"); TestDocumentsProvider.scenario = "normal" }
    }
    @Test fun databaseV1MigrationPreservesUserMetadata() {
        context.deleteDatabase("migration-test.db")
        val path = context.getDatabasePath("migration-test.db"); path.parentFile?.mkdirs()
        android.database.sqlite.SQLiteDatabase.openOrCreateDatabase(path, null).use { db ->
            db.execSQL("CREATE TABLE games(id TEXT PRIMARY KEY, tree TEXT, uri TEXT, title TEXT, system TEXT, core TEXT, favorite INTEGER, recent INTEGER, played INTEGER, available INTEGER, scan TEXT, size INTEGER, modified INTEGER)")
            db.execSQL("INSERT INTO games VALUES('kept','tree','uri','Game','NES','quicknes',1,5,61,1,'old',10,1)")
            db.execSQL("UPDATE games SET uri=? WHERE id='kept'", arrayOf("content://provider/" + "x".repeat(500)))
            db.version = 1
        }
        val db = LibraryDatabase(context, "migration-test.db")
        try { assertTrue(db.snapshot().contains("kept")); assertEquals("1", db.snapshot().trim().split('\t')[5]); assertEquals("content://provider/" + "x".repeat(500), db.snapshot().trim().split('\t')[3]); assertEquals(3, db.readableDatabase.version); assertEquals("", db.snapshot().trimEnd('\n').split('\t')[9]) }
        finally { db.close(); context.deleteDatabase("migration-test.db") }
    }
    @Test fun databaseV2MigrationPreservesSavesAndFocusMetadata() {
        context.deleteDatabase("migration-v2-test.db")
        val path=context.getDatabasePath("migration-v2-test.db")
        android.database.sqlite.SQLiteDatabase.openOrCreateDatabase(path,null).use { db ->
            db.execSQL("CREATE TABLE games(id TEXT PRIMARY KEY,tree TEXT,uri TEXT,title TEXT,system TEXT,core TEXT,favorite INTEGER,recent INTEGER,played INTEGER,available INTEGER,scan TEXT,size INTEGER,modified INTEGER,relative TEXT)")
            db.execSQL("INSERT INTO games VALUES('kept','tree','uri','Game','NES','quicknes',1,5,61,1,'old',10,1,'/NES/Game.nes')");db.version=2
        }
        val db=LibraryDatabase(context,"migration-v2-test.db")
        try { val row=db.snapshot().trimEnd('\n').split('\t');assertEquals("1",row[5]);assertEquals("61",row[7]);assertEquals("",row[9]);assertEquals("/NES/Game.nes",db.discInfo("uri")[1]) }
        finally { db.close();context.deleteDatabase("migration-v2-test.db") }
    }
}
