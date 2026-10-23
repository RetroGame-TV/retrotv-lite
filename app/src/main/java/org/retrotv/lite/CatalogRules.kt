package org.retrotv.lite

import java.security.MessageDigest

object CatalogRules {
    fun stableId(tree: String, document: String): String = MessageDigest.getInstance("SHA-256")
        .digest("$tree\u0000$document".toByteArray()).joinToString("") { "%02x".format(it) }
    fun clean(value: String) = value.replace('\t', ' ').replace('\n', ' ').replace('\r', ' ').take(240)
    fun system(folder: String, name: String): String? {
        val ext = name.substringAfterLast('.', "").lowercase()
        return when (ext) {
            "nes" -> "NES"
            "sfc", "smc" -> "SNES"
            "md", "gen", "smd" -> "Mega Drive"
            "sms", "gg" -> "Master System"
            "gb", "gbc" -> "Game Boy"
            "chd", "cue", "m3u", "pbp" -> if (folder.uppercase().contains("PS1") || folder.uppercase().contains("PLAYSTATION")) "PlayStation" else null
            "bin" -> if (folder.uppercase().contains("GENESIS") || folder.uppercase().contains("MEGA")) "Mega Drive" else null
            "zip" -> if (folder.uppercase().contains("ARCADE")) "Arcade" else null
            else -> null
        }
    }
    fun core(system: String): String = when (system) {
        "NES" -> "quicknes"; "SNES" -> "snes9x2005"; "Mega Drive", "Master System" -> "picodrive"
        "Game Boy" -> "gambatte"; "Arcade" -> "fbneo"; "PlayStation" -> "pcsx_rearmed"; else -> ""
    }
}
