package org.retrotv.lite

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import java.net.Socket

@RunWith(AndroidJUnit4::class)
class PhoneInputAndroidTest{
    private val context get()=InstrumentationRegistry.getInstrumentation().targetContext
    private fun wire(state:PhoneState)="I ${state.nativeLine()}"
    @Test fun bridgePreservesAllButtonsHomeBackDirectionsAndChordsAcrossMotionBursts(){
        val bridge=PhoneControllerBridge(context){};val peer=Any();val expected=mutableListOf<PhoneState>()
        val held=(0..11).map{PhoneState(buttons=1 shl it)}+listOf(PhoneState(buttons=1 shl 16),PhoneState(buttons=1 shl 17),PhoneState(buttons=0x301,dpad=9))+
            listOf(1,9,8,10,2,6,4,5).map{PhoneState(dpad=it)}
        for(down in held){
            assertTrue(bridge.acceptInput(peer,wire(down)))
            repeat(50){assertTrue(bridge.acceptInput(peer,wire(down.copy(lx=it,ry=-it))))}
            assertTrue(bridge.acceptInput(peer,wire(PhoneState())))
            expected+=down;expected+=PhoneState()
        }
        assertFalse(bridge.acceptInput(peer,"I invalid 0 0 0 0 0"))
        expected.forEach{assertEquals(it.nativeLine(),bridge.pollInput())}
        assertEquals(PhoneState().nativeLine(),bridge.pollInput());bridge.stop();assertEquals("off",bridge.pollInput())
    }
    @Test fun saturatedConnectionClosesExplicitlyAndNewConnectionCannotReplayOldEdges(){
        class Peer:AutoCloseable{var closed=false;override fun close(){closed=true}}
        val bridge=PhoneControllerBridge(context){};val peer=Peer()
        for(i in 1..64)assertTrue(bridge.acceptInput(peer,wire(PhoneState(buttons=i))))
        assertFalse(bridge.acceptInput(peer,wire(PhoneState())));assertTrue(peer.closed)
        val next=Peer();assertTrue(bridge.acceptInput(next,wire(PhoneState(dpad=8))))
        assertEquals("off",bridge.pollInput());assertEquals("0 8 0 0 0 0",bridge.pollInput())
        bridge.stop();assertEquals("off",bridge.pollInput());assertEquals("",bridge.pollInput())
    }
    @Test fun realTcpReceiverKeepsEveryQuickTapDuringAxisFlood(){
        val prefs=context.getSharedPreferences("phone_wifi",0);val id="latency-instrumentation-test"
        val previous=prefs.getString(id,null);val token="test-only-local-token"
        prefs.edit().putString(id,token).commit()
        val bridge=PhoneControllerBridge(context){}
        try{
            assertTrue(bridge.start())
            Socket("127.0.0.1",PhoneControllerBridge.WIFI_PORT).use{socket->
                socket.soTimeout=5000;val output=socket.getOutputStream();val input=socket.getInputStream().bufferedReader()
                // Coalesce auth + the initial state in one TCP write; buffered pairing must not swallow input.
                output.write("H $id $token\nI 0 0 0 0 0 0\n".toByteArray());output.flush();assertEquals("OK",input.readLine())
                val expected=mutableListOf<Int>();val burst=StringBuilder()
                repeat(12){i->
                    val state=PhoneState(buttons=1 shl (i%12));expected+=state.buttons;expected+=0
                    burst.append(wire(state)).append('\n')
                    repeat(100){burst.append(wire(state.copy(lx=it,ry=-it))).append('\n')}
                    burst.append(wire(PhoneState())).append('\n')
                }
                output.write(burst.toString().toByteArray());output.flush()
                val received=mutableListOf<Int>();var last=0;val deadline=System.nanoTime()+java.util.concurrent.TimeUnit.SECONDS.toNanos(10)
                while(received.size<expected.size&&System.nanoTime()<deadline){
                    val line=bridge.pollInput()
                    if(line.isNotEmpty()&&line!="off"){val buttons=line.substringBefore(' ').toInt(16);if(buttons!=last){received+=buttons;last=buttons}}
                    Thread.sleep(2)
                }
                assertEquals(expected,received)
            }
        }finally{bridge.stop();val edit=prefs.edit();if(previous==null)edit.remove(id) else edit.putString(id,previous);edit.commit()}
    }
}
