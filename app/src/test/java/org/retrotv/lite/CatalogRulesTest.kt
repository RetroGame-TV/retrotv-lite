package org.retrotv.lite
import org.junit.Assert.*
import org.junit.Test
class CatalogRulesTest {
    @Test fun ambiguousFilesAreNotMisidentified() {
        assertNull(CatalogRules.system("", "track01.bin")); assertNull(CatalogRules.system("", "game.zip"))
        assertEquals("Arcade", CatalogRules.system("/Arcade", "test.zip"))
        assertEquals("PlayStation", CatalogRules.system("/PS1", "disc.chd"))
        assertNull(CatalogRules.system("/N64", "demo.z64"))
    }
    @Test fun identifiersSurviveTitleChangesAndSeparateRoots() {
        assertEquals(CatalogRules.stableId("a", "b"), CatalogRules.stableId("a", "b"))
        assertNotEquals(CatalogRules.stableId("a", "b"), CatalogRules.stableId("c", "b"))
        assertNotEquals(CatalogRules.stableId("a", "bc"), CatalogRules.stableId("ab", "c"))
    }
    @Test fun metadataCannotBreakBridgeRecords() { assertFalse(CatalogRules.clean("a\t\nb").contains('\t')); assertEquals(240, CatalogRules.clean("a".repeat(400)).length) }
}
