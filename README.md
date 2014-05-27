Run Lua code in Haxe
====================

Run any Lua code inside Haxe on neko/cpp targets. Has the option of passing a context object that will set variables before running the script.

```haxe
var result = Lua.run("return true"); // returns a Bool to Haxe
```

Pass in values with a context object. The key names are used as variable names in the Lua script.
```haxe
var result = Lua.run("if num > 14 then return 14 else return num end", {num: 15.3});
```

What if you want to call a function created in Haxe from Lua? Just pass the function in the context!
```haxe
var result = Lua.run("return plus1(15)", { plus1: function(val:Int) { return val + 1; } });
if (result == 16)
	trace("success!");
```

Lua instances
=============

It's possible to create multiple Lua instances to run scripts with different contexts/libraries.

```haxe
var lua = new Lua();
lua.loadLibs(["base", "math"]);
lua.setVars({ myVar: 1 });
var result = lua.execute("return myVar");
```
