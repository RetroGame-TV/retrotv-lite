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
            scan("normal"); assertEquals(1, db.snapshot().trim().lines().size)
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
        try { assertTrue(db.snapshot().contains("kept")); assertEquals("1", db.snapshot().trim().split('\t')[5]); assertEquals("content://provider/" + "x".repeat(500), db.snapshot().trim().split('\t')[3]); assertEquals(2, db.readableDatabase.version) }
        finally { db.close(); context.deleteDatabase("migration-test.db") }
    }
}
