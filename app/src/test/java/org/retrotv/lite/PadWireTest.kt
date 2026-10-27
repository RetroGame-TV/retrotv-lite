package org.retrotv.lite
import org.junit.Assert.*
import org.junit.Test
class PadWireTest {
    @Test fun inputLinesAreValidatedAndClamped(){
        assertEquals("201 9 127 -127 0 64",PadWire.parseInput("I 201 9 127 -127 0 64"))
        assertEquals("0 0 127 -127 0 0",PadWire.parseInput("I 0 0 900 -900 0 0"))
        assertEquals("10001 0 0 0 0 0",PadWire.parseInput("I 10001 0 0 0 0 0"))
        assertEquals("20000 0 0 0 0 0",PadWire.parseInput("I 20000 0 0 0 0 0"))
        assertNull(PadWire.parseInput("I 11001 0 0 0 0 0"));assertNull(PadWire.parseInput("I 40000 0 0 0 0 0"))
        for(bad in listOf("I 201 9 1 2 3","X 0 0 0 0 0 0","I zz 0 0 0 0 0","I 1000 0 0 0 0 0","I 0 16 0 0 0 0","I 0 0 a 0 0 0",""))assertNull(bad,PadWire.parseInput(bad))
    }
    @Test fun quickTapSurvivesABurstAndLatestStateStays(){
        val q=PhoneInputQueue()
        assertEquals("",q.poll())
        q.push(PhoneState(buttons=1));q.push(PhoneState())
        assertEquals("1 0 0 0 0 0",q.poll());assertEquals("0 0 0 0 0 0",q.poll());assertEquals("0 0 0 0 0 0",q.poll())
        for(i in 1..6)q.push(PhoneState(buttons=i))
        assertEquals("1 0 0 0 0 0",q.poll())
        q.reset();assertEquals("off",q.poll());assertEquals("",q.poll())
    }
    @Test fun phoneIdsAreBoundedTokens(){
        assertTrue(PadWire.validId("a1b2-c3d4"));assertFalse(PadWire.validId("abc"));assertFalse(PadWire.validId("x y z w"));assertFalse(PadWire.validId("a".repeat(41)))
    }
}
