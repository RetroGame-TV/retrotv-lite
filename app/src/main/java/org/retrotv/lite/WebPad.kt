// Original RetroTV web pad transport. SPDX-License-Identifier: MIT
package org.retrotv.lite
import java.io.ByteArrayOutputStream
import java.io.EOFException
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.security.MessageDigest
import java.util.Base64

/**
 * Minimal WebSocket server side (RFC 6455) for the browser controller: text frames only, client frames must be masked,
 * no fragmentation, small payloads. Each text frame carries one line of the RetroTV pad protocol.
 */
object WebSocketFrames {
    const val MAX_PAYLOAD=1024
    private const val GUID="258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    fun acceptKey(key:String):String=Base64.getEncoder().encodeToString(MessageDigest.getInstance("SHA-1").digest((key.trim()+GUID).toByteArray()))
    class Frame(val opcode:Int,val payload:ByteArray)
    /** Next frame, or null at end of stream. */
    fun read(input:InputStream):Frame?{
        val b0=input.read();if(b0<0)return null
        val b1=input.read();if(b1<0)throw EOFException()
        if(b0 and 0x80==0)throw IOException("Fragmented frame")
        if(b1 and 0x80==0)throw IOException("Unmasked client frame")
        var length=(b1 and 0x7F).toLong()
        if(length==126L)length=((byte(input) shl 8) or byte(input)).toLong()
        else if(length==127L){length=0;repeat(8){length=(length shl 8) or byte(input).toLong()}}
        if(length>MAX_PAYLOAD)throw IOException("Frame too large")
        val mask=ByteArray(4){byte(input).toByte()}
        val payload=ByteArray(length.toInt()){byte(input).toByte()}
        for(i in payload.indices)payload[i]=(payload[i].toInt() xor mask[i%4].toInt()).toByte()
        return Frame(b0 and 0x0F,payload)
    }
    fun write(out:OutputStream,opcode:Int,payload:ByteArray){
        val header=ByteArrayOutputStream();header.write(0x80 or opcode)
        if(payload.size<126)header.write(payload.size) else {header.write(126);header.write(payload.size shr 8);header.write(payload.size and 0xFF)}
        out.write(header.toByteArray());out.write(payload);out.flush()
    }
    private fun byte(input:InputStream):Int{val b=input.read();if(b<0)throw EOFException();return b}
}

/** Presents a WebSocket as the line streams PhoneControllerBridge already speaks: one text frame per line. */
class WebSocketLines(private val raw:InputStream,private val rawOut:OutputStream){
    private val lock=Any()
    val input:InputStream=object:InputStream(){
        private var buffer=ByteArray(0);private var at=0;private var closed=false
        override fun read():Int{
            while(at>=buffer.size){
                if(closed)return -1
                val frame=WebSocketFrames.read(raw)?:return -1
                when(frame.opcode){
                    1->{buffer=frame.payload+'\n'.code.toByte();at=0}
                    8->{closed=true;synchronized(lock){runCatching{WebSocketFrames.write(rawOut,8,ByteArray(0))}};return -1}
                    9->synchronized(lock){WebSocketFrames.write(rawOut,10,frame.payload)}
                    10->{}
                    else->throw IOException("Unsupported frame")
                }
            }
            return buffer[at++].toInt() and 0xFF
        }
    }
    val output:OutputStream=object:OutputStream(){
        private val line=ByteArrayOutputStream()
        override fun write(b:Int){
            if(b=='\n'.code){val text=line.toByteArray();line.reset();synchronized(lock){WebSocketFrames.write(rawOut,1,text)}}
            else{if(line.size()>=WebSocketFrames.MAX_PAYLOAD)throw IOException("Line too long");line.write(b)}
        }
    }
}

/** Reads an HTTP/1.1 request head (request line + headers), bounded to 8 KiB. */
object HttpHead {
    class Request(val method:String,val path:String,val headers:Map<String,String>)
    fun read(input:InputStream):Request?{
        val lines=mutableListOf<String>();var total=0
        while(true){
            val bytes=ByteArrayOutputStream()
            while(true){val b=input.read();if(b<0)return null;if(b=='\n'.code)break;if(b!='\r'.code)bytes.write(b);if(++total>8192)return null}
            val text=bytes.toString("ISO-8859-1");if(text.isEmpty())break;lines+=text
        }
        val first=lines.firstOrNull()?.split(' ')?.takeIf{it.size>=2}?:return null
        val headers=lines.drop(1).mapNotNull{h->h.indexOf(':').takeIf{it>0}?.let{h.substring(0,it).trim().lowercase() to h.substring(it+1).trim()}}.toMap()
        return Request(first[0],first[1],headers)
    }
}
