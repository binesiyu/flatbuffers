local m = {}

_G.flatbuffersnative = require("flatbuffersnative")

local N = _G.flatbuffersnative.N
m.N = N

m.binaryArray = {}
m.binaryArray.New = _G.flatbuffersnative.new_binaryarray

local view = {}
view.New = _G.flatbuffersnative.new_view
m.view = view

function m.New(mt)
    return function(buf,offset,sizePrefix)
        local o = {}
        if sizePrefix then
            local n = N.UOffsetT:Unpack(buf, offset)
            offset = offset + n
        end

        o.__view = view.New(buf, offset)

        -- set mt
        setmetatable(o, {
                     __index = function(t, key)
                            local f = rawget(mt, key)
                            return f(t)
                        end,
                    -- __call = function(self,buf,pos)
                    --     self.view = fb.view.New(buf, pos)
                    -- end
        })
        return o
    end
end

function m.GetFieldFun(size,ntype,default)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            return self.__view:Get(ntype, o + self.__view.pos)
        end
        return default
    end
end

function m.GetFieldFunBool(size,ntype,default)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            return self.__view:Get(ntype, o + self.__view.pos) ~= 0
        end
        return default
    end
end

function m.GetStringFun(size)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            return self.__view:String(o + self.__view.pos)
        end
    end
end

function m.GetSubFun(size,path,isroot)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            local x
            if isroot then
                x = self.__view:Indirect(self.__view.pos)
            else
                x = o + self.__view.pos
            end
            local obj = require(path).__New(self.__view.bytes, x)
            return obj
        end
    end
end

return m
