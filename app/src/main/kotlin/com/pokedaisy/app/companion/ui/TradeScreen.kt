package com.pokedaisy.app.companion.ui

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import com.pokedaisy.app.companion.data.save.BoxSlot
import com.pokedaisy.app.companion.data.save.Gen3BoxMon
import com.pokedaisy.app.companion.data.save.Gen3SaveData
import com.pokedaisy.app.companion.data.save.Gen3SaveEditor
import com.pokedaisy.app.companion.data.save.Gen3Trade
import com.pokedaisy.app.companion.data.save.TradeEvolution
import com.pokedaisy.app.companion.data.save.TradeRefused
import com.pokedaisy.app.companion.data.save.TradeSide
import com.pokedaisy.app.companion.i18n.tk
import com.pokedaisy.app.companion.i18n.tr

/**
 * TRADE (from a game's PC BOXES): two saves' boxes side by side. A slot picked on each side makes the trade - a
 * Pokémon for a Pokémon (TRADE), or one sent over to an empty slot (SEND) - checked by the games' own rules
 * ([Gen3Trade.blocker], shown before anything happens). A Pokémon that evolves by trading asks first, like the game's
 * evolution scene (B stops it). Trades pile up in both saves until SAVE writes the two together.
 */
class TradeState(a: TradeSide, b: TradeSide) {
    var a = a
        private set
    var b = b
        private set
    var revision by mutableIntStateOf(0)
        private set
    var boxA by mutableIntStateOf(a.editor.data.currentBox.coerceIn(0, Gen3SaveData.BOXES - 1))
    var boxB by mutableIntStateOf(b.editor.data.currentBox.coerceIn(0, Gen3SaveData.BOXES - 1))
    var selA by mutableStateOf<BoxSlot?>(null)
    var selB by mutableStateOf<BoxSlot?>(null)
    /** Why the last trade didn't happen. */
    var refusal by mutableStateOf<String?>(null)
    /** The trade waiting on EVOLVE / DON'T EVOLVE. */
    var evolving by mutableStateOf<List<Pair<Gen3BoxMon, TradeEvolution>>?>(null)
    var confirmSave by mutableStateOf(false)
    var confirmDiscard by mutableStateOf(false)

    val dataA: Gen3SaveData get() { revision; return a.editor.data }
    val dataB: Gen3SaveData get() { revision; return b.editor.data }
    val changed: Boolean get() { revision; return a.editor.changed || b.editor.changed }

    enum class Kind { SEND_TO_B, SEND_TO_A, SWAP }

    /** What the two picked slots make: a send either way, a swap, or nothing yet. */
    val kind: Kind? get() {
        val ma = selA?.let { dataA.mon(it) } ?: return null
        val mb = selB?.let { dataB.mon(it) } ?: return null
        return when {
            !ma.isEmpty && mb.isEmpty -> Kind.SEND_TO_B
            ma.isEmpty && !mb.isEmpty -> Kind.SEND_TO_A
            !ma.isEmpty && !mb.isEmpty -> Kind.SWAP
            else -> null
        }
    }

    /** Why the picked trade can't happen, checked both ways for a swap. */
    val blocker: String? get() {
        val k = kind ?: return null
        val ma = dataA.mon(selA!!)
        val mb = dataB.mon(selB!!)
        return when (k) {
            Kind.SEND_TO_B -> Gen3Trade.blocker(ma, a, b)
            Kind.SEND_TO_A -> Gen3Trade.blocker(mb, b, a)
            Kind.SWAP -> Gen3Trade.blocker(ma, a, b) ?: Gen3Trade.blocker(mb, b, a)
        }
    }

    /** The trade evolutions the picked trade would set off. */
    fun evolutions(): List<Pair<Gen3BoxMon, TradeEvolution>> {
        val k = kind ?: return emptyList()
        val ma = dataA.mon(selA!!)
        val mb = dataB.mon(selB!!)
        return buildList {
            if (k != Kind.SEND_TO_A) Gen3Trade.evolution(ma, b)?.let { add(ma to it) }
            if (k != Kind.SEND_TO_B) Gen3Trade.evolution(mb, a)?.let { add(mb to it) }
        }
    }

