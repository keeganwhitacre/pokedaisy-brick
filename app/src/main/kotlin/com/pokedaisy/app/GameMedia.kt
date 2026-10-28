package com.pokedaisy.app

import android.content.Context
import com.pokedaisy.app.companion.i18n.L10n
import org.json.JSONObject
import java.io.File

/**
 * A game's ScreenScraper media, fetched once per ROM ([fetch]) and kept on the device under
 * `<external files>/media/<rom file name>/`: one file per [Kind] plus `info.json` (the facts and
 * which game ScreenScraper matched, or that it found none). Nothing here is bundled - the
 * launcher's media screen ([LauncherMedia]) and the library covers read what the player's own
 * account downloaded.
 */
object GameMedia {
    /** What a game's media set holds, in ScreenScraper's types (the first one found wins). */
    enum class Kind(val file: String, val types: List<String>) {
        BOX("box.img", listOf("box-2D")),
        BOX_3D("box3d.img", listOf("box-3D")),
        SCREENSHOT("screenshot.img", listOf("ss")),
        TITLE("title.img", listOf("sstitle")),
        WHEEL("wheel.img", listOf("wheel-hd", "wheel", "wheel-carbon")),
        FANART("fanart.img", listOf("fanart")),
        CART("cart.img", listOf("support-2D")),
        VIDEO("video.mp4", listOf("video-normalized", "video")),
    }

    /** The facts shown beside the media (strings as ScreenScraper has them, in the player's language where it has it). */
    data class Info(
        val name: String?,
        val synopsis: String?,
        val released: String?,
        val developer: String?,
        val publisher: String?,
        val genre: String?,
        val players: String?,
        /** Out of 20. */
        val rating: String?,
        /** False: ScreenScraper doesn't know the game (checked at [checkedAt]). */
        val found: Boolean = true,
        val checkedAt: Long = 0L,
        /** A download failed: the next pass fetches again (only what's missing). */
        val incomplete: Boolean = false,
        /** Whether that fetch asked for the video too (turning DOWNLOAD VIDEOS on fetches again). */
        val videos: Boolean = false,
    )

    /** How a [fetch] went. */
    enum class Result { FETCHED, CACHED, NOT_FOUND, NOT_SIGNED_IN, FAILED }

    /** A game ScreenScraper didn't know is asked again after this long (new entries get added). */
    private const val NOT_FOUND_RETRY_MS = 7L * 24 * 3600 * 1000

    fun root(context: Context): File = File(context.getExternalFilesDir(null) ?: context.filesDir, "media")

    fun dir(root: File, rom: File) = File(root, rom.name)

    fun file(root: File, rom: File, kind: Kind) = File(dir(root, rom), kind.file)

    /** [kind]'s file if it's been downloaded. */
    fun existing(root: File, rom: File, kind: Kind): File? = file(root, rom, kind).takeIf { it.isFile && it.length() > 0 }

    /** The saved facts, or null before a fetch. */
    fun info(root: File, rom: File): Info? = runCatching {
        val f = File(dir(root, rom), "info.json").takeIf { it.isFile } ?: return null
        val o = JSONObject(f.readText())
        fun s(k: String) = o.optString(k).takeIf { it.isNotBlank() }
        Info(
            name = s("name"), synopsis = s("synopsis"), released = s("released"), developer = s("developer"),
            publisher = s("publisher"), genre = s("genre"), players = s("players"), rating = s("rating"),
            found = o.optBoolean("found", true), checkedAt = o.optLong("checkedAt"),
            incomplete = o.optBoolean("incomplete"), videos = o.optBoolean("videos"),
        )
    }.getOrNull()

    /** Drops [rom]'s media (its ROM was deleted). */
    fun forget(root: File, rom: File) {
        dir(root, rom).deleteRecursively()
    }

