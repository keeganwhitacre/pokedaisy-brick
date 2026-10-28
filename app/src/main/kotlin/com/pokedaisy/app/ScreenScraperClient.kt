package com.pokedaisy.app

import android.util.Log
import com.pokedaisy.app.companion.i18n.tr
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.net.HttpURLConnection
import java.net.URL
import java.net.URLEncoder
import java.security.MessageDigest
import java.util.zip.CRC32

/**
 * ScreenScraper (screenscraper.fr) API v2: a game's whole media set - box art (2D / 3D), screenshot,
 * title screen, wheel logo, fanart, cartridge, video - plus its synopsis, release date, developer,
 * publisher, genre and players. The first source for every library cover and the launcher's media
 * screen ([GameMedia]), ahead of RetroAchievements and SteamGridDB.
 *
 * Two pairs of credentials go with every call: the app's own developer account
 * ([BuildConfig.SCREENSCRAPER_DEV_ID] / `_PASSWORD`, from the untracked local.properties or the
 * release workflow's secrets - never in this public repo) and the player's screenscraper.fr account
 * ([Prefs.screenScraperUser] / `Password`: the API has no token, so the password is kept, in the
 * app's private prefs, and never logged). Plain `HttpURLConnection` + `org.json`, like
 * [SteamGridDbClient]; every call is blocking network I/O, so never on the UI thread.
 */
object ScreenScraperClient {
    private const val TAG = "pokedaisy/screenscraper"
    private const val API = "https://api.screenscraper.fr/api2"
    private const val SOFTNAME = "PokeDaisy"

    /** The app's developer login: this build's, or (ui-preview's `sscheck`) one from the environment. */
    @Volatile
    var developer: Pair<String, String> = BuildConfig.SCREENSCRAPER_DEV_ID to BuildConfig.SCREENSCRAPER_DEV_PASSWORD

    /**
     * Whether ScreenScraper is on at all ([FeatureFlags.screenScraper], read when the process and the library start).
     * Off: no account reads as signed in, so nothing is fetched, and its rows, setup step and popup are hidden.
     */
    @Volatile
    var enabled: Boolean = BuildConfig.FEATURE_SCREENSCRAPER

    /** Where players make an account (Settings / setup / the update's popup). */
    const val SIGN_UP_PAGE = "https://www.screenscraper.fr/membreinscription.php"

    /** ScreenScraper's system ids. */
    const val SYSTEM_GBA = 12
    const val SYSTEM_GB = 9
    const val SYSTEM_GBC = 10

    data class Account(val user: String, val password: String)

    /** A signed-in account as the server sees it: [requestsToday] of [maxRequestsPerDay] used. */
    data class UserInfo(
        val name: String,
        val level: Int,
        val maxThreads: Int,
        val requestsToday: Int,
        val maxRequestsPerDay: Int,
    )

    /** One media file of a game: [type] is ScreenScraper's (`box-2D`, `ss`, `wheel`...), [region] its
     * region code (`us`, `eu`, `wor`, `jp`, `ss`...; empty for region-free ones). */
    data class Media(val type: String, val region: String, val url: String, val format: String)

    /** A game: names by region, synopses by language, its facts and every media file. */
    data class Game(
        val id: String,
        val names: Map<String, String>,
        val synopsis: Map<String, String>,
        val released: Map<String, String>,
        val developer: String?,
        val publisher: String?,
        val genres: List<String>,
        val players: String?,
        /** ScreenScraper's rating out of 20, as it sends it. */
        val rating: String?,
        val medias: List<Media>,
    )

    /** Why a call came back empty. [fatal]: no point asking for the next game either (bad login,
     * quota spent, API closed) - a library pass stops on it. */
    class ApiException(
        val code: Int,
        message: String,
        /** What ScreenScraper itself answered (its first line, often French), for the sign-in to show. */
        val serverText: String = "",
    ) : Exception(message) {
        val fatal get() = code in FATAL_CODES
        val notFound get() = code == 404
    }