    /** TRADE / SEND pressed: ask about evolutions first, else trade. */
    fun start() {
        refusal = null
        val evos = evolutions()
        if (evos.isNotEmpty()) evolving = evos else perform(evolve = false)
    }

    fun perform(evolve: Boolean) {
        evolving = null
        val k = kind ?: return
        val atA = selA!!
        val atB = selB!!
        try {
            when (k) {
                Kind.SEND_TO_B -> Gen3Trade.send(a, atA, b, atB, evolve)
                Kind.SEND_TO_A -> Gen3Trade.send(b, atB, a, atA, evolve)
                Kind.SWAP -> Gen3Trade.swap(a, atA, b, atB, evolve, evolve)
            }
            selA = null
            selB = null
        } catch (e: TradeRefused) {
            refusal = e.message
        }
        revision++
    }

    /** Both saves back to as read. */
    fun undoAll() {
        a = TradeSide(a.label, Gen3SaveEditor(a.editor.file), a.gameCode)
        b = TradeSide(b.label, Gen3SaveEditor(b.editor.file), b.gameCode)
        refusal = null
        revision++
    }
}

@Composable
fun TradeScreen(
    state: TradeState,
    m: GbaTextMetrics,
    onBack: () -> Unit,
    onSave: () -> Unit,
    saving: Boolean = false,
    iconsA: MonIconLoader? = null,
    iconsB: MonIconLoader? = null,
) {
    val small = rememberGbaTextMetrics(textScale = 1f)
    val cell = rememberGbaTextMetrics(textScale = 0.8f)
    val leave = { if (state.changed) state.confirmDiscard = true else onBack() }
    val linkBlock = Gen3Trade.linkBlocker(state.a, state.b)
    Box(Modifier.fillMaxSize()) {
        AppBackdrop()
        Column(Modifier.fillMaxSize().padding(16.dp)) {
            OptionTitleWindow(tk("TRADE"), m, trailing = "${state.a.label} · ${state.b.label}", onBack = leave)
            Spacer(Modifier.height(m.u * 4))
            BoxWithConstraints(Modifier.weight(1f).fillMaxWidth()) {
                val wide = maxWidth > maxHeight * 1.1f
                val sideA: @Composable (Modifier) -> Unit = { mod ->
                    TradeSidePane(state.dataA, state.a.label, state.boxA, { state.boxA = it }, state.selA, { state.selA = it; state.refusal = null },
                        state.revision, m, cell, iconsA, mod)
                }
                val sideB: @Composable (Modifier) -> Unit = { mod ->
                    TradeSidePane(state.dataB, state.b.label, state.boxB, { state.boxB = it }, state.selB, { state.selB = it; state.refusal = null },
                        state.revision, m, cell, iconsB, mod)
                }
                if (wide) Row(Modifier.fillMaxSize(), horizontalArrangement = Arrangement.spacedBy(m.u * 4)) {
                    sideA(Modifier.weight(1f).fillMaxHeight())
                    sideB(Modifier.weight(1f).fillMaxHeight())
                } else Column(Modifier.fillMaxSize(), verticalArrangement = Arrangement.spacedBy(m.u * 4)) {
                    sideA(Modifier.weight(1f).fillMaxWidth())
                    sideB(Modifier.weight(1f).fillMaxWidth())
                }
            }
            Spacer(Modifier.height(m.u * 4))
            Row(horizontalArrangement = Arrangement.spacedBy(m.u * 4), verticalAlignment = Alignment.CenterVertically) {
                val kind = state.kind
                val blocker = linkBlock ?: state.blocker
                val hint = state.refusal ?: blocker ?: when (kind) {
                    TradeState.Kind.SEND_TO_B -> tr("{0} goes to {1}", monLabel(state.dataA.mon(state.selA!!)), state.b.label)
                    TradeState.Kind.SEND_TO_A -> tr("{0} goes to {1}", monLabel(state.dataB.mon(state.selB!!)), state.a.label)
                    TradeState.Kind.SWAP -> tr("{0} for {1}", monLabel(state.dataA.mon(state.selA!!)), monLabel(state.dataB.mon(state.selB!!)))
                    null -> if (state.changed) tr("Not saved yet") else tr("Pick a slot on each side: a Pokémon for a Pokémon, or one for an empty slot")
                }
                // Two lines: a rule's reason names both games.
                GbaText(hint, OptionColors.onBackdrop, OptionColors.onBackdropShadow, small, Modifier.weight(1f), maxLines = 2)
                OptionButton(
                    if (kind == TradeState.Kind.SWAP) tk("TRADE") else tk("SEND"), m, onClick = { state.start() }, emphasis = true,
                    enabled = kind != null && blocker == null && !saving,
                )
                OptionButton(tk("UNDO ALL"), m, onClick = { state.undoAll() }, enabled = state.changed && !saving)
                OptionButton(tk("SAVE"), m, onClick = { state.confirmSave = true }, emphasis = true, enabled = state.changed && !saving)
            }
        }

        state.evolving?.let { evos ->
            EvolvePrompt(evos, m, onEvolve = { state.perform(evolve = true) }, onStop = { state.perform(evolve = false) }, onCancel = { state.evolving = null })
        }
        if (state.confirmSave) {
            OptionConfirm(
                title = tk("SAVE BOTH?"),
                message = tr("Both saves are written the way the game saves, each backed up to pokedaisy-backups first. If the second can't be written, the first is put back, so no Pokémon is lost or copied."),
                confirmLabel = tk("SAVE"),
                m = m,
                onDismiss = { state.confirmSave = false },
                onConfirm = { state.confirmSave = false; onSave() },
            )
        }
        if (state.confirmDiscard) {
            OptionConfirm(
                title = tk("DISCARD CHANGES?"),
                message = tr("The trades you made aren't saved. Leave without saving them?"),
                confirmLabel = tk("DISCARD"),
                m = m,
                onDismiss = { state.confirmDiscard = false },
                onConfirm = { state.confirmDiscard = false; onBack() },
            )
        }
    }
}