    /**
     * Matches [rom] on ScreenScraper and downloads its media set (blocking: hashes the ROM, network).
     * Already fetched = [Result.CACHED] unless [replace]. [label] is the library's name for it, tried
     * as a search when the hashes aren't in the database (hacks are often listed by name only).
     * Throws [ScreenScraperClient.ApiException] when the account / quota stops every further call.
     * One fetch at a time in the whole app: a free account may only run one request at once.
     */
    @Synchronized
    fun fetch(root: File, rom: File, account: ScreenScraperClient.Account, label: String, videos: Boolean, replace: Boolean): Result {
        val known = info(root, rom)
        if (!replace && known != null) {
            if (known.found && !known.incomplete && (known.videos || !videos)) return Result.CACHED
            if (!known.found && System.currentTimeMillis() - known.checkedAt < NOT_FOUND_RETRY_MS) return Result.NOT_FOUND
        }
        val hashes = ScreenScraperClient.RomHashes.of(rom) ?: return Result.FAILED
        val match = find(account, hashes, systemsOf(rom), rom.name, label, baseGameTitle(rom))
        val dir = dir(root, rom).apply { mkdirs() }
        if (match == null) {
            writeInfo(dir, JSONObject().put("found", false).put("checkedAt", System.currentTimeMillis()))
            return Result.NOT_FOUND
        }
        save(dir, match.game, RomIdentity.gameCode(rom)?.getOrNull(3), videos, replace)
        return Result.FETCHED
    }

    /** How a game was found: its exact ROM, its name, or (nothing else) the retail game it's built on. */
    enum class MatchedBy { HASH, NAME, BASE_GAME }

    class Match(val game: ScreenScraperClient.Game, val by: MatchedBy)

    /**
     * The game ScreenScraper has for a ROM: by its hashes, then a search by [label], then by [baseTitle] -
     * each on [systems] in order (a Game Boy cart may be listed under Game Boy or Game Boy Color).
     */
    fun find(
        account: ScreenScraperClient.Account,
        hashes: ScreenScraperClient.RomHashes,
        systems: List<Int>,
        fileName: String,
        label: String,
        baseTitle: String?,
    ): Match? {
        val anyRom = listOf(hashes.crc32, hashes.md5, hashes.sha1, fileName).any { it.isNotEmpty() }
        if (anyRom) for (system in systems) {
            ScreenScraperClient.gameByRom(account, hashes, system, fileName)?.let { return Match(it, MatchedBy.HASH) }
        }
        for (system in systems) searchFirst(account, label, system)?.let { return Match(it, MatchedBy.NAME) }
        if (baseTitle == null) return null
        for (system in systems) searchFirst(account, baseTitle, system)?.let { return Match(it, MatchedBy.BASE_GAME) }
        return null
    }

    /** Downloads [game]'s media into [dir] (regions by the cart's language [letter]) and writes info.json;
     * how many downloads failed. */
    fun save(dir: File, game: ScreenScraperClient.Game, letter: Char?, videos: Boolean, replace: Boolean): Int {
        dir.mkdirs()
        val regions = regionOrder(letter)
        var failed = 0
        for (kind in Kind.entries) {
            if (kind == Kind.VIDEO && !videos) continue
            val media = pick(game.medias, kind, regions) ?: continue
            val out = File(dir, kind.file)
            if ((replace || !out.isFile) && !ScreenScraperClient.download(media.url, out)) failed++
        }
        val lang = L10n.language.locale
        val info = JSONObject()
            .put("id", game.id)
            .put("found", true)
            .put("checkedAt", System.currentTimeMillis())
            .put("incomplete", failed > 0)
            .put("videos", videos)
            .put("name", firstOf(game.names, regions + "ss"))
            .put("synopsis", game.synopsis[lang] ?: game.synopsis["en"] ?: game.synopsis.values.firstOrNull())
            .put("released", firstOf(game.released, regions))
            .put("developer", game.developer)
            .put("publisher", game.publisher)
            .put("genre", game.genres.take(2).joinToString(" / ").takeIf { it.isNotEmpty() })
            .put("players", game.players)
            .put("rating", game.rating)
        writeInfo(dir, info)
        return failed
    }

    private fun writeInfo(dir: File, o: JSONObject) {
        val tmp = File(dir, "info.json.tmp")
        tmp.writeText(o.toString())
        tmp.renameTo(File(dir, "info.json"))
    }

    /** ScreenScraper's best search hit for [name] that really is that game (see [nameMatches]). */
    private fun searchFirst(account: ScreenScraperClient.Account, name: String, system: Int): ScreenScraperClient.Game? {
        val query = fold(name).trim().takeIf { it.isNotBlank() } ?: return null
        return ScreenScraperClient.search(account, query, system).firstOrNull { nameMatches(query, it) }
    }

    /** Words every Pokémon title has: they can't tell one game from another. */
    private val COMMON_WORDS = setOf("pokemon", "version", "the", "and", "of")

