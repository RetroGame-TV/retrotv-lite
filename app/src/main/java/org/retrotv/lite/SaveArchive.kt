package org.retrotv.lite

import java.io.File
import java.io.InputStream
import java.io.OutputStream
import java.nio.file.Files
import java.nio.file.StandardCopyOption
import java.security.MessageDigest
import java.util.zip.ZipEntry
import java.util.zip.ZipInputStream
import java.util.zip.ZipOutputStream

/** Local save backup, validated in staging before replacing the private directory. */
object SaveArchive {
    private const val MAX_FILE = 34L * 1024 * 1024
    private const val MAX_TOTAL = 128L * 1024 * 1024
    private fun safe(name: String): Boolean = name.length in 1..240 && name.matches(Regex("[A-Za-z0-9_./-]+")) &&
        !name.startsWith('/') && name.split('/').none { it.isEmpty() || it == "." || it == ".." }
    private fun digest(file: File): String {
        val md = MessageDigest.getInstance("SHA-256")
        file.inputStream().use { input -> val b = ByteArray(65536); while (true) { val n = input.read(b); if (n < 0) break; md.update(b, 0, n) } }
        return md.digest().joinToString("") { "%02x".format(it) }
    }
    fun export(root: File, output: OutputStream) {
        root.mkdirs()
        val files = root.walkTopDown().filter { it.isFile && !it.name.endsWith(".tmp") }.toList()
        require(files.size <= 4096) { "Muitos arquivos de save" }
        require(files.sumOf { it.length() } <= MAX_TOTAL) { "Backup excede 128 MiB" }
        ZipOutputStream(output).use { zip ->
            val manifest = StringBuilder("RETROTV-BACKUP-1\n")
            for (file in files) {
                val name = file.relativeTo(root).invariantSeparatorsPath
                require(safe(name) && file.length() <= MAX_FILE && !Files.isSymbolicLink(file.toPath())) { "Nome/tamanho de save inválido" }
                zip.putNextEntry(ZipEntry(name)); file.inputStream().use { it.copyTo(zip) }; zip.closeEntry()
                manifest.append(digest(file)).append('\t').append(name).append('\n')
            }
            zip.putNextEntry(ZipEntry("backup.manifest")); zip.write(manifest.toString().toByteArray()); zip.closeEntry()
        }
    }
    fun restore(root: File, input: InputStream) {
        val parent = root.canonicalFile.parentFile ?: error("Diretório inválido")
        require(root.name == "saves") { "Destino deve ser saves privado" }
        val stage = File(parent, "saves.restore-stage")
        val previous = File(parent, "saves.previous")
        require(!stage.exists()) { "Restauração anterior incompleta: preserve arquivos e consulte diagnóstico" }
        stage.mkdirs()
        var total = 0L
        val names = HashSet<String>()
        try {
            ZipInputStream(input).use { zip ->
                while (true) {
                    val entry = zip.nextEntry ?: break
                    require(!entry.isDirectory && safe(entry.name) && names.add(entry.name) && names.size <= 4097) { "Entrada de backup inválida/duplicada" }
                    val file = File(stage, entry.name)
                    require(file.canonicalPath.startsWith(stage.canonicalPath + File.separator)) { "Caminho fora do backup" }
                    file.parentFile?.mkdirs()
                    var size = 0L
                    file.outputStream().use { output ->
                        val b = ByteArray(65536)
                        while (true) {
                            val n = zip.read(b); if (n < 0) break
                            size += n; total += n
                            require(size <= (if (entry.name == "backup.manifest") 512 * 1024L else MAX_FILE) && total <= MAX_TOTAL) { "Backup excede limite" }
                            output.write(b, 0, n)
                        }
                        output.fd.sync()
                    }
                    zip.closeEntry()
                }
            }
            val manifest = File(stage, "backup.manifest")
            require(manifest.isFile) { "Manifest ausente" }
            val lines = manifest.readLines()
            require(lines.firstOrNull() == "RETROTV-BACKUP-1") { "Versão de backup inválida" }
            val expected = HashMap<String, String>()
            for (line in lines.drop(1)) {
                if (line.isBlank()) continue
                val parts = line.split('\t')
                require(parts.size == 2 && parts[0].matches(Regex("[a-f0-9]{64}")) && safe(parts[1]) && parts[1] != "backup.manifest" && expected.put(parts[1], parts[0]) == null) { "Manifest inválido" }
            }
            require(expected.keys == names - "backup.manifest") { "Arquivos não correspondem ao manifest" }
            for ((name, hash) in expected) require(digest(File(stage, name)) == hash) { "Checksum inválido: $name" }
            require(manifest.delete())
            // Keep one complete pre-restore directory; caller already confirmed this operation.
            if (previous.exists()) { require(previous.canonicalFile.parentFile == parent); require(previous.deleteRecursively()) }
            if (root.exists()) Files.move(root.toPath(), previous.toPath(), StandardCopyOption.ATOMIC_MOVE)
            try { Files.move(stage.toPath(), root.toPath(), StandardCopyOption.ATOMIC_MOVE) }
            catch (e: Exception) { if (!root.exists() && previous.exists()) Files.move(previous.toPath(), root.toPath(), StandardCopyOption.ATOMIC_MOVE); throw e }
        } finally { if (stage.exists()) stage.deleteRecursively() }
    }
}
