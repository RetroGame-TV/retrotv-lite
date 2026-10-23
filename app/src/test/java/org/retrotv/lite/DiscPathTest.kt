package org.retrotv.lite
import org.junit.Assert.*
import org.junit.Test
class DiscPathTest {
    @Test fun multidiscRelativeReferencesStayInsideTree() {
        assertEquals(listOf("PS1", "disc2.bin"), DiscAccess.normalize("saf:/PS1/set/../disc2.bin"))
        for (path in listOf("../outside", "saf:/../../outside", "PS1\\track.bin", "x\u0000y")) {
            var rejected = false; try { DiscAccess.normalize(path) } catch (e: Exception) { rejected = true }; assertTrue(path, rejected)
        }
    }
}
