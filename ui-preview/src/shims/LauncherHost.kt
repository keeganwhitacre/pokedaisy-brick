package com.pokedaisy.app

import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import java.io.File

/** Stand-in for the launcher's second-screen Presentation: what it would show lands in [Captured.second]. */
class LauncherSecondScreen(private val activity: androidx.activity.ComponentActivity, private val content: @Composable () -> Unit) {
    fun show(): Boolean {
        CapturedSecond.content = { com.pokedaisy.app.companion.ui.theme.AppTheme { content() } }
        return true
    }

    fun dismiss() {}
}

/** The second screen's content, for a shot of it. */
object CapturedSecond {
    var content: (@Composable () -> Unit)? = null
}

/** No video player in the preview: the media screen keeps its screenshot. */
@Composable
fun LauncherVideo(file: File, modifier: Modifier) {}
