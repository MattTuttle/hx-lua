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

	public static function main()
	{
		var runner = new haxe.unit.TestRunner();
		runner.add(new TestLua());
		runner.run();
	}

}
