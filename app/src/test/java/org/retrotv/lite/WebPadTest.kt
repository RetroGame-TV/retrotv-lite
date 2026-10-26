package org.retrotv.lite
import org.junit.Assert.*
import org.junit.Test
import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream
import java.io.IOException

class WebPadTest {
    private fun clientFrame(opcode:Int,payload:ByteArray,mask:ByteArray=byteArrayOf(1,2,3,4)):ByteArray{
        val out=ByteArrayOutputStream();out.write(0x80 or opcode)
        if(payload.size<126)out.write(0x80 or payload.size) else {out.write(0x80 or 126);out.write(payload.size shr 8);out.write(payload.size and 0xFF)}
        out.write(mask);payload.forEachIndexed{i,b->out.write(b.toInt() xor mask[i%4].toInt())};return out.toByteArray()
    }
    @Test fun acceptKeyMatchesRfc6455Example(){
        assertEquals("s3pPLMBiTxaQ9kYGzzhZRbK+xOo=",WebSocketFrames.acceptKey("dGhlIHNhbXBsZSBub25jZQ=="))
    }
    @Test fun maskedTextFrameIsDecodedAndBadFramesRejected(){
        val frame=WebSocketFrames.read(ByteArrayInputStream(clientFrame(1,"I 1 0 0 0 0 0".toByteArray())))!!
        assertEquals(1,frame.opcode);assertEquals("I 1 0 0 0 0 0",String(frame.payload))
        assertNull(WebSocketFrames.read(ByteArrayInputStream(ByteArray(0))))
        assertThrows(IOException::class.java){WebSocketFrames.read(ByteArrayInputStream(byteArrayOf(0x81.toByte(),0x01,0x41)))}
        assertThrows(IOException::class.java){WebSocketFrames.read(ByteArrayInputStream(clientFrame(1,ByteArray(2000))))}
    }
    @Test fun linesRideOneTextFrameEachAndPingsAreAnswered(){
        val incoming=clientFrame(9,"p".toByteArray())+clientFrame(1,"H web-1234 -".toByteArray())+clientFrame(8,ByteArray(0))
        val sent=ByteArrayOutputStream()
        val lines=WebSocketLines(ByteArrayInputStream(incoming),sent)
        assertEquals("H web-1234 -",PadWire.readLine(lines.input))
        assertNull(PadWire.readLine(lines.input))
        lines.output.write("OK\n".toByteArray())
        val bytes=sent.toByteArray()
        // pong for the ping, then a close echo, then our text frame "OK"
        assertEquals(0x8A,bytes[0].toInt() and 0xFF);assertEquals('p'.code,bytes[2].toInt())
        assertEquals(0x88,bytes[3].toInt() and 0xFF)
        assertArrayEquals(byteArrayOf(0x81.toByte(),2,'O'.code.toByte(),'K'.code.toByte()),bytes.copyOfRange(5,9))
    }
    @Test fun httpHeadIsParsedAndBounded(){
        val req=HttpHead.read(ByteArrayInputStream("GET /ws HTTP/1.1\r\nHost: tv\r\nUpgrade: websocket\r\nSec-WebSocket-Key: abc==\r\n\r\n".toByteArray()))!!
        assertEquals("GET",req.method);assertEquals("/ws",req.path);assertEquals("websocket",req.headers["upgrade"]);assertEquals("abc==",req.headers["sec-websocket-key"])
        assertNull(HttpHead.read(ByteArrayInputStream(("GET / HTTP/1.1\r\nX: "+"a".repeat(9000)+"\r\n\r\n").toByteArray())))
    }
}
