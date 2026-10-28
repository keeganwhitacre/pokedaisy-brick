package com.pokedaisy.app

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.pokedaisy.app.companion.i18n.tk
import com.pokedaisy.app.companion.i18n.tr
import com.pokedaisy.app.companion.ui.GbaText
import com.pokedaisy.app.companion.ui.GbaTextMetrics
import com.pokedaisy.app.companion.ui.OptionButton
import com.pokedaisy.app.companion.ui.OptionColors
import com.pokedaisy.app.companion.ui.OptionListWindow
import com.pokedaisy.app.companion.ui.OptionOverlay
import com.pokedaisy.app.companion.ui.OptionTextField
import com.pokedaisy.app.companion.ui.OptionTitleWindow

/**
 * The ScreenScraper sign-in, shared by Settings > ScreenScraper, first-time setup's GAME MEDIA step
 * and the update's one-time popup ([ScreenScraperIntro]). The check is a real call
 * ([ScreenScraperClient.userInfo]); only an account the server took is saved.
 */
class ScreenScraperLogin(signedIn: String? = null) {
    var user by mutableStateOf(signedIn.orEmpty())
    var password by mutableStateOf("")
    var signingIn by mutableStateOf(false)
    var error by mutableStateOf<String?>(null)
    /** What ScreenScraper itself said, or the failure's kind, under [error]. */
    var errorDetail by mutableStateOf<String?>(null)
    /** The signed-in account's name; null = signed out. */
    var signedInAs by mutableStateOf(signedIn)
    /** The account's quota, once a call has told us. */
    var info by mutableStateOf<ScreenScraperClient.UserInfo?>(null)

    val canSignIn get() = !signingIn && user.isNotBlank() && password.isNotEmpty()

    /** Checks the account on a background thread; [ui] runs a block on the UI thread (the answer), then [done] on success. */
    fun signIn(prefs: Prefs, ui: (() -> Unit) -> Unit, done: () -> Unit = {}) {
        if (!canSignIn) return
        val account = ScreenScraperClient.Account(user.trim(), password)
        signingIn = true
        error = null
        errorDetail = null
        Thread({
            val result = runCatching { ScreenScraperClient.userInfo(account) }
            ui {
                signingIn = false
                result.onSuccess {
                    prefs.setScreenScraperAccount(account)
                    password = ""
                    signedInAs = account.user
                    info = it
                    done()
                }.onFailure { t ->
                    android.util.Log.w("pokedaisy/screenscraper", "sign-in failed: ${ScreenScraperClient.safeDescription(t)}")
                    when (t) {
                        is ScreenScraperClient.ApiException -> {
                            error = t.message
                            errorDetail = t.serverText.takeIf { it.isNotBlank() && it != t.message }
                                ?.let { tr("ScreenScraper says: {0}", it) }
                        }
                        is java.io.IOException -> {
                            error = tr("Can't reach ScreenScraper - check the connection")
                            errorDetail = ScreenScraperClient.safeDescription(t)
                        }
                        else -> {
                            error = tr("ScreenScraper sent an answer the app can't read")
                            errorDetail = ScreenScraperClient.safeDescription(t)
                        }
                    }
                }
            }
        }, "pokedaisy-ss-login").apply { isDaemon = true; start() }
    }

    /** Reads the signed-in account's quota for the panel (background; silent on failure). */
    fun refresh(prefs: Prefs, ui: (() -> Unit) -> Unit) {
        val account = prefs.screenScraperAccount ?: return
        Thread({
            val i = runCatching { ScreenScraperClient.userInfo(account) }.getOrNull()
            if (i != null) ui { info = i }
        }, "pokedaisy-ss-info").apply { isDaemon = true; start() }
    }

    companion object {
        fun of(prefs: Prefs) = ScreenScraperLogin(prefs.screenScraperAccount?.user)
    }

    fun signOut(prefs: Prefs) {
        prefs.setScreenScraperAccount(null)
        signedInAs = null
        info = null
        password = ""
    }
}

/** Signed in: whose account and today's quota, SIGN OUT. Signed out: username, password, SIGN IN and SIGN UP. */
@Composable
fun ScreenScraperAccountPanel(
    login: ScreenScraperLogin,
    m: GbaTextMetrics,
    small: GbaTextMetrics,
    onSignIn: () -> Unit,
    onSignOut: () -> Unit,
    onSignUp: () -> Unit,
) {
    Column(Modifier.fillMaxWidth()) {
        val name = login.signedInAs
        if (name != null) {
            GbaText(tr("SIGNED IN AS"), OptionColors.label, OptionColors.labelShadow, m)
            GbaText(name.uppercase(), OptionColors.value, OptionColors.valueShadow, m)
            login.info?.let { i ->
                GbaText(
                    tr("{0} OF {1} REQUESTS USED TODAY", i.requestsToday, i.maxRequestsPerDay),
                    OptionColors.muted, OptionColors.mutedShadow, small, maxLines = 2,
                )
            }
            Row(Modifier.padding(vertical = m.u * 4), horizontalArrangement = Arrangement.spacedBy(m.u * 4)) {
                OptionButton(tk("SIGN OUT"), m, onClick = onSignOut)
            }
        } else {
            GbaText(tr("USERNAME"), OptionColors.label, OptionColors.labelShadow, m, Modifier.padding(vertical = m.u * 2))
            OptionTextField(login.user, { login.user = it; login.error = null; login.errorDetail = null }, small, placeholder = tr("screenscraper.fr username"))
            Spacer(Modifier.height(m.u * 4))
            GbaText(tr("PASSWORD"), OptionColors.label, OptionColors.labelShadow, m, Modifier.padding(vertical = m.u * 2))
            OptionTextField(login.password, { login.password = it; login.error = null; login.errorDetail = null }, small, placeholder = tr("password"), password = true)
            login.error?.let {
                GbaText(it.uppercase(), OptionColors.value, OptionColors.valueShadow, small, Modifier.padding(top = m.u * 4), maxLines = 3)
            }
            login.errorDetail?.let {
                GbaText(it, OptionColors.muted, OptionColors.mutedShadow, small, Modifier.padding(top = m.u * 2), maxLines = 3)
            }
            // Every call needs the app's own developer login too; a build without one is refused.
            if (!ScreenScraperClient.hasDeveloperLogin && login.error == null) {
                GbaText(
                    tr("This build has no ScreenScraper developer login, so ScreenScraper may refuse to sign in. Release builds have one."),
                    OptionColors.muted, OptionColors.mutedShadow, small, Modifier.padding(top = m.u * 4), maxLines = Int.MAX_VALUE,
                )
            }
            Row(Modifier.padding(vertical = m.u * 4), horizontalArrangement = Arrangement.spacedBy(m.u * 4)) {
                OptionButton(
                    if (login.signingIn) tk("SIGNING IN…") else tk("SIGN IN"), m, emphasis = true,
                    enabled = login.canSignIn, onClick = onSignIn,
                )
                OptionButton(tr("NO ACCOUNT? SIGN UP"), m, onClick = onSignUp)
            }
        }
    }
}

