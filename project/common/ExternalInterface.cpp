#ifndef STATIC_LINK
#define IMPLEMENT_API
#endif

#if defined(HX_WINDOWS) || defined(HX_MACOS) || defined(HX_LINUX)
#define NEKO_COMPATIBLE
#endif

#include <hx/CFFI.h>
#include <cmath>
#include <cstring>

extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

vkind kind_lua_vm;

// HACK!!! For some reason this isn't properly defined in neko...
#if !(defined(IPHONE) || defined(ANDROID))
#define val_fun_nargs(v)	((vfunction*)(v))->nargs
#endif

// forward declarations
int haxe_to_lua(value v, lua_State *l);
value lua_value_to_haxe(lua_State *l, int lua_v);

#define BEGIN_TABLE_LOOP(l, v) lua_pushnil(l); \
	while (lua_next(l, v) != 0) {
#define END_TABLE_LOOP(l) lua_pop(l, 1); }

inline value lua_table_to_haxe(lua_State *l, int lua_v)
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
			if(arr)
			{
				arr[index] = lua_value_to_haxe(l, lua_v+2);
			}
			else
			{
				val_array_set_i(v, index, lua_value_to_haxe(l, lua_v+2));
			}
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
		case LUA_TFUNCTION:
		case LUA_TUSERDATA:
		case LUA_TTHREAD:
		case LUA_TLIGHTUSERDATA:
			printf("return value not supported");
			break;
	}
	return v;
}

static int haxe_callback(lua_State *l)
{
	int num_args = lua_gettop(l);
	AutoGCRoot *root = (AutoGCRoot *)lua_topointer(l, lua_upvalueindex(1));
	int expected_args = lua_tonumber(l, lua_upvalueindex(2));
	if (num_args != expected_args)
	{
		printf("Expected %d arguments, received %d", expected_args, num_args);
	}
	else
	{
		value *args = new value[num_args];
		for (int i = 0; i < num_args; ++i)
		{
			args[i] = lua_value_to_haxe(l, i + 1);
		}
		value result = val_callN(root->get(), args, num_args);
		delete [] args;
		return haxe_to_lua(result, l);
	}
	return 0;
}

inline void haxe_array_to_lua(value v, lua_State *l)
{
	int size = val_array_size(v);
	value *arr = val_array_value(v);

	lua_createtable(l, size, 0);
	for (int i = 0; i < size; i++)
	{
		lua_pushnumber(l, i + 1); // lua index is 1 based instead of 0
		if(arr)
		{
			haxe_to_lua(arr[i], l);
		}
		else
		{
			haxe_to_lua(val_array_i(v, i), l);
		}

		lua_settable(l, -3);
	}
}

void haxe_iter_object(value v, field f, void *state)
{
	lua_State *l = (lua_State *)state;
	const char *name = val_string(val_field_name(f));
	lua_pushstring(l, name);
	haxe_to_lua(val_field(v, f), l);
	lua_settable(l, -3);
}

void haxe_iter_global(value v, field f, void *state)
{
	lua_State *l = (lua_State *)state;
	const char *name = val_string(val_field_name(f));
	haxe_to_lua(val_field(v, f), l);
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
			// TODO: figure out a way to delete/cleanup the AutoGCRoot pointers
			lua_pushlightuserdata(l, new AutoGCRoot(v));
			lua_pushnumber(l, val_fun_nargs(v));
			// using a closure instead of a function so we can add upvalues
			lua_pushcclosure(l, haxe_callback, 2);
			break;
		case valtArray:
			haxe_array_to_lua(v, l);
			break;
		case valtAbstractBase: // should abstracts be handled??
			printf("abstracts not supported");
			return 0;
		case valtObject: // falls through
		case valtEnum: // falls through
		case valtClass:
			lua_newtable(l);
			val_iter_fields(v, haxe_iter_object, l);
			break;
	}
	return 1;
}

