#ifndef STATIC_LINK
#define IMPLEMENT_API
#endif

#if defined(HX_WINDOWS) || defined(HX_MACOS) || defined(HX_LINUX)
#define NEKO_COMPATIBLE
#endif

#include <hx/CFFI.h>
#include <cmath>
#include <vector>
#include <string>

extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

struct Func {
	AutoGCRoot *root;
	std::string name;
};

std::vector<Func> gFuncs;

// forward declarations
int haxe_to_lua(value v, lua_State *l);
value lua_value_to_haxe(lua_State *l, int lua_v);

#define BEGIN_TABLE_LOOP(l, v) lua_pushnil(l); \
	while (lua_next(l, v) != 0) {
#define END_TABLE_LOOP(l) lua_pop(l, 1); }

value lua_table_to_haxe(lua_State *l, int lua_v)
{
	value v;
	int field_count = 0;
	bool array = true;

	// count the number of key/value pairs and figure out if it's an array or object
	BEGIN_TABLE_LOOP(l, lua_v)
		// check for all number keys (array), otherwise it's an object
		if (lua_type(l, -2) != LUA_TNUMBER) array = false;

		field_count += 1;
	END_TABLE_LOOP(l)

	if (array)
	{
		v = alloc_array(field_count);
		value *arr = val_array_value(v);
		BEGIN_TABLE_LOOP(l, lua_v)
			int index = (int)(lua_tonumber(l, -2) - 1); // lua has 1 based indices instead of 0
			arr[index] = lua_value_to_haxe(l, lua_v+2);
		END_TABLE_LOOP(l)
	}
	else
	{
		v = alloc_empty_object();
		BEGIN_TABLE_LOOP(l, lua_v)
			// TODO: don't assume string keys
			const char *key = lua_tostring(l, -2);
			alloc_field(v, val_id(key), lua_value_to_haxe(l, lua_v+2));
		END_TABLE_LOOP(l)
	}

	return v;
}

value lua_value_to_haxe(lua_State *l, int lua_v)
{
	lua_Number n;
	value v;
	switch (lua_type(l, lua_v))
	{
		case LUA_TNIL:
			v = alloc_null();
			break;
		case LUA_TFUNCTION:
			printf("function return is unsupported");
			break;
		case LUA_TNUMBER:
			n = lua_tonumber(l, lua_v);
			// check if number is int or float
			v = (fmod(n, 1) == 0) ? alloc_int(n) : alloc_float(n);
			break;
		case LUA_TTABLE:
			v = lua_table_to_haxe(l, lua_v);
			break;
		case LUA_TSTRING:
			v = alloc_string(lua_tostring(l, lua_v));
			break;
		case LUA_TBOOLEAN:
			v = alloc_bool(lua_toboolean(l, lua_v));
			break;
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

static int haxe_callback(lua_State *l)
{
	const char *func_name = lua_tostring(l, 0);
	// loop through and find the function
	// TODO: make this faster with a map?
	for (std::vector<Func>::iterator it = gFuncs.begin(); it != gFuncs.end(); ++it)
	{
		if (strcmp((*it).name.c_str(), func_name) == 0)
		{
			int num_args = 0;
			value *args = new value[num_args];
			value result = val_callN((*it).root->get(), args, num_args);
			delete [] args;
			return haxe_to_lua(result, l);
		}
	}
	return 0;
}

void haxe_value(lua_State *l, value v, const char *name)
{
	if (val_is_function(v))
	{
		// register a new function callback
		Func f;
		f.root = new AutoGCRoot(v);
		f.name = name;
		// TODO: check that name isn't already registered as function
		gFuncs.push_back(f);
		lua_pushcfunction(l, haxe_callback);
	}
	else
	{
		haxe_to_lua(v, l);
	}
}

void haxe_iter_object(value v, field f, void *state)
{
	lua_State *l = (lua_State *)state;
	const char *name = val_string(val_field_name(f));
	lua_pushstring(l, name);
	haxe_value(l, v, name);
	lua_settable(l, -3);
}

void haxe_iter_global(value v, field f, void *state)
{
	lua_State *l = (lua_State *)state;
	const char *name = val_string(val_field_name(f));
	haxe_value(l, v, name);
	lua_setglobal(l, name);
}

// convert haxe values to lua
int haxe_to_lua(value v, lua_State *l)
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
			return 0;
		case valtArray:
			haxe_array_to_lua(v, l);
			break;
		case valtAbstractBase: // should abstracts be handled??
			return 0;
		case valtObject: // falls through
		case valtEnum: // falls through
		case valtClass:
			lua_createtable(l, 0, 0);
			val_iter_fields(v, haxe_iter_object, l);
			break;
	}
	return 1;
}

static value lua_execute(value inScript, value inContext)
{
	value v;
	lua_State *l = luaL_newstate();
	static const luaL_Reg lualibs[] = {
		{ "base", luaopen_base },
		{ "math", luaopen_math },
		{ "table", luaopen_table },
		{ NULL, NULL }
	};

	// load libraries
	const luaL_Reg *lib = lualibs;
	for (;lib->func != NULL; lib++)
	{
		luaL_requiref(l, lib->name, lib->func, 1);
		lua_settop(l, 0);
	}

	// load context, if any
	if (!val_is_null(inContext) && val_is_object(inContext))
	{
		val_iter_fields(inContext, haxe_iter_global, l);
	}

	// run the script
	if (luaL_dostring(l, val_string(inScript)) == LUA_OK)
	{
		// convert the lua values to haxe
		int lua_v;
		while ((lua_v = lua_gettop(l)) != 0)
		{
			v = lua_value_to_haxe(l, lua_v);
			lua_pop(l, 1);
		}
	}
	else
	{
		// get error message
		v = alloc_string(lua_tostring(l, -1));
		lua_pop(l, 1);
	}

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
