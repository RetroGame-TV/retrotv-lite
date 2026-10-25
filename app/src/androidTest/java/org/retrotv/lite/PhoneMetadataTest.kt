package org.retrotv.lite
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.json.JSONObject
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
@RunWith(AndroidJUnit4::class)
class PhoneMetadataTest {
    @Test fun metadataKeepsUtf8EscapesAndBoundsWithoutStartingBluetooth(){
        val bridge=PhoneControllerBridge(InstrumentationRegistry.getInstrumentation().targetContext){}
        bridge.publish("SNES\tAção \"especial\" 🎮\tpaused")
        val packet=bridge.currentPacket()
        val json=JSONObject(packet)
        assertEquals(1,json.getInt("version"));assertEquals("SNES",json.getString("system"))
        assertEquals("Ação \"especial\" 🎮",json.getString("title"));assertEquals("paused",json.getString("mode"))
        bridge.publish("bad input");assertEquals(packet,bridge.currentPacket())
        bridge.publish("PlayStation\t"+"x".repeat(10000)+"\trunning")
        assertEquals(160,JSONObject(bridge.currentPacket()).getString("title").length)
        assertTrue((bridge.currentPacket()+"\n").toByteArray().size<=2049)
        assertEquals("8a421b90-d104-4ba2-8c72-50a34191ce01",PhoneControllerBridge.SERVICE_UUID)
    }
}