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
