package com.pokedaisy.app

import android.graphics.SurfaceTexture
import android.media.MediaPlayer
import android.util.Log
import android.view.Surface
import android.view.TextureView
import androidx.activity.ComponentActivity
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.remember
import androidx.compose.ui.Modifier
import androidx.compose.ui.platform.ComposeView
import androidx.compose.ui.viewinterop.AndroidView
import com.pokedaisy.app.companion.ui.theme.AppTheme
import java.io.File

/**
 * The launcher's second-screen window ([LibraryActivity]): a [DualScreenPresentation] showing
 * [content] - the media screen, or the game list after a swap. Never takes key focus (the
 * Presentation hands every key to the activity), so the D-pad and A / B / X / Y act from either
 * screen. Shown while the library is started, gone before a game opens its own.
 */
class LauncherSecondScreen(private val activity: ComponentActivity, private val content: @Composable () -> Unit) {
    private var presentation: DualScreenPresentation? = null

    /** Shows it on the second screen; false = there isn't one (or it couldn't be shown). */
    fun show(): Boolean {
        val display = Screens.second(activity) ?: run { dismiss(); return false }
        presentation?.let { if (it.display.displayId == display.displayId && it.isShowing) return true }
        dismiss()
        presentation = runCatching {
            DualScreenPresentation(activity, display) { ctx ->
                ComposeView(ctx).apply { setContent { AppTheme { content() } } }
            }.also { it.show() }
        }.onFailure { Log.w("pokedaisy", "launcher presentation failed", it) }.getOrNull()
        return presentation != null
    }

    fun dismiss() {
        presentation?.let { runCatching { it.dismiss() }; it.releaseContent() }
        presentation = null
    }
}

/**
 * A game's video in a frame of the media screen: muted, looping, from the start. A TextureView, not a
 * SurfaceView - it sits inside Compose layout, fades with the swap and stays under overlays.
 */
@Composable
fun LauncherVideo(file: File, modifier: Modifier) {
    val player = remember(file) { MediaPlayer() }
    DisposableEffect(player) {
        onDispose { runCatching { player.release() } }
    }
    AndroidView(
        modifier = modifier,
        factory = { ctx ->
            TextureView(ctx).apply {
                surfaceTextureListener = object : TextureView.SurfaceTextureListener {
                    private var surface: Surface? = null
                    override fun onSurfaceTextureAvailable(st: SurfaceTexture, w: Int, h: Int) {
                        val s = Surface(st).also { surface = it }
                        runCatching {
                            player.setSurface(s)
                            player.setDataSource(file.absolutePath)
                            player.isLooping = true
                            player.setVolume(0f, 0f)
                            player.setOnPreparedListener { it.start() }
                            player.prepareAsync()
                        }.onFailure { Log.w("pokedaisy", "launcher video failed: ${file.name}", it) }
                    }
                    override fun onSurfaceTextureSizeChanged(st: SurfaceTexture, w: Int, h: Int) {}
                    override fun onSurfaceTextureDestroyed(st: SurfaceTexture): Boolean {
                        runCatching { player.setSurface(null) }
                        surface?.release()
                        surface = null
                        return true
                    }
                    override fun onSurfaceTextureUpdated(st: SurfaceTexture) {}
                }
            }
        },
    )
}
