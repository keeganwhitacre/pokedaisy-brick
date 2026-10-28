package com.pokedaisy.app

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

/** ScreenScraper's answers as [ScreenScraperClient] reads them, and which media [GameMedia] keeps. */
class ScreenScraperTest {

    /** The shape of a jeuInfos.php `jeu` (trimmed; made-up values). */
    private val game = """
        {
          "id": "1234",
          "noms": [{"region": "ss", "text": "Sample Quest"}, {"region": "us", "text": "Sample Quest US"}],
          "synopsis": [{"langue": "fr", "text": "Une histoire."}, {"langue": "en", "text": "A story."}],
          "dates": [{"region": "jp", "text": "2004-01-29"}, {"region": "us", "text": "2004-09-07"}],
          "developpeur": {"id": "1", "text": "Sample Studio"},
          "editeur": {"id": "2", "text": "Sample Games"},
          "genres": [{"id": "7", "noms": [{"langue": "de", "text": "Rollenspiel"}, {"langue": "en", "text": "Role playing game"}]}],
          "joueurs": {"text": "1-2"},
          "note": {"text": "16"},
          "medias": [
            {"type": "box-2D", "region": "jp", "url": "https://example.invalid/jp", "format": "png"},
            {"type": "box-2D", "region": "us", "url": "https://example.invalid/us", "format": "png"},
            {"type": "ss", "region": "", "url": "https://example.invalid/ss", "format": "png"},
            {"type": "wheel", "region": "wor", "url": "https://example.invalid/wheel", "format": "png"},
            {"type": "video", "url": "not a url", "format": "mp4"}
          ]
        }
    """.trimIndent()

    @Test
    fun parsesAGame() {
        val g = ScreenScraperClient.parseGame(JSONObject(game))!!
        assertEquals("1234", g.id)
        assertEquals("Sample Quest US", g.names["us"])
        assertEquals("A story.", g.synopsis["en"])
        assertEquals("2004-09-07", g.released["us"])
        assertEquals("Sample Studio", g.developer)
        assertEquals("Sample Games", g.publisher)
        assertEquals(listOf("Role playing game"), g.genres)
        assertEquals("1-2", g.players)
        assertEquals("16", g.rating)
        // A media entry without a real URL is dropped.
        assertEquals(4, g.medias.size)
    }

    @Test
    fun anEmptySearchHitIsNoGame() {
        assertNull(ScreenScraperClient.parseGame(JSONObject("{}")))
        assertNull(ScreenScraperClient.parseGame(JSONObject("""{"id": "0"}""")))
    }

    @Test
    fun picksTheCartsRegionFirst() {
        val medias = ScreenScraperClient.parseGame(JSONObject(game))!!.medias
        assertEquals("us", GameMedia.pick(medias, GameMedia.Kind.BOX, GameMedia.regionOrder('E'))!!.region)
        assertEquals("jp", GameMedia.pick(medias, GameMedia.Kind.BOX, GameMedia.regionOrder('J'))!!.region)
        // No European box: the next region in line.
        assertEquals("us", GameMedia.pick(medias, GameMedia.Kind.BOX, GameMedia.regionOrder('P'))!!.region)
        // Region-free media, and a kind's second type (wheel-hd missing -> wheel).
        assertEquals("https://example.invalid/ss", GameMedia.pick(medias, GameMedia.Kind.SCREENSHOT, GameMedia.regionOrder('E'))!!.url)
        assertEquals("https://example.invalid/wheel", GameMedia.pick(medias, GameMedia.Kind.WHEEL, GameMedia.regionOrder('E'))!!.url)
        assertNull(GameMedia.pick(medias, GameMedia.Kind.FANART, GameMedia.regionOrder('E')))
    }

    @Test
    fun textAnswersAreReadForWhatTheyMean() {
        fun code(body: String) = runCatching { ScreenScraperClient.parseAnswer(200, body) }
            .exceptionOrNull() as ScreenScraperClient.ApiException
        // A game it doesn't know: skip it, not the whole pass.
        assertEquals(404, code("Erreur : Rom/Iso/Dossier non trouvée !  ").code)
        assertEquals(false, code("Erreur : Rom/Iso/Dossier non trouvée !").fatal)
        assertEquals(430, code("Votre quota de scrape est dépassé pour aujourd'hui !").code)
        assertEquals(423, code("API totalement fermé").code)
        assertEquals(429, code("API fermé pour les non membres").code)
        assertEquals(401, code("Erreur de login : Vérifier vos identifiants développeur !").code)
        assertEquals(true, code("Erreur de login : Vérifier vos identifiants développeur !").fatal)
        assertEquals(400, code("Il manque des champs obligatoires dans l'url").code)
    }

    @Test
    fun aTrailingCommaIsForgiven() {
        val json = ScreenScraperClient.parseAnswer(200, "{\"header\": {\"success\": \"true\"}, \"response\": {\"jeu\": {\"id\": \"5\", \"noms\": [],\n\t\t}}}")
        assertEquals("5", json.getJSONObject("response").getJSONObject("jeu").getString("id"))
    }

    @Test
    fun aSearchHitMustBeTheGameNamed() {
        fun game(vararg names: String) = ScreenScraperClient.Game(
            "1", names.mapIndexed { i, n -> "r$i" to n }.toMap(), emptyMap(), emptyMap(), null, null, emptyList(), null, null, emptyList(),
        )
        val fireRed = game("Pokemon - Fire Red Version", "Pokemon Version Rouge Feu")
        assertEquals(true, GameMedia.nameMatches("Pokémon FireRed Version", fireRed))
        // A hack's name must not take the base game's entry.
        assertEquals(false, GameMedia.nameMatches("Pokémon Radical Red", fireRed))
        assertEquals(true, GameMedia.nameMatches("Pokémon Radical Red", game("Pokemon Radical Red (Hack)")))
        assertEquals(true, GameMedia.nameMatches("Pokémon R.O.W.E.", game("Pokemon ROWE")))
        assertEquals(true, GameMedia.nameMatches("Pokémon Heart & Soul", game("Pokemon Heart and Soul")))
        // Nothing telling to match on.
        assertEquals(false, GameMedia.nameMatches("Pokémon", fireRed))
    }
}