    // 400: fields missing (no developer login in this build) - every call would fail the same way.
    private val FATAL_CODES = setOf(400, 401, 403, 423, 426, 429, 430, 431)

    /** What the player's account looks like now - the sign-in check. Throws [ApiException] / I/O. */
    fun userInfo(account: Account): UserInfo {
        val o = getJson("ssuserInfos.php", account, emptyMap()).optJSONObject("response")?.optJSONObject("ssuser")
            ?: throw ApiException(500, tr("No account in the answer"))
        return UserInfo(
            name = o.optString("id").ifBlank { account.user },
            level = o.optString("niveau").toIntOrNull() ?: 0,
            maxThreads = o.optString("maxthreads").toIntOrNull() ?: 1,
            requestsToday = o.optString("requeststoday").toIntOrNull() ?: 0,
            maxRequestsPerDay = o.optString("maxrequestsperday").toIntOrNull() ?: 0,
        )
    }

    /** The game a ROM file is, by its hashes, size and file name (empty ones are left out). Null = not in the database
     * (or a hack it only knows by name - see [search]). Throws [ApiException] for anything else. */
    fun gameByRom(account: Account, rom: RomHashes, system: Int, fileName: String): Game? {
        val params = mapOf(
            "systemeid" to system.toString(),
            "romtype" to "rom",
            "crc" to rom.crc32,
            "md5" to rom.md5,
            "sha1" to rom.sha1,
            "romnom" to fileName,
            "romtaille" to rom.size.takeIf { it > 0 }?.toString().orEmpty(),
        ).filterValues { it.isNotEmpty() }
        return try {
            getJson("jeuInfos.php", account, params).optJSONObject("response")?.optJSONObject("jeu")?.let(::parseGame)
        } catch (e: ApiException) {
            if (e.notFound) null else throw e
        }
    }

    /** ScreenScraper's search by name on [system], best match first (games with media only). */
    fun search(account: Account, name: String, system: Int): List<Game> {
        val json = try {
            getJson("jeuRecherche.php", account, mapOf("systemeid" to system.toString(), "recherche" to name))
        } catch (e: ApiException) {
            if (e.notFound) return emptyList() else throw e
        }
        val list = json.optJSONObject("response")?.optJSONArray("jeux") ?: return emptyList()
        // An empty search answers [{}].
        return list.objects().mapNotNull(::parseGame)
    }

    /** Downloads [url] to [out] through a temp file, so a failed download never replaces what's
     * there. The URL carries the credentials: never logged. */
    fun download(url: String, out: File): Boolean = runCatching {
        out.parentFile?.mkdirs()
        val tmp = File(out.parentFile, "${out.name}.tmp")
        val conn = open(url)
        try {
            if (conn.responseCode !in 200..299) error("HTTP ${conn.responseCode}")
            // A missing media answers 200 with a short text ("NOMEDIA", an error) instead of an image.
            val type = conn.contentType.orEmpty()
            if (type.startsWith("text/")) error("no media ($type)")
            conn.inputStream.use { input -> tmp.outputStream().use { input.copyTo(it) } }
        } finally {
            conn.disconnect()
        }
        check(tmp.length() > 64 && tmp.renameTo(out))
    }.onFailure { t -> Log.w(TAG, "media download failed: ${safeDescription(t)}") }.isSuccess

    /** A ROM's identity as ScreenScraper matches it: the whole ROM (an archive's ROM) hashed once. */
    data class RomHashes(val crc32: String, val md5: String, val sha1: String, val size: Long) {
        companion object {
            fun of(rom: File): RomHashes? = runCatching {
                val crc = CRC32()
                val md5 = MessageDigest.getInstance("MD5")
                val sha1 = MessageDigest.getInstance("SHA-1")
                var size = 0L
                RomArchive.open(rom).buffered(1 shl 16).use { s ->
                    val buf = ByteArray(1 shl 16)
                    while (true) {
                        val n = s.read(buf)
                        if (n < 0) break
                        crc.update(buf, 0, n)
                        md5.update(buf, 0, n)
                        sha1.update(buf, 0, n)
                        size += n
                    }
                }
                RomHashes("%08X".format(crc.value), md5.digest().hex(), sha1.digest().hex(), size)
            }.getOrNull()

            private fun ByteArray.hex() = joinToString("") { "%02X".format(it) }
        }
    }

