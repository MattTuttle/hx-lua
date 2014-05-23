#ifndef STATIC_LINK
#define IMPLEMENT_API
#endif

#if defined(HX_WINDOWS) || defined(HX_MACOS) || defined(HX_LINUX)
#define NEKO_COMPATIBLE
#endif

#include <hx/CFFI.h>

extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
	#include "math.h"
}

// forward declarations
void haxe_to_lua(value v, lua_State *l);

value lua_to_haxe(lua_State *l)
{
	lua_Number n;
	int lua_v;
	value v;
	while ((lua_v = lua_gettop(l)) != 0)
	{
		switch (lua_type(l, lua_v))
		{
			case LUA_TNUMBER:
				n = lua_tonumber(l, lua_v);
				// check if number is int or float
				v = (fmod(n, 1) == 0) ? alloc_int(n) : alloc_float(n);
				break;
			case LUA_TTABLE:
				break;
			case LUA_TSTRING:
				v = alloc_string(lua_tostring(l, lua_v));
				break;
			case LUA_TBOOLEAN:
				v = alloc_bool(lua_toboolean(l, lua_v));
				break;
		}
		lua_pop(l, 1);
	}
	return v;
}

inline void haxe_array_to_lua(value v, lua_State *l)
{
	int size = val_array_size(v);
	value *arr = val_array_value(v);
	lua_createtable(l, size, 0);
	for (int i = 0; i < size; i++)
	{
		lua_pushnumber(l, i + 1);
		haxe_to_lua(arr[i], l);
		lua_settable(l, -3);
	}
}

void haxe_iter_object(value v, field f, void *state)
{
	lua_State *l = (lua_State *)state;
	const char *name = val_string(val_field_name(f));
	lua_pushstring(l, name);
	haxe_to_lua(v, l);
	lua_settable(l, -3);
}

void haxe_iter_global(value v, field f, void *state)
{
	lua_State *l = (lua_State *)state;
	haxe_to_lua(v, l);
	lua_setglobal(l, val_string(val_field_name(f)));
}

void haxe_to_lua(value v, lua_State *l)
{
	switch (val_type(v))
	{
		case valtNull:
			lua_pushnil(l);
			break;
		case valtBool:
			lua_pushboolean(l, val_bool(v));
			break;
		case valtFloat:
			lua_pushnumber(l, val_float(v));
			break;
		case valtInt:
			lua_pushnumber(l, val_int(v));
			break;
		case valtString:
			lua_pushstring(l, val_string(v));
			break;
		case valtFunction:
			break;
		case valtArray:
			haxe_array_to_lua(v, l);
			break;
		case valtAbstractBase: // abstract
			break;
		case valtObject:
		case valtEnum:
		case valtClass:
			lua_createtable(l, 0, 0);
			val_iter_fields(v, haxe_iter_object, l);
			break;
	}
}

static value lua_execute(value inScript, value inArgs)
{
	static const luaL_Reg lualibs[] = {
		{ "base", luaopen_base },
		{ "math", luaopen_math },
		{ "table", luaopen_table },
		{ NULL, NULL }
	};

	lua_State *l = luaL_newstate();

	// load libraries
	const luaL_Reg *lib = lualibs;
	while (lib->func != NULL)
	{
		lib->func(l);
		lua_settop(l, 0);
		lib++;
	}

	if (!val_is_null(inArgs) && val_is_object(inArgs))
	{
		val_iter_fields(inArgs, haxe_iter_global, l);
	}

	// load the script
	int status = luaL_loadstring(l, val_string(inScript));
	int result = 0;
	if (status == LUA_OK)
	{
		result = lua_pcall(l, 0, LUA_MULTRET, 0);
	}
	else
	{
		return alloc_bool(false);
	}

	value v = lua_to_haxe(l);

	// close the lua state
	lua_close(l);

	return v;
}
DEFINE_PRIM(lua_execute, 2);


extern "C" void lua_main()
{
	val_int(0); // Fix Neko init
}
DEFINE_ENTRY_POINT(lua_main);

extern "C" int lua_register_prims() { return 0; }