/** One side: its box (the picked slot framed; PC BOXES has the details). */
@Composable
private fun TradeSidePane(
    data: Gen3SaveData, label: String, box: Int, onBox: (Int) -> Unit, sel: BoxSlot?, onSel: (BoxSlot?) -> Unit,
    @Suppress("UNUSED_PARAMETER") revision: Int, m: GbaTextMetrics, cell: GbaTextMetrics, icons: MonIconLoader?, modifier: Modifier,
) {
    BoxGrid(
        data, box, onBox, m, cell, icons, modifier, title = label,
        selected = { it == sel }, target = { false },
        onTap = { at -> onSel(if (at == sel) null else at) },
    )
}

/** The game's "What? X is evolving!" with its choice: EVOLVE, or B (DON'T EVOLVE: traded as it is). */
@Composable
private fun EvolvePrompt(
    evos: List<Pair<Gen3BoxMon, TradeEvolution>>, m: GbaTextMetrics, onEvolve: () -> Unit, onStop: () -> Unit, onCancel: () -> Unit,
) {
    OptionOverlay(onCancel, Modifier.widthIn(max = 520.dp)) {
        Column {
            OptionTitleWindow(tk("EVOLVING!"), m)
            Spacer(Modifier.height(m.u * 4))
            OptionListWindow(m, Modifier.fillMaxWidth()) {
                Column {
                    val lines = evos.joinToString("\n") { (mon, evo) -> tr("{0} evolves into {1} when it arrives.", monLabel(mon), speciesLabel(evo.into)) }
                    GbaText(lines, OptionColors.label, OptionColors.labelShadow, m, maxLines = Int.MAX_VALUE,
                        modifier = Modifier.padding(horizontal = m.u * 8, vertical = m.u * 4))
                    Row(
                        horizontalArrangement = Arrangement.spacedBy(m.u * 4),
                        modifier = Modifier.fillMaxWidth().padding(horizontal = m.u * 4, vertical = m.u * 4),
                    ) {
                        OptionButton(tk("DON'T EVOLVE"), m, onClick = onStop, modifier = Modifier.weight(1f).height(m.rowHeight * 1.4f))
                        OptionButton(tk("EVOLVE"), m, onClick = onEvolve, emphasis = true, modifier = Modifier.weight(1f).height(m.rowHeight * 1.4f))
                    }
                }
            }
        }
    }
}
