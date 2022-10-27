-- need to update the Lua path to point to the local flatbuffers implementation
-- package.cpath = string.format("/Users/yubin/.luarocks/lib/lua/5.3/?.so;%s",package.path)
-- package.cpath = string.format("/Users/yubin/Documents/dev/lua/lua-flatbuffers-master/external/lua-flatbuffers/?.so;%s",package.cpath)
-- package.path = string.format("/Users/yubin/Documents/dev/lua/lua-flatbuffers-master/external/lua-flatbuffers/?.lua;%s",package.path)
-- package.cpath = string.format("/Users/yubin/Library/Developer/Xcode/DerivedData/flatbuffersnative-erfjxmselgihdweufjfcsooyxvke/Build/Products/Debug/?.dylib;%s",package.cpath)
package.cpath = string.format("../lua_fix/?.so;%s",package.cpath)
package.path = string.format("../lua_fix/?.lua;%s",package.path)
package.path = string.format("/Users/yubin/Documents/dev/lua/luban_examples-main/MiniTemplate/output_code/?.lua;%s",package.path)

-- require the library

local function checkReadBuffer(buf, offset, sizePrefix)
    offset = offset or 0

    print("bufAsString",string.len(buf))
    local s = string.gsub(buf,"(.)",function (x) return string.format("%d ",string.byte(x)) end)
    print(s)


    -- collectgarbage("collect")
    -- print("释放前lua:", collectgarbage("count"))
    local monroot = require("cfg.Item")

    for i,mon in ipairs(monroot) do
        print("===========",mon.id,mon.name)
        for k,v in pairs(mon.exchange_list) do
            print("pairs",k,v.id,v.num)
        end
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