    // ---- parsing --------------------------------------------------------------

    internal fun parseGame(o: JSONObject): Game? {
        val id = o.optString("id").takeIf { it.isNotBlank() && it != "0" } ?: return null
        val names = o.optJSONArray("noms").regionTexts("region")
        val medias = o.optJSONArray("medias").objects().mapNotNull { m ->
            val url = m.optString("url").takeIf { it.startsWith("http") } ?: return@mapNotNull null
            Media(m.optString("type"), m.optString("region"), url, m.optString("format"))
        }
        return Game(
            id = id,
            names = names,
            synopsis = o.optJSONArray("synopsis").regionTexts("langue"),
            released = o.optJSONArray("dates").regionTexts("region"),
            developer = o.optJSONObject("developpeur")?.optString("text")?.takeIf { it.isNotBlank() },
            publisher = o.optJSONObject("editeur")?.optString("text")?.takeIf { it.isNotBlank() },
            genres = o.optJSONArray("genres").objects().mapNotNull { g ->
                // A genre's name per language: English first.
                val byLang = g.optJSONArray("noms").regionTexts("langue")
                byLang["en"] ?: byLang.values.firstOrNull()
            },
            players = o.optJSONObject("joueurs")?.optString("text")?.takeIf { it.isNotBlank() },
            rating = o.optJSONObject("note")?.optString("text")?.takeIf { it.isNotBlank() },
            medias = medias,
        )
    }

    /** `[{"region": "us", "text": "..."}]` -> {us: ...}; the first of a key wins. */
    private fun JSONArray?.regionTexts(key: String): Map<String, String> {
        val out = LinkedHashMap<String, String>()
        objects().forEach { o ->
            val text = o.optString("text").trim()
            if (text.isNotEmpty()) out.putIfAbsent(o.optString(key), text)
        }
        return out
    }

    private fun JSONArray?.objects(): List<JSONObject> =
        if (this == null) emptyList() else (0 until length()).mapNotNull { optJSONObject(it) }

    // ---- HTTP -------------------------------------------------------------------

    private fun getJson(endpoint: String, account: Account, params: Map<String, String>): JSONObject {
        val (devId, devPassword) = developer
        val all = linkedMapOf(
            "devid" to devId,
            "devpassword" to devPassword,
            "softname" to SOFTNAME,
            "output" to "json",
            "ssid" to account.user,
            "sspassword" to account.password,
        ) + params
        val query = all.entries.joinToString("&") { (k, v) -> "$k=${enc(v)}" }
        val conn = open("$API/$endpoint?$query")
        try {
            val code = conn.responseCode
            val body = (if (code in 200..299) conn.inputStream else conn.errorStream)
                ?.use { it.readBytes().toString(Charsets.UTF_8) }.orEmpty()
            return parseAnswer(code, body)
        } finally {
            conn.disconnect()
        }
    }

    /**
     * An answer as JSON, or the [ApiException] it means. ScreenScraper sends most refusals as HTTP 200
     * with a line of French (a game it doesn't know, the day's quota, the API closed...), so the text
     * decides, not only the code - a game not found must skip that game, not stop the whole pass.
     */
    internal fun parseAnswer(code: Int, body: String): JSONObject {
        val trimmed = body.trimStart()
        if (code !in 200..299 || !trimmed.startsWith("{")) {
            val real = refusalCode(code, body)
            val said = serverLine(body)
            // The answer's text only (never the request URL, which carries the passwords).
            warn("refused: HTTP $code (read as $real): $said")
            throw ApiException(real, errorText(real, body), said)
        }
        // Its JSON sometimes has a trailing comma before a closing brace (Skyscraper patches the same).
        val json = JSONObject(trimmed.replace(TRAILING_COMMA, "$1$2"))
        val header = json.optJSONObject("header")
        if (header != null && header.optString("success", "true") != "true") {
            val error = header.optString("error")
            warn("refused in JSON: $error")
            throw ApiException(refusalCode(500, error), error.ifBlank { "ScreenScraper error" }, error)
        }
        return json
    }

