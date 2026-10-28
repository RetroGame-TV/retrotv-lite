package org.retrotv.lite

import java.io.InputStream
import java.io.IOException
import java.io.EOFException

/** Mixed legacy lines and negotiated fixed-length binary states, on the same reliable stream. */
object CompactInput {
    const val MARKER=0xF1
    data class Packet(val state:PhoneState?)
    fun read(input:InputStream):Packet?{
        val first=input.read();if(first<0)return null
        if(first!=MARKER)return Packet(PadWire.readLine(input,first)?.let(PadWire::parseState))
        val b=ByteArray(8);var offset=0
        while(offset<b.size){val n=input.read(b,offset,b.size-offset);if(n<0)throw EOFException("Truncated input state");if(n==0)throw IOException("Empty stream read");offset+=n}
        val buttons=(b[0].toInt() and 255) or ((b[1].toInt() and 255) shl 8) or ((b[2].toInt() and 255) shl 16)
        val dpad=b[3].toInt() and 255
        if(buttons and 0x30FFF!=buttons||dpad>15||(4..7).any{b[it].toInt()==-128})throw IOException("Invalid input state")
        return Packet(PhoneState(buttons,dpad,b[4].toInt(),b[5].toInt(),b[6].toInt(),b[7].toInt()))
    }
}
