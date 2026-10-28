package android.util

object Log {
    fun d(t: String, m: String) = 0
    fun i(t: String, m: String) = 0
    fun w(t: String, m: String) = 0
    fun w(t: String, m: String, e: Throwable?) = 0
    fun e(t: String, m: String) = 0
    fun e(t: String, m: String, e: Throwable?) = 0
}

/** android.util.LruCache's surface the library's cover cache uses: an access-ordered map evicting by [sizeOf]. */
open class LruCache<K : Any, V : Any>(private val maxSize: Int) {
    private val map = LinkedHashMap<K, V>(16, 0.75f, true)
    private var size = 0
    protected open fun sizeOf(key: K, value: V): Int = 1
    @Synchronized fun get(key: K): V? = map[key]
    @Synchronized fun put(key: K, value: V): V? {
        val old = map.put(key, value)
        if (old != null) size -= sizeOf(key, old)
        size += sizeOf(key, value)
        val it = map.entries.iterator()
        while (size > maxSize && it.hasNext()) {
            val e = it.next()
            if (e.key == key) continue
            size -= sizeOf(e.key, e.value)
            it.remove()
        }
        return old
    }
}
