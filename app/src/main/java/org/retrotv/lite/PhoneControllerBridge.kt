// Original RetroTV phone bridge. SPDX-License-Identifier: MIT
package org.retrotv.lite
import android.annotation.SuppressLint
import android.bluetooth.*
import android.content.Context
import android.net.ConnectivityManager
import android.net.nsd.NsdManager
import android.net.nsd.NsdServiceInfo
import android.os.Build
import org.json.JSONObject
import java.io.InputStream
import java.io.OutputStream
import java.net.InetSocketAddress
import java.net.ServerSocket
import java.net.Socket
import java.security.SecureRandom
import java.util.UUID
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicReference

/** Line protocol shared with RetroTV Pad. Input lines carry only controller state; nothing is executed. */
object PadWire {
    const val MAX_LINE=256
    /** "I <buttons hex> <dpad> <lx> <ly> <rx> <ry>" with axes in -127..127; returns the native form or null. */
    fun parseState(line:String):PhoneState?{
        val p=line.split(' ');if(p.size!=7||p[0]!="I")return null
        // Bits 0-11: gamepad buttons; bit 16: Home; bit 17: one step back (bits 12-15 stay free, they are the d-pad in HID).
        val buttons=p[1].toIntOrNull(16)?.takeIf{it in 0..0x30FFF&&(it and 0xF000)==0}?:return null
        val dpad=p[2].toIntOrNull()?.takeIf{it in 0..15}?:return null
        val axes=p.drop(3).map{it.toIntOrNull()?.coerceIn(-127,127)?:return null}
        return PhoneState(buttons,dpad,axes[0],axes[1],axes[2],axes[3])
    }
    fun parseInput(line:String):String?=parseState(line)?.nativeLine()
    fun readLine(input:InputStream):String?{
        val bytes=java.io.ByteArrayOutputStream()
        while(true){val b=input.read();if(b==-1)return if(bytes.size()==0)null else throw java.io.EOFException("Truncated line")
            if(b==10)return bytes.toString("UTF-8");if(bytes.size()>=MAX_LINE)throw java.io.IOException("Line too long");bytes.write(b)}
    }
    fun validId(value:String)=value.matches(Regex("[A-Za-z0-9-]{4,40}"))
}

/**
 * Opt-in phone bridge. Bluetooth: secure RFCOMM, paired devices only. Wi-Fi: local TCP announced by NSD; a phone is
 * accepted after typing the 4-digit code shown on the TV and then keeps a random token. Both send console context to
 * the phone and receive controller state that native code feeds into a virtual game controller.
 */
