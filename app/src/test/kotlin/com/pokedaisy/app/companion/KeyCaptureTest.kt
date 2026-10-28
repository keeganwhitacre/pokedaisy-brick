package com.pokedaisy.app.companion

import android.view.KeyEvent.KEYCODE_BACK
import android.view.KeyEvent.KEYCODE_BUTTON_A
import android.view.KeyEvent.KEYCODE_BUTTON_L2
import android.view.KeyEvent.KEYCODE_BUTTON_R1
import android.view.KeyEvent.KEYCODE_BUTTON_SELECT
import android.view.KeyEvent.KEYCODE_BUTTON_X
import android.view.KeyEvent.KEYCODE_DPAD_UP
import android.view.KeyEvent.KEYCODE_VOLUME_UP
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/** GAME BUTTONS / HOTKEYS by pressing the keys: what a capture takes, what it lets through, and how it ends. */
class KeyCaptureTest {

    private fun capture(chord: Boolean, done: MutableList<List<Int>>, cleared: (() -> Unit)? = null) = KeyCapture().apply {
        start(KeyCapture.Request(if (chord) "SAVE STATE" else "GBA A", chord, onDone = { done += it }, onClear = cleared))
    }

    @Test
    fun aChordEndsOnceEveryKeyIsLetGo() {
        val done = mutableListOf<List<Int>>()
        val c = capture(chord = true, done)
        assertTrue(c.onKey(KEYCODE_BUTTON_SELECT, true))
        assertTrue(c.onKey(KEYCODE_BUTTON_R1, true))
        assertEquals(listOf(KEYCODE_BUTTON_SELECT, KEYCODE_BUTTON_R1), c.keys)
        assertTrue(c.onKey(KEYCODE_BUTTON_R1, false))
        assertTrue(c.active)   // SELECT still held
        assertTrue(c.onKey(KEYCODE_BUTTON_SELECT, false))
        assertEquals(listOf(listOf(KEYCODE_BUTTON_SELECT, KEYCODE_BUTTON_R1)), done)
        assertFalse(c.active)
    }

    @Test
    fun aButtonTakesOneKeyAndNeverTheDpad() {
        val done = mutableListOf<List<Int>>()
        val c = capture(chord = false, done)
        // The D-pad is the GBA's own: taken (it doesn't reach the game or the focus) but not bound.
        assertTrue(c.onKey(KEYCODE_DPAD_UP, true))
        assertTrue(c.onKey(KEYCODE_DPAD_UP, false))
        assertTrue(c.active)
        c.onKey(KEYCODE_BUTTON_X, true)
        c.onKey(KEYCODE_BUTTON_A, true)
        c.onKey(KEYCODE_BUTTON_X, false)
        c.onKey(KEYCODE_BUTTON_A, false)
        assertEquals(listOf(listOf(KEYCODE_BUTTON_X)), done)
    }

    @Test
    fun theKeyThatOpenedTheRowEndsNothing() {
        val done = mutableListOf<List<Int>>()
        val c = capture(chord = false, done)
        // A's press opened the row before the capture began: its release (and repeats) are swallowed, not bound.
        assertTrue(c.onKey(KEYCODE_BUTTON_A, true, repeatCount = 3))
        assertTrue(c.onKey(KEYCODE_BUTTON_A, false))
        assertTrue(c.active)
        assertTrue(done.isEmpty())
    }

    @Test
    fun backCancelsAndKeepsItsRelease() {
        val done = mutableListOf<List<Int>>()
        val c = capture(chord = true, done)
        assertTrue(c.onKey(KEYCODE_BACK, true))
        assertFalse(c.active)
        // Its release mustn't reach the activity as a BACK tap of its own.
        assertTrue(c.onKey(KEYCODE_BACK, false))
        assertFalse(c.onKey(KEYCODE_BACK, false))
        assertTrue(done.isEmpty())
    }

    @Test
    fun aKeyHeldWhenCancelledHasItsReleaseKept() {
        val c = capture(chord = true, mutableListOf())
        c.onKey(KEYCODE_BUTTON_SELECT, true)
        c.cancel()
        assertTrue(c.onKey(KEYCODE_BUTTON_SELECT, false))
        // Pressed again later, it's the game's again, release and all.
        assertFalse(c.onKey(KEYCODE_BUTTON_SELECT, true))
        assertFalse(c.onKey(KEYCODE_BUTTON_SELECT, false))
    }

    @Test
    fun systemKeysPassAndIdleCaptureTakesNothing() {
        val c = capture(chord = true, mutableListOf())
        assertFalse(c.onKey(KEYCODE_VOLUME_UP, true))
        c.cancel()
        assertFalse(c.onKey(KEYCODE_BUTTON_A, true))
        assertFalse(c.onKey(KEYCODE_BUTTON_A, false))
        assertFalse(c.onTriggers(1f, 0f))
    }

    @Test
    fun clearEndsWithoutBinding() {
        val done = mutableListOf<List<Int>>()
        var cleared = 0
        val c = capture(chord = true, done) { cleared++ }
        c.onKey(KEYCODE_BUTTON_X, true)
        c.clear()
        assertEquals(1, cleared)
        assertNull(c.request)
        assertTrue(done.isEmpty())
    }

    @Test
    fun anAnalogTriggerIsItsKey() {
        val done = mutableListOf<List<Int>>()
        val c = capture(chord = false, done)
        assertTrue(c.onTriggers(0.2f, 0f))
        assertTrue(c.onTriggers(0.9f, 0f))
        assertTrue(c.onTriggers(0f, 0f))
        assertEquals(listOf(listOf(KEYCODE_BUTTON_L2)), done)
    }

    @Test
    fun aTriggerHeldIntoTheCaptureIsNotItsKey() {
        val done = mutableListOf<List<Int>>()
        val c = KeyCapture()
        c.onTriggers(1f, 0f)   // held while nothing waits
        c.start(KeyCapture.Request("GBA A", chord = false, onDone = { done += it }))
        c.onTriggers(0f, 0f)   // let go: the press wasn't the capture's
        assertTrue(c.active)
        assertTrue(done.isEmpty())
    }

    @Test
    fun startAndEndAreReported() {
        val seen = mutableListOf<Boolean>()
        val c = KeyCapture().apply { onActiveChanged = { seen += it } }
        c.start(KeyCapture.Request("GBA A", chord = false, onDone = {}))
        c.start(KeyCapture.Request("GBA B", chord = false, onDone = {}))   // another row: still one capture
        c.onKey(KEYCODE_BUTTON_X, true)
        c.onKey(KEYCODE_BUTTON_X, false)
        assertEquals(listOf(true, false), seen)
    }
}
