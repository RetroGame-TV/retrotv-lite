package org.retrotv.lite

import org.junit.Assert.*
import org.junit.Test

class PhoneInputQueueTest{
    @Test fun motionFloodUsesNewestAxesWithoutDelayingButtons(){
        val q=PhoneInputQueue()
        repeat(10000){assertTrue(q.push(PhoneState(lx=it%128)))}
        assertTrue(q.push(PhoneState(buttons=1,lx=90)));assertTrue(q.push(PhoneState(lx=127)))
        assertEquals("1 0 127 0 0 0",q.poll());assertEquals("0 0 127 0 0 0",q.poll())
    }
    @Test fun everyRapidTapAndDigitalDirectionSurvivesMotionBursts(){
        val q=PhoneInputQueue();val expected=mutableListOf<PhoneState>()
        for(i in 1..12){
            val down=PhoneState(buttons=1 shl (i%12),dpad=if(i%2==0)9 else 6)
            expected+=down;expected+=PhoneState()
            assertTrue(q.push(down))
            repeat(300){assertTrue(q.push(down.copy(lx=it%128,ry=-it%128)))}
            assertTrue(q.push(PhoneState()))
        }
        expected.forEach{assertEquals(it.nativeLine(),q.poll())};assertEquals(PhoneState().nativeLine(),q.poll())
    }
    @Test fun heldDirectionsAndChordsDoNotGenerateRepeatedEdges(){
        val q=PhoneInputQueue(2);val chord=PhoneState(buttons=0x301,dpad=9)
        repeat(10000){assertTrue(q.push(chord))}
        repeat(100){assertEquals(chord.nativeLine(),q.poll())}
        assertTrue(q.push(PhoneState(buttons=0x300,dpad=8)));assertTrue(q.push(PhoneState()))
        assertEquals("300 8 0 0 0 0",q.poll());assertEquals("0 0 0 0 0 0",q.poll())
    }
    @Test fun overflowIsExplicitAndDoesNotEvictAnAcceptedEdge(){
        val q=PhoneInputQueue(2)
        assertTrue(q.push(PhoneState(buttons=1)));assertTrue(q.push(PhoneState()))
        assertFalse(q.push(PhoneState(buttons=2)))
        assertEquals("1 0 0 0 0 0",q.poll());assertEquals("0 0 0 0 0 0",q.poll())
        assertTrue(q.push(PhoneState(buttons=2)));assertEquals("2 0 0 0 0 0",q.poll())
    }
    @Test fun disconnectCannotEraseNewConnectionOrReplayOldPresses(){
        val q=PhoneInputQueue();q.push(PhoneState(buttons=1));q.reset();q.push(PhoneState(dpad=8))
        assertEquals("off",q.poll());assertEquals("0 8 0 0 0 0",q.poll());assertEquals("0 8 0 0 0 0",q.poll())
        q.reset();assertEquals("off",q.poll());assertEquals("",q.poll())
    }
    @Test fun producerAndConsumerKeepEveryAcceptedEdgeInOrder(){
        val q=PhoneInputQueue(128);val received=java.util.Collections.synchronizedList(mutableListOf<Int>())
        val error=java.util.concurrent.atomic.AtomicReference<Throwable?>()
        val producer=Thread{try{for(i in 1..100){check(q.push(PhoneState(buttons=i)));repeat(10){check(q.push(PhoneState(buttons=i,lx=it)))}}}catch(t:Throwable){error.set(t)}}
        producer.start()
        val deadline=System.nanoTime()+java.util.concurrent.TimeUnit.SECONDS.toNanos(5)
        while(received.size<100&&System.nanoTime()<deadline){val line=q.poll();if(line.isNotEmpty()){val b=line.substringBefore(' ').toInt(16);if(received.lastOrNull()!=b)received+=b}}
        producer.join(1000);assertNull(error.get());assertEquals((1..100).toList(),received)
    }
    @Test fun protocolIsAsciiInArabicLocale(){
        val original=java.util.Locale.getDefault()
        try{java.util.Locale.setDefault(java.util.Locale.forLanguageTag("ar-EG"))
            assertEquals("201 9 127 -127 0 64",PadWire.parseInput("I 201 9 127 -127 0 64"))
        }finally{java.util.Locale.setDefault(original)}
    }
}
