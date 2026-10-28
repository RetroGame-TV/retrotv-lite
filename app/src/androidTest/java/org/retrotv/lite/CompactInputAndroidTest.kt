package org.retrotv.lite
import android.os.Bundle
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import java.net.Socket
import java.io.ByteArrayInputStream

@RunWith(AndroidJUnit4::class)
class CompactInputAndroidTest{
    private val context get()=InstrumentationRegistry.getInstrumentation().targetContext
    private fun frame(s:PhoneState)=byteArrayOf(0xF1.toByte(),s.buttons.toByte(),(s.buttons ushr 8).toByte(),(s.buttons ushr 16).toByte(),s.dpad.toByte(),s.lx.toByte(),s.ly.toByte(),s.rx.toByte(),s.ry.toByte())
    @Test fun binaryFloodThroughRealReceiverKeeps24EdgesAndMetadataOnlyChanges(){
        val prefs=context.getSharedPreferences("phone_wifi",0);val id="compact-test-phone";val previous=prefs.getString(id,null);prefs.edit().putString(id,"local-test-token").commit()
        val bridge=PhoneControllerBridge(context){}
        try{assertTrue(bridge.start());Socket("127.0.0.1",PhoneControllerBridge.WIFI_PORT).use{s->
            s.soTimeout=3000;val out=s.getOutputStream();val input=s.getInputStream().bufferedReader()
            out.write("H $id local-test-token\nI 0 0 0 0 0 0\n".toByteArray());out.flush();assertEquals("OK",input.readLine());assertEquals(1,org.json.JSONObject(input.readLine()).getInt("inputBinary"))
            s.soTimeout=1200;try{input.readLine();fail("redundant context")}catch(_:java.net.SocketTimeoutException){}
            bridge.publish("SNES\tStreet Fighter test\tplaying");assertEquals("SNES",org.json.JSONObject(input.readLine()).getString("system"))
            val expected=mutableListOf<Int>();val burst=java.io.ByteArrayOutputStream()
            repeat(12){val down=PhoneState(buttons=1 shl it,dpad=9);expected+=down.buttons;expected+=0;burst.write(frame(down))
                repeat(1000){i->burst.write(frame(down.copy(lx=i%128,ry=-(i%128))))};burst.write(frame(PhoneState()))}
            out.write(burst.toByteArray());out.flush();val received=mutableListOf<Int>();var last=0;val until=System.nanoTime()+10_000_000_000L
            while(received.size<24&&System.nanoTime()<until){val v=bridge.pollPacked();if(v>0){val b=(v and 0x30FFF).toInt();if(b!=last){received+=b;last=b}};Thread.sleep(1)}
            assertEquals(expected,received);assertEquals(0,bridge.pollPacked().toInt() and 0x30FFF)
        }}finally{bridge.stop();prefs.edit().apply{if(previous==null)remove(id) else putString(id,previous)}.commit()}
    }
    @Test fun receiverAndJniQueueCpuBenchmark(){
        val states=List(256){PhoneState(it,it%16,it%255-127,127,-1,0)}
        val text=states.map{("I "+it.nativeLine()+"\n").toByteArray()};val binary=states.map(::frame);var sum=0
        fun parse(compact:Boolean):Long{val start=System.nanoTime();repeat(5000){val s=CompactInput.read(ByteArrayInputStream((if(compact)binary else text)[it%256]))!!.state!!;sum=sum xor s.buttons};return (System.nanoTime()-start)/5000}
        val stringQueue=PhoneInputQueue().apply{push(PhoneState(0x301,9,-127,127,-1,0))};val packedQueue=PhoneInputQueue().apply{push(PhoneState(0x301,9,-127,127,-1,0))}
        fun poll(packed:Boolean):Long{val start=System.nanoTime();repeat(5000){sum=sum xor if(packed)packedQueue.pollPacked().toInt() else stringQueue.poll().hashCode()};return (System.nanoTime()-start)/5000}
        repeat(4){parse(false);parse(true);poll(false);poll(true)}
        val a=mutableListOf<Long>();val b=mutableListOf<Long>();val c=mutableListOf<Long>();val d=mutableListOf<Long>()
        repeat(7){a+=parse(false);b+=parse(true);c+=poll(false);d+=poll(true)}
        InstrumentationRegistry.getInstrumentation().sendStatus(0,Bundle().apply{putString("stream","Android CPU ns/state median: decode text=${a.sorted()[3]}, binary=${b.sorted()[3]}; queue text=${c.sorted()[3]}, packed=${d.sorted()[3]}; checksum=$sum. Excludes radio/rendering.\n")})
    }
    @Test fun overflowClosesBeforeBufferedTailCanReactivateOldButtons(){
        val prefs=context.getSharedPreferences("phone_wifi",0);val id="compact-overflow-test";val previous=prefs.getString(id,null);prefs.edit().putString(id,"overflow-local-token").commit()
        val bridge=PhoneControllerBridge(context){}
        try{assertTrue(bridge.start())
            Socket("127.0.0.1",PhoneControllerBridge.WIFI_PORT).use{s->
                s.soTimeout=5000;val out=s.getOutputStream();val input=s.getInputStream().buffered(8192)
                out.write("H $id overflow-local-token\n".toByteArray());out.flush();assertEquals("OK",PadWire.readLine(input));assertTrue(PadWire.readLine(input)!!.startsWith("{"))
                val burst=java.io.ByteArrayOutputStream();repeat(120){burst.write(frame(PhoneState(buttons=if(it%2==0)1 else 0)))}
                runCatching{out.write(burst.toByteArray());out.flush()}
                assertEquals(-1,input.read())
            }
            assertEquals(-1L,bridge.pollPacked());assertEquals(0L,bridge.pollPacked())
            Socket("127.0.0.1",PhoneControllerBridge.WIFI_PORT).use{s->
                s.soTimeout=5000;val out=s.getOutputStream();val input=s.getInputStream().buffered(8192)
                out.write("H $id overflow-local-token\n".toByteArray()+frame(PhoneState(dpad=8)));out.flush();assertEquals("OK",PadWire.readLine(input));PadWire.readLine(input)
                val deadline=System.nanoTime()+5_000_000_000L;var value=0L
                while(value==0L&&System.nanoTime()<deadline){value=bridge.pollPacked();Thread.sleep(1)}
                assertEquals(PhoneState(dpad=8).packed(),value)
            }
        }finally{bridge.stop();prefs.edit().apply{if(previous==null)remove(id) else putString(id,previous)}.commit()}
    }
}
