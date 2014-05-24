class TestLua extends haxe.unit.TestCase
{

	public function testBoolean()
	{
		assertTrue(Lua.run("return true"));
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
		assertTrue(Lua.run('return foo["bar"]', {foo: {bar: true}}));
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

	public static function main()
	{
		var runner = new haxe.unit.TestRunner();
		runner.add(new TestLua());
		runner.run();
	}

}
