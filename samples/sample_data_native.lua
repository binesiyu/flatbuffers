-- need to update the Lua path to point to the local flatbuffers implementation
-- package.cpath = string.format("/Users/yubin/.luarocks/lib/lua/5.3/?.so;%s",package.path)
-- package.cpath = string.format("/Users/yubin/Documents/dev/lua/lua-flatbuffers-master/external/lua-flatbuffers/?.so;%s",package.cpath)
-- package.path = string.format("/Users/yubin/Documents/dev/lua/lua-flatbuffers-master/external/lua-flatbuffers/?.lua;%s",package.path)
-- package.cpath = string.format("/Users/yubin/Library/Developer/Xcode/DerivedData/flatbuffersnative-erfjxmselgihdweufjfcsooyxvke/Build/Products/Debug/?.dylib;%s",package.cpath)
package.cpath = string.format("../lua_fix/?.so;%s",package.cpath)
package.path = string.format("../lua_fix/?.lua;%s",package.path)
package.path = string.format("./luafix/?.lua;%s",package.path)

-- require the library
local weapon = require("MyGame.Sample.Weapon")
local equipment = require("MyGame.Sample.Equipment")

local function checkReadBuffer(buf, offset, sizePrefix)
    offset = offset or 0

    print("bufAsString",string.len(buf))
    local s = string.gsub(buf,"(.)",function (x) return string.format("%d ",string.byte(x)) end)
    print(s)


    -- collectgarbage("collect")
    -- print("释放前lua:", collectgarbage("count"))
    local monroot = require("MyGame.Sample.Monster")
    print("id",monroot[3].id)
    collectgarbage("collect")
    print("id",monroot[3].id)
    print("id",monroot[3].id)
    for id,mon in pairs(monroot) do
        -- print(id,"id")
    end

    for i,mon in ipairs(monroot) do
        -- print(i,"i")
        collectgarbage("collect")
        -- print("释放后lua:", collectgarbage("count"))

        for k,v in pairs(mon) do
            -- print("pairs",k,v)
        end
        -- mon.test = 1
        assert(mon.hp == 300, "Monster Hp is not 300")
        assert(mon.hp == 300, "Monster Hp is not 300")
        assert(mon.hp == 300, "Monster Hp is not 300")
        assert(mon.mana == 150, "Monster Mana is not 150")
        assert(mon.mana == 150, "Monster Mana is not 150")
        assert(mon.name == "Orc", "Monster Name is not MyMonster")
        assert(mon.name == "Orc", "Monster Name is not MyMonster")
        assert(mon.name == "Orc", "Monster Name is not MyMonster")
        assert(mon.pos.x == 10.0)
        assert(mon.pos.x == 10.0)
        assert(mon.pos.y == 2.0)
        assert(mon.pos.z == 3.0)
        assert(mon.isnpc == true)

        local expected = {
            {w = 'axe', d = 100},
            {w = 'bow', d = 90}
        }

        collectgarbage("collect")
        print(mon.equipped)
        print(mon.equipped)
        collectgarbage("collect")
        print(mon.equipped)
        print(mon.equipped)

        for i=1,#mon.weapons do
            for k,v in pairs(mon.weapons[i]) do
                -- print("pairs--weapons",k,v)
            end
           if expected[i] then
               assert(mon.weapons[i].name == expected[i].w)
               assert(mon.weapons[i].damage == expected[i].d)
           end
        end

        for name,w in pairs(mon.weapons) do
            -- print("pairs",name,w.damage)
        end

        if i == 1 then
            assert(mon.weapons.cxe.damage == 110)
            assert(mon.weapons.dxe.damage == 120)
            assert(mon.weapons.xxe.damage == 130)
            assert(mon.weapons.zxe.damage == 140)
        end

        -- print(mon.weapons.axe.damage)
        assert(mon.weapons.axe.damage == 100)
        assert(mon.equipped_type == equipment.Weapon)

        local unionWeapon = weapon(mon.equipped.bytes,mon.equipped.pos)
        assert(unionWeapon.name == "bow")
        assert(unionWeapon.damage == 90)
    end
end

local function testCanonicalData()
    local f = assert(io.open('monsterdata.bin', 'rb'))
    local wireData = f:read("*a")
    f:close()
    checkReadBuffer(wireData)
end

collectgarbage("collect")
print("释放前lua:", collectgarbage("count"))
testCanonicalData()
collectgarbage("collect")
print("释放后lua:", collectgarbage("count"))

print("The Lua FlatBuffer example was successfully created and verified!")