/**
 * The update's news, once (Prefs.screenScraperAsked): ScreenScraper's media for every game, with a
 * sign-in right here. After it the library's media pass runs ([sync]) and the window shows how far
 * it got; CLOSE leaves it running.
 */
@Composable
fun ScreenScraperIntro(
    login: ScreenScraperLogin,
    sync: CoverSync,
    m: GbaTextMetrics,
    small: GbaTextMetrics,
    onSignIn: () -> Unit,
    onSignUp: () -> Unit,
    onClose: () -> Unit,
) {
    OptionOverlay(onClose, Modifier.widthIn(max = 760.dp)) {
        Column {
            OptionTitleWindow(tr("NEW: GAME MEDIA FROM SCREENSCRAPER"), m)
            Spacer(Modifier.height(m.u * 4))
            OptionListWindow(m, Modifier.fillMaxWidth()) {
                Column(Modifier.padding(m.u * 4).verticalScroll(rememberScrollState())) {
                    GbaText(
                        tr("Box art, screenshots, title screens, logos, videos and each game's story for your whole library, from ScreenScraper.fr. It comes first for covers, and with two screens the library shows the game you're on, in full, on the other one."),
                        OptionColors.label, OptionColors.labelShadow, small, maxLines = Int.MAX_VALUE,
                    )
                    Spacer(Modifier.height(m.u * 4))
                    GbaText(
                        tr("It needs a free screenscraper.fr account. Set it up now?"),
                        OptionColors.muted, OptionColors.mutedShadow, small, maxLines = Int.MAX_VALUE,
                    )
                    Spacer(Modifier.height(m.u * 4))
                    if (login.signedInAs == null) {
                        ScreenScraperAccountPanel(login, m, small, onSignIn = onSignIn, onSignOut = {}, onSignUp = onSignUp)
                    } else {
                        GbaText(tr("SIGNED IN AS {0}", login.signedInAs!!.uppercase()), OptionColors.value, OptionColors.valueShadow, m)
                        Spacer(Modifier.height(m.u * 4))
                        CoverSyncStatus(sync, m)
                    }
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(m.u * 4),
                        modifier = Modifier.fillMaxWidth().padding(top = m.u * 6),
                    ) {
                        Spacer(Modifier.weight(1f))
                        OptionButton(
                            if (login.signedInAs == null) tr("NOT NOW") else tr("CLOSE"), m, onClick = onClose,
                            modifier = Modifier.widthIn(min = 160.dp),
                        )
                    }
                }
            }
        }
    }
}

/** Setup's GAME MEDIA step: what ScreenScraper brings and the sign-in, then the pass's progress. */
@Composable
internal fun ScreenScraperSetupStep(
    login: ScreenScraperLogin,
    sync: CoverSync,
    m: GbaTextMetrics,
    small: GbaTextMetrics,
    onSignIn: () -> Unit,
    onSignOut: () -> Unit,
    onSignUp: () -> Unit,
) {
    GbaText(tr("GAME MEDIA (OPTIONAL)"), OptionColors.label, OptionColors.labelShadow, m, Modifier.padding(top = m.u * 2, bottom = m.u * 2))
    GbaText(
        tr("ScreenScraper.fr has box art, screenshots, title screens, logos, videos and each game's story. Sign in with your free account and PokéDaisy fetches them for every game: covers in the library, and with two screens, everything about the game you're on on the other screen."),
        OptionColors.label, OptionColors.labelShadow, small, maxLines = Int.MAX_VALUE, modifier = Modifier.padding(bottom = small.u * 4),
    )
    GbaText(
        tr("Skip this if you like - sign in any time in SETTINGS > SCREENSCRAPER."),
        OptionColors.muted, OptionColors.mutedShadow, small, maxLines = Int.MAX_VALUE, modifier = Modifier.padding(bottom = small.u * 4),
    )
    ScreenScraperAccountPanel(login, m, small, onSignIn, onSignOut, onSignUp)
    if (sync.running || sync.log.isNotEmpty()) {
        Spacer(Modifier.height(m.u * 4))
        CoverSyncStatus(sync, m)
    }
}