static lua_State *lua_from_handle(value inHandle)
{
	if (val_is_kind(inHandle, kind_lua_vm))
	{
		lua_State *l = (lua_State *)val_to_kind(inHandle, kind_lua_vm);
		return l;
	}
	return NULL;
}

static void release_lua(value inHandle)
{
	lua_State *l = lua_from_handle(inHandle);
	if (l)
	{
		lua_close(l);
	}
}

static value lua_create()
{
	lua_State *l = luaL_newstate();
	value result = alloc_abstract(kind_lua_vm, l);
	val_gc(result, release_lua);
	return result;
}
DEFINE_PRIM(lua_create, 0);

static value lua_get_version()
{
	return alloc_string(LUA_VERSION);
}
DEFINE_PRIM(lua_get_version, 0);

static value lua_load_libs(value inHandle, value inLibs)
{
	static const luaL_Reg lualibs[] = {
		{ "base", luaopen_base },
		{ "debug", luaopen_debug },
		{ "io", luaopen_io },
		{ "math", luaopen_math },
		{ "os", luaopen_os },
		{ "package", luaopen_package },
		{ "string", luaopen_string },
		{ "table", luaopen_table },
		{ NULL, NULL }
	};

	lua_State *l = lua_from_handle(inHandle);
	if (l)
	{
		int numLibs = val_array_size(inLibs);
		value *libs = val_array_value(inLibs);

		for (int i = 0; i < numLibs; i++)
		{
			const luaL_Reg *lib = lualibs;
			for (;lib->func != NULL; lib++)
			{
				const char* libname;

				if(libs)
				{
					libname = val_string(libs[i]);
				}
				else
				{
					libname = val_string(val_array_i(inLibs, i));
				}

				if (strcmp(libname, lib->name) == 0)
				{
					// printf("loading lua library %s\n", lib->name);
					luaL_requiref(l, lib->name, lib->func, 1);
					lua_settop(l, 0);
					break;
				}
			}
		}
	}
	return alloc_null();
}
DEFINE_PRIM(lua_load_libs, 2);

static value lua_load_context(value inHandle, value inContext)
{
	lua_State *l = lua_from_handle(inHandle);
	if (l)
	{
		// load context, if any
		if (!val_is_null(inContext) && val_is_object(inContext))
		{
			val_iter_fields(inContext, haxe_iter_global, l);
		}
	}
	return alloc_null();
}
DEFINE_PRIM(lua_load_context, 2);

static value lua_call_function(value inHandle, value inFunction, value inArgs)
{
	const char *func = val_get_string(inFunction);
	lua_State *l = lua_from_handle(inHandle);
	if (l)
	{
		lua_getglobal(l, func);

		int numArgs = 1;
		if (val_is_array(inArgs))
		{
			numArgs = val_array_size(inArgs);
			value *args = val_array_value(inArgs);
			for (int i = 0; i < numArgs; i++)
			{
				if(args)
				{
					haxe_to_lua(args[i], l);
				}
				else
				{
					haxe_to_lua(val_array_i(inArgs,i), l);
				}
			}
		}
		else
		{
			haxe_to_lua(inArgs, l);
		}

		if (lua_pcall(l, numArgs, 1, 0) == 0)
		{
			value v = lua_value_to_haxe(l, -1);
			lua_pop(l, 1);
			return v;
		}
	}
	return alloc_null();
}
DEFINE_PRIM(lua_call_function, 3);

static value lua_execute(value inHandle, value inScript)
{
	value v;
	lua_State *l = lua_from_handle(inHandle);
	if (l)
	{
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
		return v;
	}
	return alloc_null();
}
DEFINE_PRIM(lua_execute, 2);

extern "C" void lua_main()
{
	kind_share(&kind_lua_vm, "lua::vm"); // Fix Neko init
}
DEFINE_ENTRY_POINT(lua_main);

extern "C" int lua_register_prims() { return 0; }
