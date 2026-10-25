// Original RetroTV metadata bridge. SPDX-License-Identifier: MIT
package org.retrotv.lite
import android.annotation.SuppressLint
import android.bluetooth.*
import android.content.Context
import org.json.JSONObject
import java.util.UUID
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicReference

/** Optional secure RFCOMM metadata only; game input remains Android Bluetooth HID. */
@SuppressLint("MissingPermission")
class PhoneControllerBridge(private val context:Context,private val failed:()->Unit){
    companion object { const val SERVICE_UUID="8a421b90-d104-4ba2-8c72-50a34191ce01" }
    private val generation=AtomicInteger(0)
    private val message=AtomicReference("{\"version\":1,\"system\":\"\",\"title\":\"\",\"mode\":\"library\"}")
    private val clients=ConcurrentHashMap<BluetoothSocket,Boolean>()
    @Volatile private var server:BluetoothServerSocket?=null
    @Volatile private var running=false
    @Volatile var status="Sincronia do celular desativada"
        private set
    internal fun currentPacket():String=message.get()
    fun publish(value:String){
        val parts=value.split('\t',limit=3);if(parts.size!=3)return
        message.set(JSONObject().put("version",1).put("system",parts[0].take(64)).put("title",parts[1].take(160)).put("mode",parts[2].take(24)).toString())
    }
    fun start():Boolean {
        if(running)return true
        val adapter=context.getSystemService(BluetoothManager::class.java)?.adapter
        if(adapter==null){status="Bluetooth indisponível nesta TV";return false}
        try{if(!adapter.isEnabled){status="Ative o Bluetooth da TV primeiro";return false}}catch(_:SecurityException){status="Permissão Bluetooth necessária";return false}
        val token=generation.incrementAndGet();running=true;status="Aguardando RetroTV Pad por Bluetooth"
        Thread({
            var local:BluetoothServerSocket?=null
            try{
                local=adapter.listenUsingRfcommWithServiceRecord("RetroTV Console",UUID.fromString(SERVICE_UUID))
                if(token!=generation.get()){local.close();return@Thread}
                server=local
                while(token==generation.get()){
                    val peer=local.accept()
                    if(token!=generation.get()||peer.remoteDevice.bondState!=BluetoothDevice.BOND_BONDED||clients.size>=2){peer.close();continue}
                    clients[peer]=true
                    Thread({
                        try{
                            val out=peer.outputStream
                            while(token==generation.get()){
                                val bytes=(message.get()+"\n").toByteArray(Charsets.UTF_8)
                                require(bytes.size<=2049);out.write(bytes);out.flush()
                                Thread.sleep(1000)
                            }
                        }catch(_:Exception){}finally{clients.remove(peer);runCatching{peer.close()}}
                    },"RetroTV-PhoneState").apply{isDaemon=true;start()}
                }
            }catch(_:Exception){if(token==generation.get()){running=false;status="Não foi possível abrir a sincronia Bluetooth. Tente ativar novamente.";failed()}}
            finally{runCatching{local?.close()}}
        },"RetroTV-PhoneAccept").apply{isDaemon=true;start()}
        return true
    }
    fun stop(){generation.incrementAndGet();running=false;runCatching{server?.close()};server=null;clients.keys.forEach{runCatching{it.close()}};clients.clear();status="Sincronia do celular desativada"}
}