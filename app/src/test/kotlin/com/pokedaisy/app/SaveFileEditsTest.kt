package com.pokedaisy.app

import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import kotlin.random.Random

class SaveFileEditsTest {

    @get:Rule
    val tmp = TemporaryFolder()

    private val before = Random(1).nextBytes(131088)
    private val after = before.copyOf().also { it[100] = (it[100] + 1).toByte() }

    @Test
    fun writesOverTheSaveAfterBackingItUp() {
        val save = tmp.newFile("Pokemon FireRed.sav").apply { writeBytes(before) }
        val result = SaveFileEdits.write(save, before, after) { SaveBackups.snapshot(it) }
        assertTrue(result is SaveFileEdits.Result.Written)
        assertArrayEquals(after, save.readBytes())
        // The backup holds the save as it was, and no temp file is left behind.
        assertArrayEquals(before, (result as SaveFileEdits.Result.Written).backup.readBytes())
        assertEquals(listOf("Pokemon FireRed.sav"), save.parentFile!!.listFiles()!!.filter { it.isFile }.map { it.name })
    }

    @Test
    fun anExistingBackupOfTheSameSaveCounts() {
        val save = tmp.newFile("Pokemon Emerald.sav").apply { writeBytes(before) }
        SaveBackups.snapshot(save)
        // snapshot() makes no second copy of an unchanged save: the newest one is used.
        val result = SaveFileEdits.write(save, before, after) { SaveBackups.snapshot(it) }
        assertTrue(result is SaveFileEdits.Result.Written)
        assertEquals(1, SaveBackups.list(save).size)
    }

    @Test
    fun aSaveThatChangedSinceItWasReadIsLeftAlone() {
        val save = tmp.newFile("Pokemon Ruby.sav").apply { writeBytes(before) }
        val other = before.copyOf().also { it[5] = (it[5] + 1).toByte() }
        val result = SaveFileEdits.write(save, other, after) { SaveBackups.snapshot(it) }
        assertTrue(result is SaveFileEdits.Result.Failed)
        assertArrayEquals(before, save.readBytes())
    }

    @Test
    fun noBackupNoWrite() {
        val save = tmp.newFile("Pokemon LeafGreen.sav").apply { writeBytes(before) }
        val result = SaveFileEdits.write(save, before, after) { null }
        assertTrue(result is SaveFileEdits.Result.Failed)
        assertArrayEquals(before, save.readBytes())
    }

    @Test
    fun aTradeWritesBothOrNeither() {
        val other = Random(2).nextBytes(131088)
        val otherAfter = other.copyOf().also { it[200] = (it[200] + 1).toByte() }
        val a = tmp.newFile("Pokemon FireRed.sav").apply { writeBytes(before) }
        val b = tmp.newFile("Pokemon Emerald.sav").apply { writeBytes(other) }
        val ok = SaveFileEdits.writeBoth(
            a, SaveFileEdits.Edit(a, before, after), b, SaveFileEdits.Edit(b, other, otherAfter),
        ) { SaveBackups.snapshot(it) }
        assertTrue(ok is SaveFileEdits.Result.Written)
        assertArrayEquals(after, a.readBytes())
        assertArrayEquals(otherAfter, b.readBytes())

        // A changed behind our back: B, written first, is put back as it was.
        a.writeBytes(before)
        b.writeBytes(other)
        val stale = before.copyOf().also { it[7] = (it[7] + 1).toByte() }
        val failed = SaveFileEdits.writeBoth(
            a, SaveFileEdits.Edit(a, stale, after), b, SaveFileEdits.Edit(b, other, otherAfter),
        ) { SaveBackups.snapshot(it) }
        assertTrue(failed is SaveFileEdits.Result.Failed)
        assertArrayEquals(before, a.readBytes())
        assertArrayEquals(other, b.readBytes())
    }
}
