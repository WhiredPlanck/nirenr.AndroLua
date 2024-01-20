/*
 * $Id: LuaStateFactory.java,v 1.4 2006/12/22 14:06:40 thiago Exp $
 * Copyright (C) 2003-2007 Kepler Project.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

package nirenr.luajava;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This class is responsible for instantiating new LuaStates.
 * When a new LuaState is instantiated it is put into a List
 * and an index is returned. This index is registred in Lua
 * and it is used to find the right LuaState when lua calls
 * a Java Function.
 * 
 * @author Thiago Ponte
 */
public final class LuaStateFactory
{
	/**
	 * Map with all luaState's instances
	 */
	private static final Map<Long, LuaState> states = new HashMap<>();
	
	/**
	 * Non-public constructor. 
	 */
	private LuaStateFactory()
	{}
	
	/**
	 * Method that creates a new instance of LuaState
	 * @return LuaState
	 */
	public synchronized static LuaState newLuaState()
	{
		LuaState L = new LuaState();
		
		states.put(L.getCPtrPeer(), L);
		
		return L;
	}
	
	/**
	 * Returns a existing instance of LuaState
	 * @param peer a long value represents the pointer
	 * @return LuaState
	 */
	public synchronized static LuaState getExistingState(long peer)
	{
		return (LuaState) states.get(peer);
	}
	
	/**
	 * Receives a existing LuaState and checks if it exists in the states map.
	 * If it doesn't exist adds it to the list.
	 * @param L a lua state
	 * @return int
	 */
	public synchronized static long insertLuaState(LuaState L)
	{
		long peer = L.getCPtrPeer();

		states.put(peer, L);
		
		return peer;
	}
	
	/**
	 * removes the luaState from the states map
	 * @param peer a long value represents the lua state pointer
	 */
	public synchronized static void removeLuaState(long peer)
	{
		states.put(peer, null);
	}
}