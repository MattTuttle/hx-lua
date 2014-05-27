#if cpp
import cpp.Lib;
#elseif neko
import neko.Lib;
#end

class Lua
{

	/**
	 * Creates a new lua vm state
	 */
	public function new()
	{
		handle = lua_create();
	}

	/**
	 * Loads lua libraries (base, debug, io, math, os, package, string, table)
	 * @param libs An array of library names to load
	 */
	public function loadLibs(libs:Array<String>)
	{
		lua_load_libs(handle, libs);
	}

	/**
	 * Defines variables in the lua vars
	 * @param vars An object defining the lua variables to create
	 */
	public function setVars(vars:Dynamic)
	{
		lua_load_context(handle, vars);
	}

	/**
	 * Runs a lua script
	 * @param script The lua script to run in a string
	 * @return The result from the lua script in Haxe
	 */
	public function execute(script:String):Dynamic
	{
		return lua_run(handle, script);
	}

	/**
	 * Convienient way to run a lua script in Haxe without loading any libraries
	 * @param script The lua script to run in a string
	 * @param vars An object defining the lua variables to create
	 * @return The result from the lua script in Haxe
	 */
	public static function run(script:String, ?vars:Dynamic):Dynamic
	{
		var lua = new Lua();
		lua.setVars(vars);
		return lua.execute(script);
	}

	private static function load(func:String, numArgs:Int):Dynamic
	{
#if neko
		if (!moduleInit)
		{
			// initialize neko
			var init = Lib.load("lua", "neko_init", 5);
			if (init != null)
			{
				init(function(s) return new String(s), function(len:Int) { var r = []; if (len > 0) r[len - 1] = null; return r; }, null, true, false);
			}
			else
			{
				throw("Could not find NekoAPI interface.");
			}

			moduleInit = true;
		}
#end

		return Lib.load("lua", func, numArgs);
	}

	private var handle:Dynamic;

	private static var lua_create = load("lua_create", 0);
	private static var lua_run = load("lua_run", 2);
	private static var lua_load_context = load("lua_load_context", 2);
	private static var lua_load_libs = load("lua_load_libs", 2);
	private static var moduleInit:Bool = false;

}
