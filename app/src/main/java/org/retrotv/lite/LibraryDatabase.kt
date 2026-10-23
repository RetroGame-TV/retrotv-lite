package org.retrotv.lite

import android.content.ContentValues
import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

class LibraryDatabase(context: Context, name: String = "catalog.db") : SQLiteOpenHelper(context, name, null, 2) {
    override fun onCreate(db: SQLiteDatabase) {
        db.execSQL("CREATE TABLE games(id TEXT PRIMARY KEY, tree TEXT NOT NULL, uri TEXT NOT NULL, title TEXT NOT NULL, system TEXT NOT NULL, core TEXT NOT NULL, favorite INTEGER NOT NULL DEFAULT 0, recent INTEGER NOT NULL DEFAULT 0, played INTEGER NOT NULL DEFAULT 0, available INTEGER NOT NULL DEFAULT 1, scan TEXT NOT NULL, size INTEGER NOT NULL DEFAULT 0, modified INTEGER NOT NULL DEFAULT 0)")
        db.execSQL("CREATE INDEX collection ON games(system, title)")
        db.execSQL("CREATE INDEX roots ON games(tree, scan)")
        db.execSQL("ALTER TABLE games ADD COLUMN relative TEXT NOT NULL DEFAULT ''")
    }
    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        if (oldVersion == 1 && newVersion == 2) db.execSQL("ALTER TABLE games ADD COLUMN relative TEXT NOT NULL DEFAULT ''")
        else error("Unsupported database migration $oldVersion -> $newVersion; preserve catalog.db")
    }
    fun upsert(v: ContentValues) {
        // Updating preserves favorites, history and played time. Never REPLACE the row.
        val db = writableDatabase
        if (db.update("games", v, "id=?", arrayOf(v.getAsString("id"))) == 0) db.insertOrThrow("games", null, v)
    }
    fun snapshot(): String = buildString {
        readableDatabase.rawQuery("SELECT id,title,system,uri,core,favorite,recent,played,available FROM games ORDER BY title COLLATE NOCASE LIMIT 10000", null).use { c ->
            while (c.moveToNext()) {
                append((0..8).joinToString("\t") { index ->
                    val value = c.getString(index) ?: ""
                    if (index == 1) CatalogRules.clean(value) else value.replace('\t', ' ').replace('\n', ' ').replace('\r', ' ')
                }); append('\n')
            }
        }
    }
    fun favorite(id: String, value: Boolean) = writableDatabase.execSQL("UPDATE games SET favorite=? WHERE id=?", arrayOf(if(value) 1 else 0, id))
    fun record(id: String, seconds: Long) = writableDatabase.execSQL("UPDATE games SET recent=?,played=played+? WHERE id=?", arrayOf(System.currentTimeMillis(), seconds.coerceIn(0, 86400), id))
    fun finishScan(tree: String, scan: String) = writableDatabase.execSQL("UPDATE games SET available=CASE WHEN scan=? THEN 1 ELSE 0 END WHERE tree=?", arrayOf(scan, tree))
    fun unavailable(uri: String) = writableDatabase.execSQL("UPDATE games SET available=0 WHERE uri=?", arrayOf(uri))
    fun treeUnavailable(tree: String) = writableDatabase.execSQL("UPDATE games SET available=0 WHERE tree=?", arrayOf(tree))
    fun discInfo(uri: String): Array<String> = readableDatabase.rawQuery("SELECT tree,relative,size,modified FROM games WHERE uri=?", arrayOf(uri)).use { c ->
        require(c.moveToFirst()) { "Disco não está no catálogo" }; Array(4) { c.getString(it) }
    }
}
