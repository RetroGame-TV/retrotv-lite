package org.retrotv.lite

data class PhoneState(val buttons:Int=0,val dpad:Int=0,val lx:Int=0,val ly:Int=0,val rx:Int=0,val ry:Int=0){
    fun nativeLine()="${buttons.toString(16)} $dpad $lx $ly $rx $ry"
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
        val previous=latest?.let{Digital(it.buttons,it.dpad)}?:Digital()
        val next=Digital(state.buttons,state.dpad)
        if(next!=previous){if(pending.size>=capacity)return false;pending.addLast(next)}
        latest=state;return true
    }
    @Synchronized fun poll():String{
        // A reset must not erase states already accepted from the replacement connection.
        if(off){off=false;return "off"}
        val axes=latest?:return ""
        pending.pollFirst()?.let{consumed=it}
        return axes.copy(buttons=consumed.buttons,dpad=consumed.dpad).nativeLine()
    }
    @Synchronized fun reset(){pending.clear();latest=null;consumed=Digital();off=true}
}
