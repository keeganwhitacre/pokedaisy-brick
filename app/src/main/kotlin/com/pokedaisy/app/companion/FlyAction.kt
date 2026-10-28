package com.pokedaisy.app.companion

import com.pokedaisy.app.companion.data.FlyResult
import com.pokedaisy.app.companion.data.FlyTargets

/** FLY from the MAP tab (Fly.kt); null where the game has no such support. Every [done] runs on the main thread. */
interface FlyAction {
    /** Whether this ROM has FLY here at all (the button shows when a party Pokémon also knows the move). */
    fun canFly(): Boolean

    /** Where FLY can go now, or why it can't. */
    fun targets(done: (FlyTargets) -> Unit)

    /** Flies to [mapsec] (one of [targets]' destinations): [done] with OK once the game has taken over. */
    fun fly(mapsec: Int, targets: FlyTargets, done: (FlyResult) -> Unit)
}