    /** Log.w, off the JVM tests' path: there it's layoutlib's android.util.Log, whose native half isn't loaded. */
    private fun warn(message: String) {
        runCatching { Log.w(TAG, message) }
    }

    private val TRAILING_COMMA = Regex("""([\]}]),(\s*[}\]])""")

    /** The HTTP code a refusal really means, from its text when ScreenScraper answered 200 (or anything vague). */
    internal fun refusalCode(code: Int, body: String): Int {
        val b = body.lowercase()
        return when {
            "non trouv" in b || "not found" in b -> 404
            "quota" in b -> 430
            "blacklist" in b -> 426
            "non membre" in b || "non-registered" in b || "threads" in b -> 429
            "fermé" in b || "closed" in b -> 423
            "identifiant" in b || "login" in b -> 401
            "manque" in b || "missing" in b -> 400
            code in 200..299 -> 500
            else -> code
        }
    }

    /** [t]'s kind and message with any URL's query dropped: a request URL carries both passwords. */
    fun safeDescription(t: Throwable): String =
        t.javaClass.simpleName + (t.message?.replace(Regex("""\?\S*"""), "")?.take(100)?.let { ": $it" } ?: "")

    /** Whether this build carries the app's developer login (local.properties / the release's secrets). */
    val hasDeveloperLogin get() = developer.first.isNotBlank() && developer.second.isNotBlank()

    /** The first line of an answer that isn't JSON, tags dropped (an HTML error page), kept short. */
    private fun serverLine(body: String): String =
        body.replace(Regex("<[^>]*>"), " ").lineSequence().map { it.trim() }.firstOrNull { it.isNotEmpty() }?.take(160).orEmpty()

    /** The server's own words (French) are short; a code we know gets a plain English reason. */
    private fun errorText(code: Int, body: String): String = when (code) {
        401, 403 -> if (!hasDeveloperLogin || body.contains("veloppeur", ignoreCase = true) || body.contains("developer", ignoreCase = true)) {
            tr("This build has no ScreenScraper developer access")
        } else {
            tr("Wrong username or password")
        }
        404 -> tr("Game not found")
        423 -> tr("ScreenScraper's API is closed for now")
        426 -> tr("ScreenScraper doesn't accept this app version")
        429 -> tr("Too many requests at once")
        430 -> tr("Today's ScreenScraper quota is used up")
        431 -> tr("Too many unknown ROMs today")
        // Fields missing: with no developer login in this build, that's what's missing.
        400 -> if (!hasDeveloperLogin) tr("This build has no ScreenScraper developer access") else serverLine(body).ifEmpty { "HTTP $code" }
        else -> serverLine(body).ifEmpty { "HTTP $code" }
    }

    private fun enc(s: String) = URLEncoder.encode(s, "UTF-8")

    private fun open(url: String): HttpURLConnection = (URL(url).openConnection() as HttpURLConnection).apply {
        connectTimeout = 15_000
        readTimeout = 30_000
        requestMethod = "GET"
        setRequestProperty("User-Agent", "$SOFTNAME/${BuildConfig.VERSION_NAME}")
    }
}

/** The signed-in ScreenScraper account, or null - always null while the feature is off ([ScreenScraperClient.enabled]). */
val Prefs.screenScraperAccount: ScreenScraperClient.Account?
    get() {
        if (!ScreenScraperClient.enabled) return null
        val user = screenScraperUser ?: return null
        val password = screenScraperPassword ?: return null
        return ScreenScraperClient.Account(user, password)
    }

fun Prefs.setScreenScraperAccount(account: ScreenScraperClient.Account?) {
    screenScraperUser = account?.user
    screenScraperPassword = account?.password
}
