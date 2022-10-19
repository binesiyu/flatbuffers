local m = {}

_G.flatbuffersnative = require("flatbuffersnative")

local N = _G.flatbuffersnative.N
m.N = N

m.binaryArray = {}
m.binaryArray.New = _G.flatbuffersnative.new_binaryarray

local view = {}
view.New = _G.flatbuffersnative.new_view
view.NewEmpty = _G.flatbuffersnative.new_view_empty
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
                            if f then
                                return f(t)
                            end

                            assert(false,key)
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

function m.GetUnionFun(size)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            local obj = view.NewEmpty()
            self.__view:Union(obj, o)
            return obj
        end
    end
end

function m.GetSubFun(size,path,isTable)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            local x
            if isTable then
                x = self.__view:Indirect(self.__view.pos)
            else
                x = o + self.__view.pos
            end
            local obj = require(path).__New(self.__view.bytes, x)
            return obj
        end
    end
end

local function commonipairs(t)
    local idx = 0
    local l = #t
    return function()
            idx = idx + 1
            if idx <= l then
                return idx, t[idx]
            end
        end
end

function m.GetArrayFun(size,ntype,ntypesize,key,default)
    return function(self)
        local ret = rawget(self, key)
        if ret then
            return ret
        end
        ret = setmetatable({}, {
                __len = function(_)
                    local o = self.__view:Offset(size)
                    if o ~= 0 then
                        return self.__view:VectorLen(o)
                    end
                    return 0
                end,

                __index = function(_, j)
                    if type(j) == 'number' then
                        local o = self.__view:Offset(size)
                        if o ~= 0 then
                            local a = self.__view:Vector(o)
                            return self.__view:Get(ntype, a + ((j-1) * ntypesize))
                        end
                        return default
                    else
                        return rawget(self, j)
                    end
                end,

                __ipairs = commonipairs,
                }
        )
        rawset(self, key, ret)
        return ret
    end
end

function m.GetArraySubFun(size,path,ntypesize,key,isTable)
    return function(self)
        local ret = rawget(self, key)
        if ret then
            return ret
        end
        ret = setmetatable({}, {
                __len = function(_)
                    local o = self.__view:Offset(size)
                    if o ~= 0 then
                        return self.__view:VectorLen(o)
                    end
                    return 0
                end,

                __index = function(_, j)
                    if type(j) == 'number' then
                        local o = self.__view:Offset(size)
                        if o ~= 0 then
                            local x = self.__view:Vector(o)
                            x = x + ((j-1) * ntypesize)
                            if isTable then
                                x = self.__view:Indirect(x)
                            end
                            local obj = require(path).__New(self.__view.bytes, x)
                            return obj
                        end
                    else
                        return rawget(self, j)
                    end
                end,

                __ipairs = commonipairs,
                }
        )
        rawset(self, key, ret)
        return ret
    end
end

return m
