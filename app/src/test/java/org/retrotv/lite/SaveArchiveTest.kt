package org.retrotv.lite
import org.junit.Assert.*
import org.junit.Test
import java.io.*
import java.nio.file.Files
import java.util.zip.*
class SaveArchiveTest {
    @Test fun roundtripPreservesAndBacksUpExistingSaves() {
        val base = Files.createTempDirectory("retrotv-test").toFile()
        try {
            val source = File(base, "source").apply { mkdirs() }; File(source, "game").mkdirs(); File(source, "game/progress.sram").writeText("new")
            val archive = ByteArrayOutputStream(); SaveArchive.export(source, archive)
            val target = File(base, "saves").apply { mkdirs() }; File(target, "old.sram").writeText("old")
            SaveArchive.restore(target, ByteArrayInputStream(archive.toByteArray()))
            assertEquals("new", File(target, "game/progress.sram").readText()); assertEquals("old", File(base, "saves.previous/old.sram").readText())
        } finally { base.deleteRecursively() }
    }
    @Test fun traversalAndCorruptionNeverReplaceCurrentDirectory() {
        for (name in listOf("../escape", "/escape", "safe.sram")) {
            val base = Files.createTempDirectory("retrotv-test").toFile()
            try {
                val target = File(base, "saves").apply { mkdirs() }; File(target, "valid").writeText("keep")
                val bytes = ByteArrayOutputStream()
                ZipOutputStream(bytes).use { z -> z.putNextEntry(ZipEntry(name)); z.write("bad".toByteArray()); z.closeEntry() }
                var rejected = false; try { SaveArchive.restore(target, ByteArrayInputStream(bytes.toByteArray())) } catch (e: Exception) { rejected = true }
                assertTrue(rejected); assertEquals("keep", File(target, "valid").readText()); assertFalse(File(base, "escape").exists())
            } finally { base.deleteRecursively() }
        }
    }
    @Test fun wrongChecksumRejectedBeforeSwap() {
        val base = Files.createTempDirectory("retrotv-test").toFile()
        try {
            val target = File(base, "saves").apply { mkdirs() }; File(target, "old").writeText("keep")
            val bytes = ByteArrayOutputStream(); ZipOutputStream(bytes).use { z ->
                z.putNextEntry(ZipEntry("bad.sram")); z.write(byteArrayOf(1)); z.closeEntry()
                z.putNextEntry(ZipEntry("backup.manifest")); z.write(("RETROTV-BACKUP-1\n" + "0".repeat(64) + "\tbad.sram\n").toByteArray()); z.closeEntry()
            }
            var rejected = false; try { SaveArchive.restore(target, ByteArrayInputStream(bytes.toByteArray())) } catch (e: Exception) { rejected = true }
            assertTrue(rejected); assertEquals("keep", File(target, "old").readText())
        } finally { base.deleteRecursively() }
    }
}
