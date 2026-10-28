package org.retrotv.lite

data class PhoneState(val buttons:Int=0,val dpad:Int=0,val lx:Int=0,val ly:Int=0,val rx:Int=0,val ry:Int=0){
    fun nativeLine()="${buttons.toString(16)} $dpad $lx $ly $rx $ry"
    fun packed(buttons:Int=this.buttons,dpad:Int=this.dpad):Long{
        fun axis(value:Int,shift:Int)=(value.coerceIn(-127,127).toLong() and 255L) shl shift
        return (1L shl 54) or (buttons.toLong() and 0x30FFFL) or ((dpad.toLong() and 15L) shl 18) or axis(lx,22) or axis(ly,30) or axis(rx,38) or axis(ry,46)
    }
}

/** One digital transition per frame, newest axes on every frame. Motion/keepalives never fill the edge queue.
 * Returns false on saturation so the bridge can disconnect and neutralize rather than silently lose commands.
 */
class PhoneInputQueue(private val capacity:Int=64){
    private data class Digital(val buttons:Int=0,val dpad:Int=0)
    private val pending=java.util.ArrayDeque<Digital>()
    private var latest:PhoneState?=null
    private var consumed=Digital()
    private var off=false
    init{require(capacity>0)}
    @Synchronized fun push(state:PhoneState):Boolean{
        val previous=latest
        // Keep the connection's initial snapshot: SDL must attach/open the virtual pad before its next press event.
        if(previous==null||previous.buttons!=state.buttons||previous.dpad!=state.dpad){if(pending.size>=capacity)return false;pending.addLast(Digital(state.buttons,state.dpad))}
        latest=state;return true
    }
    @Synchronized fun poll():String{
        // A reset must not erase states already accepted from the replacement connection.
        if(off){off=false;return "off"}
        val axes=latest?:return ""
        pending.pollFirst()?.let{consumed=it}
        return axes.copy(buttons=consumed.buttons,dpad=consumed.dpad).nativeLine()
    }
    /** Same queue, without strings, copies or numeric parsing in the per-frame JNI path. */
    @Synchronized fun pollPacked():Long{
        if(off){off=false;return -1L}
        val axes=latest?:return 0L
        pending.pollFirst()?.let{consumed=it}
        return axes.packed(consumed.buttons,consumed.dpad)
    }
    @Synchronized fun reset(){pending.clear();latest=null;consumed=Digital();off=true}
}