@SuppressLint("MissingPermission")
class PhoneControllerBridge(private val context:Context,private val failed:()->Unit){
    companion object { const val SERVICE_UUID="8a421b90-d104-4ba2-8c72-50a34191ce01";const val NSD_TYPE="_retrotv._tcp.";const val WIFI_PORT=47800;const val WEB_PORT=47801 }
    private val generation=AtomicInteger(0)
    private val message=AtomicReference("{\"version\":1,\"system\":\"\",\"title\":\"\",\"mode\":\"library\"}")
    private val clients=ConcurrentHashMap<Any,Boolean>()
    private val tokens by lazy{context.getSharedPreferences("phone_wifi",Context.MODE_PRIVATE)}
    private val random=SecureRandom()
    @Volatile private var server:BluetoothServerSocket?=null
    @Volatile private var tcp:ServerSocket?=null
    @Volatile private var web:ServerSocket?=null
    @Volatile private var nsd:NsdManager.RegistrationListener?=null
    @Volatile private var running=false
    private val states=PhoneInputQueue()
    @Volatile private var owner:Any?=null
    @Volatile private var code:String?=null
    @Volatile private var codeUntil=0L
    @Volatile var status="Sincronia do celular desativada"
        private set
    internal fun currentPacket():String=message.get()
    fun publish(value:String){
        val parts=value.split('\t',limit=3);if(parts.size!=3)return
        message.set(JSONObject().put("version",1).put("system",parts[0].take(64)).put("title",parts[1].take(160)).put("mode",parts[2].take(24)).toString())
    }
    /** Native polling: current state, "off" once after the controlling phone disconnects, or "" when idle. */
    fun pollInput():String=states.poll()
    /** True while a phone is connected (or authorizing); native polls input every frame only then. */
    fun linked():Boolean=clients.isNotEmpty()
    /** Code to show on screen while a phone is pairing over Wi-Fi, or "". */
    fun overlay():String{val c=code;return if(c!=null&&System.currentTimeMillis()<codeUntil)c else ""}
    @Synchronized internal fun acceptInput(from:Any,line:String):Boolean{
        val state=PadWire.parseState(line)?:return false
        if(owner!=null&&owner!==from)states.reset()
        if(!states.push(state)){
            // Never silently discard a press/release when a producer exceeds frame consumption.
            owner=null;states.reset();runCatching{(from as? AutoCloseable)?.close()}
            status="Fila do controle cheia. Reconectando o celular…";return false
        }
        owner=from;return true
    }
    @Synchronized private fun release(from:Any){if(owner===from){owner=null;states.reset()}}
    fun start():Boolean {
        if(running)return true
        val token=generation.incrementAndGet();running=true
        val bluetooth=startBluetooth(token);val wifi=startWifi(token);val browser=wifi&&startWeb(token)
        if(!bluetooth&&!wifi){running=false;return false}
        status=listOfNotNull(if(bluetooth)"Bluetooth" else null,if(wifi)"Wi-Fi "+(address()?.let{"$it:$WIFI_PORT"}?:"") else null).joinToString(" e ",prefix="Aguardando RetroTV Pad por ")+
            (if(browser)address()?.let{"\niPhone ou navegador: http://$it:$WEB_PORT"}?:"" else "")
        return true
    }
    private fun startBluetooth(token:Int):Boolean{
        val adapter=context.getSystemService(BluetoothManager::class.java)?.adapter?:return false
        if(Build.VERSION.SDK_INT>=31&&context.checkSelfPermission(android.Manifest.permission.BLUETOOTH_CONNECT)!=android.content.pm.PackageManager.PERMISSION_GRANTED)return false
        try{if(!adapter.isEnabled)return false}catch(_:SecurityException){return false}
        Thread({
            var local:BluetoothServerSocket?=null
            try{
                local=adapter.listenUsingRfcommWithServiceRecord("RetroTV Console",UUID.fromString(SERVICE_UUID))
                if(token!=generation.get()){local.close();return@Thread}
                server=local
                while(token==generation.get()){
                    val peer=local.accept()
                    if(token!=generation.get()||peer.remoteDevice.bondState!=BluetoothDevice.BOND_BONDED||clients.size>=3){peer.close();continue}
                    clients[peer]=true
                    Thread({try{serve(token,peer,peer.inputStream.buffered(8192),peer.outputStream)}finally{clients.remove(peer);release(peer);runCatching{peer.close()}}},"RetroTV-PhoneBt").apply{isDaemon=true;start()}
                }
            }catch(_:Exception){if(token==generation.get()&&tcp==null){running=false;status="Não foi possível abrir a sincronia do celular. Tente ativar novamente.";failed()}}
            finally{runCatching{local?.close()}}
        },"RetroTV-PhoneAccept").apply{isDaemon=true;start()}
        return true
    }
    private fun startWifi(token:Int):Boolean{
        val socket=try{ServerSocket().apply{reuseAddress=true;bind(InetSocketAddress(WIFI_PORT),4)}}catch(_:Exception){return false}
        tcp=socket
        val listener=object:NsdManager.RegistrationListener{
            override fun onServiceRegistered(info:NsdServiceInfo){}
            override fun onRegistrationFailed(info:NsdServiceInfo,error:Int){}
            override fun onServiceUnregistered(info:NsdServiceInfo){}
            override fun onUnregistrationFailed(info:NsdServiceInfo,error:Int){}
        }
        runCatching{context.getSystemService(NsdManager::class.java)?.registerService(NsdServiceInfo().apply{serviceName="RetroTV "+Build.MODEL.take(40);serviceType=NSD_TYPE;port=WIFI_PORT},NsdManager.PROTOCOL_DNS_SD,listener);nsd=listener}
        Thread({
            try{
                while(token==generation.get()){
                    val peer=socket.accept()
                    if(token!=generation.get()||clients.size>=3){peer.close();continue}
                    peer.tcpNoDelay=true;peer.soTimeout=15000;clients[peer]=true
                    Thread({try{
                        // Reuse one buffer across pairing and play: a read may include the first input after H/P.
                        val input=peer.getInputStream().buffered(8192)
                        if(authorize(input,peer.getOutputStream()){peer.soTimeout=it})serve(token,peer,input,peer.getOutputStream())
                    }catch(_:Exception){}finally{clients.remove(peer);release(peer);runCatching{peer.close()}}},"RetroTV-PhoneWifi").apply{isDaemon=true;start()}
                }
            }catch(_:Exception){}finally{runCatching{socket.close()}}
        },"RetroTV-WifiAccept").apply{isDaemon=true;start()}
        return true
    }
    /**
     * Browser controller (iPhone or any phone, no app): GET / serves the page from assets/web/pad.html; GET /ws upgrades
     * to a WebSocket that carries the same lines as the TCP channel, with the same code pairing and tokens.
     */
    private fun startWeb(token:Int):Boolean{
        val socket=try{ServerSocket().apply{reuseAddress=true;bind(InetSocketAddress(WEB_PORT),4)}}catch(_:Exception){return false}
        web=socket
        Thread({
            try{
                while(token==generation.get()){
                    val peer=socket.accept()
                    if(token!=generation.get()){peer.close();continue}
                    peer.tcpNoDelay=true;peer.soTimeout=15000
                    Thread({try{webClient(token,peer)}catch(_:Exception){}finally{clients.remove(peer);release(peer);runCatching{peer.close()}}},"RetroTV-PhoneWeb").apply{isDaemon=true;start()}
                }
            }catch(_:Exception){}finally{runCatching{socket.close()}}
        },"RetroTV-WebAccept").apply{isDaemon=true;start()}
        return true
    }
    private fun webClient(token:Int,peer:Socket){
        val input=peer.getInputStream();val out=peer.getOutputStream()
        val request=HttpHead.read(input)?:return
        val path=request.path.substringBefore('?')
        fun reply(status:String,type:String,body:ByteArray){
            out.write("HTTP/1.1 $status\r\nContent-Type: $type\r\nContent-Length: ${body.size}\r\nCache-Control: no-store\r\nConnection: close\r\n\r\n".toByteArray());out.write(body);out.flush()
        }
        when{
            request.method!="GET"->reply("405 Method Not Allowed","text/plain","".toByteArray())
            path=="/ws"&&request.headers["upgrade"]?.equals("websocket",true)==true->{
                val key=request.headers["sec-websocket-key"]?:return reply("400 Bad Request","text/plain","".toByteArray())
                if(clients.size>=3)return reply("503 Service Unavailable","text/plain","".toByteArray())
                out.write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ${WebSocketFrames.acceptKey(key)}\r\n\r\n".toByteArray());out.flush()
                clients[peer]=true
                val lines=WebSocketLines(input,out)
                if(authorize(lines.input,lines.output){peer.soTimeout=it})serve(token,peer,lines.input,lines.output)
            }
            path=="/"||path=="/index.html"->reply("200 OK","text/html; charset=utf-8",context.assets.open("web/pad.html").use{it.readBytes()})
            else->reply("404 Not Found","text/plain","".toByteArray())
        }
    }
    /** "H <phone id> <token|->": a known token is accepted; otherwise the TV shows a code and expects "P <code>" (three tries). */
    private fun authorize(input:InputStream,out:OutputStream,timeout:(Int)->Unit):Boolean{
        val hello=PadWire.readLine(input)?.split(' ')?:return false
        if(hello.size!=3||hello[0]!="H"||!PadWire.validId(hello[1]))return false
        val known=tokens.getString(hello[1],null)
        if(known!=null&&known==hello[2]){out.write("OK\n".toByteArray());out.flush();return true}
        val shown="%04d".format(random.nextInt(10000));code=shown;codeUntil=System.currentTimeMillis()+90000
        try{
            out.write("C\n".toByteArray());out.flush();timeout(90000)
            repeat(3){
                val answer=PadWire.readLine(input)?.split(' ')?:return false
                if(answer.size==2&&answer[0]=="P"&&answer[1]==shown){
                    val secret=ByteArray(16).also(random::nextBytes).joinToString(""){"%02x".format(it)}
                    val edit=tokens.edit();if(tokens.all.size>=8)tokens.all.keys.firstOrNull()?.let(edit::remove);edit.putString(hello[1],secret).apply()
                    out.write("T $secret\n".toByteArray());out.flush();return true
                }
                out.write("R\n".toByteArray());out.flush()
            }
            return false
        }finally{if(code==shown)code=null;timeout(15000)}
    }
    /** Sends console context every second and reads controller lines until the phone leaves; "K" lines only keep it alive. */
    private fun serve(token:Int,peer:Any,input:InputStream,out:OutputStream){
        val writer=Thread({
            try{while(token==generation.get()){synchronized(out){out.write((message.get()+"\n").toByteArray(Charsets.UTF_8));out.flush()};Thread.sleep(1000)}}
            catch(_:Exception){}finally{runCatching{(peer as? AutoCloseable)?.close()}}
        },"RetroTV-PhoneState").apply{isDaemon=true;start()}
        try{while(token==generation.get()){val line=PadWire.readLine(input)?:break;if(token!=generation.get())break;if(line.startsWith("I "))acceptInput(peer,line)}}
        catch(_:Exception){}finally{writer.interrupt()}
    }
    private fun address():String?=runCatching{
        val manager=context.getSystemService(ConnectivityManager::class.java)
        manager.getLinkProperties(manager.activeNetwork)?.linkAddresses?.map{it.address}?.firstOrNull{it is java.net.Inet4Address&&!it.isLoopbackAddress}?.hostAddress
    }.getOrNull()
    fun stop(){
        generation.incrementAndGet();running=false
        runCatching{server?.close()};server=null;runCatching{tcp?.close()};tcp=null;runCatching{web?.close()};web=null
        nsd?.let{l->runCatching{context.getSystemService(NsdManager::class.java)?.unregisterService(l)}};nsd=null
        clients.keys.forEach{runCatching{(it as? AutoCloseable)?.close()}};clients.clear()
        owner=null;states.reset();code=null;status="Sincronia do celular desativada"
    }
}
