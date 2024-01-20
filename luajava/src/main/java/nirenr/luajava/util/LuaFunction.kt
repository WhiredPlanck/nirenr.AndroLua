package nirenr.luajava.util

import nirenr.luajava.LuaException
import nirenr.luajava.LuaObject
import nirenr.luajava.LuaState

class LuaFunction<out R: Any>: LuaObject, Function<R> {
    constructor(state: LuaState, globalName: String): super(state, globalName)
    constructor(state: LuaState, index: Int): super(state, index)

    @Throws(LuaException::class)
    override fun call(vararg args: Any?): R {
        @Suppress("UNCHECKED_CAST")
        return super.call(args) as R
    }

    operator fun invoke(vararg args: Any?): R = call(args)
}
