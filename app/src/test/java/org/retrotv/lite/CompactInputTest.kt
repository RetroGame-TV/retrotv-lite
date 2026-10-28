package org.retrotv.lite
import org.junit.Assert.*
import org.junit.Test
import java.io.ByteArrayInputStream
class CompactInputTest{
    private val fixture=byteArrayOf(0xF1.toByte(),0xA5.toByte(),15,3,9,(-127).toByte(),127,(-1).toByte(),0)
    @Test fun decodesSharedFixture(){assertEquals(PhoneState(0x30FA5,9,-127,127,-1,0),CompactInput.read(ByteArrayInputStream(fixture))!!.state)}
    @Test fun fragmentedAndCoalescedPacketsNeverTreatPayloadNewlinesAsLines(){
        val newline=fixture.copyOf().apply{this[5]=10;this[7]=13}
        val bytes="I 0 0 0 0 0 0\nK\n".toByteArray()+newline+fixture+"I 1 8 -127 127 0 0\n".toByteArray()
        val stream=object:ByteArrayInputStream(bytes){override fun read(b:ByteArray,o:Int,l:Int)=super.read(b,o,minOf(l,1))}
        assertEquals(PhoneState(),CompactInput.read(stream)!!.state);assertNull(CompactInput.read(stream)!!.state)
        assertEquals(PhoneState(0x30FA5,9,10,127,13,0),CompactInput.read(stream)!!.state)
        assertEquals(PhoneState(0x30FA5,9,-127,127,-1,0),CompactInput.read(stream)!!.state)
        assertEquals(PhoneState(1,8,-127,127),CompactInput.read(stream)!!.state);assertNull(CompactInput.read(stream))
    }
    @Test fun truncationAndInvalidMasksAreRejected(){
        fun rejects(b:ByteArray){try{CompactInput.read(ByteArrayInputStream(b));fail("accepted malformed frame")}catch(_:java.io.IOException){}}
        for(size in 1..8)rejects(fixture.copyOf(size))
        for((index,value) in listOf(2 to 16,3 to 4,4 to 16,5 to 128,6 to 128,7 to 128,8 to 128))rejects(fixture.copyOf().apply{this[index]=value.toByte()})
    }
    @Test fun packedPollingPreservesEveryTransitionAndNewestAxes(){
        val q=PhoneInputQueue();assertEquals(0L,q.pollPacked())
        val expected=mutableListOf<PhoneState>();expected+=PhoneState();q.push(PhoneState())
        repeat(12){val s=PhoneState(buttons=1 shl it,dpad=9);q.push(s);q.push(PhoneState());expected+=s;expected+=PhoneState()}
        repeat(10000){assertTrue(q.push(PhoneState(lx=it%128,ry=-127)))}
        for(s in expected)assertEquals(s.copy(lx=9999%128,ry=-127).packed(),q.pollPacked())
        q.reset();assertEquals(-1L,q.pollPacked());assertEquals(0L,q.pollPacked())
    }
}
