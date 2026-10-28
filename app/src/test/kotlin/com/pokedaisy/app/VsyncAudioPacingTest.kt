package com.pokedaisy.app

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import kotlin.random.Random

/** Vsync pacing's audio guard (GitHub #80: music hiccups at 1x in 1.2.0). */
class VsyncAudioPacingTest {
    private val buffer = 4096

    @Test fun lowRunsAtOnceHighWaitsElseTheTick() {
        assertEquals(VsyncPace.CATCH_UP, vsyncPace(0, buffer))
        assertEquals(VsyncPace.CATCH_UP, vsyncPace(buffer * 3 / 8 - 1, buffer))
        assertEquals(VsyncPace.ON_TICK, vsyncPace(buffer / 2, buffer))
        assertEquals(VsyncPace.HOLD, vsyncPace(buffer * 7 / 10 + 1, buffer))
        assertEquals(VsyncPace.ON_TICK, vsyncPace(0, 0))   // no audio track: plain ticks
    }

    @Test fun troubleNeedsSeveralUnderrunsCloseTogether() {
        val w = AudioTroubleWatch(max = 4, windowNanos = 10_000_000_000L)
        val s = 1_000_000_000L
        // Spread out: never trips.
        assertFalse(w.record(0))
        assertFalse(w.record(11 * s))
        assertFalse(w.record(22 * s))
        assertFalse(w.record(33 * s))
        // Four within ten seconds: trips.
        assertFalse(w.record(40 * s))
        assertFalse(w.record(42 * s))
        assertFalse(w.record(44 * s))
        assertTrue(w.record(46 * s))
    }

    /**
     * A 60 Hz screen whose refresh ticks sometimes go missing (the loop busy, or two refreshes close together
     * against a bank of one): the device plays a frame's worth of audio every refresh either way. With the guard,
     * a low buffer runs the next frame at once and the device never runs dry; ticks alone (1.2.0) drain it.
     */
    @Test fun lostRefreshesNoLongerRunTheBufferDry() {
        assertEquals(0, underruns(guard = true))
        assertTrue(underruns(guard = false) > 0)
    }

    private fun underruns(guard: Boolean): Int {
        val perFrame = 800        // audio frames a game frame makes (48 kHz / 60)
        val rng = Random(80)
        var queued = buffer / 2
        var dry = 0
        var catchUpRun = 0
        repeat(20_000) {
            // This refresh's frame, unless its tick went missing.
            if (rng.nextInt(20) != 0) queued = minOf(buffer, queued + perFrame)
            // The guard: run more frames at once while the buffer is low (at most two in a row).
            if (guard) {
                while (vsyncPace(queued, buffer) == VsyncPace.CATCH_UP && catchUpRun < 2) {
                    queued = minOf(buffer, queued + perFrame)
                    catchUpRun++
                }
                catchUpRun = 0
            }
            // The device plays a refresh's worth.
            queued -= perFrame
            if (queued < 0) {
                dry++
                queued = 0
            }
        }
        return dry
    }
}
