package android.view

class KeyEvent(val action: Int = ACTION_DOWN, val keyCode: Int = 0, val repeatCount: Int = 0) {
    companion object {
        fun keyCodeFromString(s: String): Int = s.hashCode() and 0xFFFF
        /** The real name for the constants below (the rebind shots show them), else the number. */
        fun keyCodeToString(c: Int): String = "KEYCODE_" + (
            KeyEvent::class.java.declaredFields.firstOrNull { f ->
                f.name.startsWith("KEYCODE_") && runCatching { f.isAccessible = true; f.getInt(null) == c }.getOrDefault(false)
            }?.name?.removePrefix("KEYCODE_") ?: c.toString()
        )
        const val ACTION_DOWN = 0
        const val ACTION_UP = 1
        const val KEYCODE_DPAD_UP = 19
        const val KEYCODE_DPAD_DOWN = 20
        const val KEYCODE_DPAD_LEFT = 21
        const val KEYCODE_DPAD_RIGHT = 22
        const val KEYCODE_DPAD_CENTER = 23
        const val KEYCODE_ENTER = 66
        const val KEYCODE_ESCAPE = 111
        const val KEYCODE_BUTTON_A = 96
        const val KEYCODE_BUTTON_B = 97
        const val KEYCODE_BUTTON_X = 99
        const val KEYCODE_BUTTON_Y = 100
        const val KEYCODE_BUTTON_L1 = 102
        const val KEYCODE_BUTTON_R1 = 103
        const val KEYCODE_BUTTON_START = 108
        const val KEYCODE_BUTTON_SELECT = 109
        const val KEYCODE_BACK = 4
        const val KEYCODE_BUTTON_L2 = 104
        const val KEYCODE_BUTTON_R2 = 105
        const val KEYCODE_HOME = 3
        const val KEYCODE_VOLUME_UP = 24
        const val KEYCODE_VOLUME_DOWN = 25
        const val KEYCODE_POWER = 26
        const val KEYCODE_VOLUME_MUTE = 164
        const val KEYCODE_APP_SWITCH = 187
        const val KEYCODE_DPAD_UP_LEFT = 268
        const val KEYCODE_DPAD_DOWN_LEFT = 269
        const val KEYCODE_DPAD_UP_RIGHT = 270
        const val KEYCODE_DPAD_DOWN_RIGHT = 271
    }
}

/** Joystick motion (Settings' analog L2 / R2 capture): never sent in the preview. */
class MotionEvent {
    fun isFromSource(source: Int): Boolean = false
    fun getAxisValue(axis: Int): Float = 0f
    companion object {
        const val AXIS_LTRIGGER = 17
        const val AXIS_RTRIGGER = 18
        const val AXIS_THROTTLE = 19
        const val AXIS_GAS = 22
        const val AXIS_BRAKE = 23
    }
}

class InputDevice {
    companion object {
        const val SOURCE_JOYSTICK = 0x01000010
    }
}