    /**
     * Whether [game] is the game [query] names: one of its names holds each of the query's telling
     * words (spaces ignored, so "Fire Red" is "FireRed"). A search for a hack otherwise came back with
     * the base game first - Radical Red would have shown FireRed's box.
     */
    internal fun nameMatches(query: String, game: ScreenScraperClient.Game): Boolean {
        // Runs of single letters are one word ("R.O.W.E." -> rowe).
        val tokens = fold(query).lowercase().split(Regex("[^a-z0-9]+")).filter { it.isNotEmpty() }
        val merged = mutableListOf<String>()
        var run = ""
        for (t in tokens) {
            if (t.length == 1) {
                run += t
                continue
            }
            if (run.isNotEmpty()) merged += run
            run = ""
            merged += t
        }
        if (run.isNotEmpty()) merged += run
        val words = merged.filter { it.length > 1 && it !in COMMON_WORDS }
        if (words.isEmpty()) return false
        return game.names.values.any { n ->
            val compact = fold(n).lowercase().filter { it.isLetterOrDigit() }
            words.all { it in compact }
        }
    }

    /** Accents dropped (Pokémon -> Pokemon): ScreenScraper's names and searches are plain ASCII. */
    private fun fold(s: String): String =
        java.text.Normalizer.normalize(s, java.text.Normalizer.Form.NFD).replace(Regex("\\p{Mn}+"), "")

    /**
     * ScreenScraper systems to ask, in order: GBA, or for a Game Boy cart both GB systems - a Color-only
     * cart (0x143 = 0xC0) under Color first, any other under Game Boy first. Yellow is flagged
     * Color-compatible (0x80) but is a Game Boy game there.
     */
    private fun systemsOf(rom: File): List<Int> {
        if (!RomIdentity.isGameBoy(rom)) return listOf(ScreenScraperClient.SYSTEM_GBA)
        val cgb = RomArchive.readAt(rom, 0x143L, 1)?.firstOrNull()?.toInt()?.and(0xFF) ?: 0
        return if (cgb == 0xC0) listOf(ScreenScraperClient.SYSTEM_GBC, ScreenScraperClient.SYSTEM_GB)
        else listOf(ScreenScraperClient.SYSTEM_GB, ScreenScraperClient.SYSTEM_GBC)
    }

    /** The retail game a hack is built on, by its header (searched when nothing else matched). */
    private fun baseGameTitle(rom: File): String? = baseGameTitle(RomIdentity.gameCode(rom))

    /** The retail game a header [code] belongs to (BPR = FireRed ...), as ScreenScraper names it. */
    fun baseGameTitle(code: String?): String? = when (code?.take(3)) {
        "BPR" -> "Pokemon FireRed Version"
        "BPG" -> "Pokemon LeafGreen Version"
        "BPE" -> "Pokemon Emerald Version"
        "AXV" -> "Pokemon Ruby Version"
        "AXP" -> "Pokemon Sapphire Version"
        else -> null
    }

    /** Media regions to prefer for a cart whose game code ends in [letter] (E, P, J, F, D, S, I). */
    internal fun regionOrder(letter: Char?): List<String> = when (letter) {
        'J' -> listOf("jp", "wor", "us", "eu", "ss")
        'P' -> listOf("eu", "wor", "us", "ss", "jp")
        'F' -> listOf("fr", "eu", "wor", "us", "ss", "jp")
        'D' -> listOf("de", "eu", "wor", "us", "ss", "jp")
        'S' -> listOf("sp", "eu", "wor", "us", "ss", "jp")
        'I' -> listOf("it", "eu", "wor", "us", "ss", "jp")
        else -> listOf("us", "wor", "eu", "ss", "jp")
    }

    /** The best media for [kind]: its types in order, each in [regions] order, then any region. */
    internal fun pick(medias: List<ScreenScraperClient.Media>, kind: Kind, regions: List<String>): ScreenScraperClient.Media? {
        for (type in kind.types) {
            val ofType = medias.filter { it.type == type }
            if (ofType.isEmpty()) continue
            return regions.firstNotNullOfOrNull { r -> ofType.firstOrNull { it.region == r } }
                ?: ofType.firstOrNull { it.region.isEmpty() }
                ?: ofType.first()
        }
        return null
    }

    private fun firstOf(byRegion: Map<String, String>, regions: List<String>): String? =
        regions.firstNotNullOfOrNull { byRegion[it] } ?: byRegion.values.firstOrNull()
}
