package nirenr.luajava.util

import nirenr.luajava.LuaObject
import nirenr.luajava.LuaState


class LuaTable<K, V>: LuaObject, MutableMap<K, V> {
    constructor(state: LuaState, globalName: String): super(state, globalName)
    constructor(state: LuaState, index: Int): super(state, index)
    constructor(state: LuaState): super(state, -1) {
        L.newTable()
    }

    override val entries: MutableSet<MutableMap.MutableEntry<K, V>>
        get() {
            val set = mutableSetOf<MutableMap.MutableEntry<K, V>>()
            visitWhileCatch {
                @Suppress("UNCHECKED_CAST")
                set.add(
                    LuaEntry(L.toJavaObject(-2) as K, L.toJavaObject(-1) as V)
                )
                L.pop(1)
            }
            return set
        }

    override val keys: MutableSet<K>
        get() {
            val set = mutableSetOf<K>()
            visitWhileCatch {
                @Suppress("UNCHECKED_CAST")
                set.add(L.toJavaObject(-2) as K)
                L.pop(1)
            }
            return set
        }

    override val size: Int
        get() {
            push()
            val n = L.rawLen(-1)
            L.pop(1)
            return n
        }

    override val values: MutableCollection<V>
        get() {
            val collection = mutableListOf<V>()
            visitWhileCatch {
                @Suppress("UNCHECKED_CAST")
                collection.add(L.toJavaObject(-1) as V)
            }
            return collection
        }

    override fun clear() {
        visitWhile {
            L.pop(1)
            L.pushValue(-1)
            L.pushNil()
            L.setTable(-4)
        }
    }

    override fun isEmpty(): Boolean {
        push()
        L.pushNil()
        return (L.next(-2) == 0).also {
            if (it) L.pop(1) else L.pop(3)
        }
    }

    override fun remove(key: K): V? {
        visitCatch(false) {
            L.pushObjectValue(key)
            L.setTable(-2)
        }
        return null
    }

    override fun putAll(from: Map<out K, V>) {
        TODO("Not yet implemented")
    }

    override fun put(key: K, value: V): V? {
        visitCatch(false) {
            L.pushObjectValue(key)
            L.pushObjectValue(value)
            L.setTable(-3)
        }
        return null
    }

    override fun get(key: K): V? {
        var value: V? = null
        visitCatch(false) {
            L.pushObjectValue(key)
            L.getTable(-2)
            @Suppress("UNCHECKED_CAST")
            value = L.toJavaObject(-1) as V
            L.pop(1)
        }
        return value
    }

    override fun containsValue(value: V): Boolean {
        // TODO("Not yet implemented")
        return false
    }

    override fun containsKey(key: K): Boolean {
        var contains = false
        visitCatchOr(false, {
            L.pushObjectValue(key)
            contains = (L.getTable(-2) != LuaState.LUA_TNIL)
            L.pop(1)
        }) { return@visitCatchOr false }
        return contains
    }

    inner class LuaEntry<K, V>(override val key: K, override var value: V) : MutableMap.MutableEntry<K, V> {
        override fun setValue(newValue: V): V {
            val old = value
            value = newValue
            return old
        }
    }

    private fun visit(pushNil: Boolean = true, action: () -> Unit) {
        push()
        if (pushNil) L.pushNil()
        action()
        L.pop(1)
    }

    private fun visitWhile(action: () -> Unit) {
        visit {
            while (L.next(-2) != 0) {
                action()
            }
        }
    }

    private fun visitCatch(pushNil: Boolean = true, action: () -> Unit) {
        visit(pushNil) {
            runCatching { action() }
        }
    }

    private fun <T> visitCatchOr(pushNil: Boolean = true, action: () -> Unit, or: () -> T) {
        visit(pushNil) {
            runCatching { action() }.getOrElse { or() }
        }
    }

    private fun visitWhileCatch(action: () -> Unit) {
        visitWhile {
            runCatching { action() }
        }
    }
}