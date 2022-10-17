-- need to update the Lua path to point to the local flatbuffers implementation
package.path = string.format("/Users/yubin/.luarocks/lib/lua/5.3/?.so;%s",package.path)
-- package.path = string.format("../lua/?.so;%s",package.path)
-- package.path = string.format("../lua/?.lua;%s",package.path)
-- package.path = string.format("/Users/yubin/Documents/dev/lua/lua-flatbuffers-master/external/lua-flatbuffers/?.so;%s",package.path)
-- package.path = string.format("/Users/yubin/Documents/dev/lua/lua-flatbuffers-master/external/lua-flatbuffers/?.lua;%s",package.path)
package.path = string.format("./luafix/?.lua;%s",package.path)

-- require the library
local flatbuffers = require("flatbuffers")
local monster = require("MyGame.Sample.Monster")

local function checkReadBuffer(buf, offset, sizePrefix)
    offset = offset or 0

    if type(buf) == "string" then
        buf = flatbuffers.binaryArray.New(buf)
    end

    if sizePrefix then
        local size = flatbuffers.N.Int32:Unpack(buf, offset)
        assert(size == buf.size - offset - 4)
        offset = offset + flatbuffers.N.Int32.bytewidth
    end

    local mon = monster.GetRootAsMonster(buf, offset)
    assert(mon.hp == 300, "Monster Hp is not 300")
    assert(mon.mana == 150, "Monster Mana is not 150")
    assert(mon.name == "Orc", "Monster Name is not MyMonster")
    assert(mon.pos.x == 10.0)
    assert(mon.pos.y == 2.0)
    assert(mon.pos.z == 3.0)
end

local function testCanonicalData()
    local f = assert(io.open('monsterdata.bin', 'rb'))
    local wireData = f:read("*a")
    f:close()
    checkReadBuffer(wireData)
end

testCanonicalData()

print("The Lua FlatBuffer example was successfully created and verified!")
