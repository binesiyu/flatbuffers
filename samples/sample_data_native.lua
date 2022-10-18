-- need to update the Lua path to point to the local flatbuffers implementation
-- package.cpath = string.format("/Users/yubin/.luarocks/lib/lua/5.3/?.so;%s",package.path)
-- package.cpath = string.format("/Users/yubin/Documents/dev/lua/lua-flatbuffers-master/external/lua-flatbuffers/?.so;%s",package.cpath)
-- package.path = string.format("/Users/yubin/Documents/dev/lua/lua-flatbuffers-master/external/lua-flatbuffers/?.lua;%s",package.path)
package.path = string.format("../lua_fix/?.lua;%s",package.path)
package.path = string.format("./luafix/?.lua;%s",package.path)

-- require the library
local flatbuffers = require("flatbuffers")
local weapon = require("MyGame.Sample.Weapon")
local monster = require("MyGame.Sample.Monster")
local color = require("MyGame.Sample.Color")
local equipment = require("MyGame.Sample.Equipment")

local function checkReadBuffer(buf, offset, sizePrefix)
    offset = offset or 0

    if type(buf) == "string" then
        print("bufAsString",string.len(buf))
        local s = string.gsub(buf,"(.)",function (x) return string.format("%d ",string.byte(x)) end)
        print(s)

        buf = flatbuffers.binaryArray.New(buf)
    end


    if sizePrefix then
        local size = flatbuffers.N.Int32:Unpack(buf, offset)
        assert(size == buf.size - offset - 4)
        offset = offset + flatbuffers.N.Int32.bytewidth
    end

    local mon = monster.__New(buf, offset,true)
    assert(mon.hp == 300, "Monster Hp is not 300")
    assert(mon.mana == 150, "Monster Mana is not 150")
    assert(mon.name == "Orc", "Monster Name is not MyMonster")
    assert(mon.pos.x == 10.0)
    assert(mon.pos.y == 2.0)
    assert(mon.pos.z == 3.0)
    assert(mon.isnpc == true)

    local expected = {
        {w = 'axe', d = 100},
        {w = 'bow', d = 90}
    }

    for i=1,mon.weaponsLength do
       assert(mon.weapons[i].name == expected[i].w)
       assert(mon.weapons[i].damage == expected[i].d)
    end

    assert(mon.equipped_type == equipment.Weapon)

    local unionWeapon = weapon.__New(mon.equipped.bytes,mon.equipped.pos)
    assert(unionWeapon.name == "bow")
    assert(unionWeapon.damage == 90)
end

local function testCanonicalData()
    local f = assert(io.open('monsterdata.bin', 'rb'))
    local wireData = f:read("*a")
    f:close()
    checkReadBuffer(wireData)
end

testCanonicalData()

print("The Lua FlatBuffer example was successfully created and verified!")
