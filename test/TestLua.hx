class TestLua extends haxe.unit.TestCase
{

	public function testNull()
	{
		assertEquals(null, Lua.run("return null"));
	}

	public function testBoolean()
	{
		assertTrue(Lua.run("return true"));
		assertFalse(Lua.run("return false"));
	}

	public function testArray()
	{
		assertEquals(1, Lua.run("if arr[2] then return 1 else return 2 end", {
			arr: [false, true, false]
		}));
	}

	public function testInteger()
	{
		assertEquals(15, Lua.run("return num", {num: 15}));
	}

	public function testFloat()
	{
		assertEquals(15.3, Lua.run("return num", {num: 15.3}));
	}

	public function testObjectToTable()
	{
		assertTrue(Lua.run('return foo.bar', {foo: {bar: true}}));
	}

	public function testReturnTable()
	{
		var result:Array<Dynamic> = Lua.run('return {"foo", true, 8.3, 6}');
		assertEquals(4, result.length);
		assertEquals("foo", result[0]);
		assertEquals(true, result[1]);
		assertEquals(8.3, result[2]);
		assertEquals(6, result[3]);
	}

	public function testReturnObject()
	{
		var result = Lua.run('return {foo = true, bar = 95}');
		assertEquals(true, result.foo);
		assertEquals(95, result.bar);
	}

	public function testEmbededObjects()
	{
		var result = Lua.run('return {foo = {bar = {baz = 30}, foo = 99}, bar = {14, 52, 25, 1, 7}}');
		assertEquals(30, result.foo.bar.baz);
		assertEquals(99, result.foo.foo);
		assertEquals(5, cast(result.bar, Array<Dynamic>).length);
		assertEquals(25, result.bar[2]);
	}

	public function testFunctionNoArgs()
	{
		assertEquals(true, Lua.run("return num()", {num: function() { return true; }}));
	}

	public function testFunctionArgs()
	{
		assertEquals(15, Lua.run("return num(true, 1)", {
			num: function(a:Bool, b:Int) { return 15; }
		}));
	}

	public function testFunctionPassThrough()
	{
		assertEquals("hello world", Lua.run('return greet(message)', {
			message: "hello world",
			greet: function(greeting:String) { return greeting; }
		}));
	}

	public function testMathLibrary()
	{
		var lua = new Lua();
		lua.loadLibs(["math"]);
		assertEquals(3, lua.execute("return math.floor(3.6)"));
	}

	public function testMultipleInstances()
	{
		var l1 = new Lua(),
			l2 = new Lua();

		var context = {foo: 1};
		l1.setVars(context);

		assertEquals(1, l1.execute("return foo"));

		// change the context for l2
		context.foo = 2;
		l2.setVars(context);

		assertEquals(1, l1.execute("return foo"));
		assertEquals(2, l2.execute("return foo"));

		// change foo on l1 but not l2
		context.foo = 3;
		l1.setVars(context);

		assertEquals(3, l1.execute("return foo"));
		assertEquals(2, l2.execute("return foo"));
	}

	public static function main()
	{
		var runner = new haxe.unit.TestRunner();
		runner.add(new TestLua());
		runner.run();
	}

}
