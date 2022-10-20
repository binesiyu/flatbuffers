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

local F = {}
m.F = F


local function createObj(buf,offset,sizePrefix)
    local o = {}
    if sizePrefix then
        local n = N.UOffsetT:Unpack(buf, offset)
        offset = offset + n
    end

    o.__view = view.New(buf, offset)
    return o
end

local function createIndex(mt)
    return function(t,key)
        local f = rawget(mt, key)
        if f then
            return f(t)
        end

        assert(false,key)
    end
end

local function createIndexCache(mt)
    return function(t,key)
        local f = rawget(mt, key)
        if f then
            local value = f(t)
            --值缓存起来
            rawset(t,key,value)
            return value
        end

        assert(false,key)
    end
end

local function createIndexCacheWeak(mt,index,obj)
    return function(_,key)
        local f = rawget(mt, key)
        if f then
            local value = f(obj)
            --值缓存起来
            rawset(index,key,value)
            return value
        end

        assert(false,key)
    end
end

-- local tablereadonly = function(_,key,value)
--     assert(false,string.format("table is readonly key[%s] value[%s]",key,value))
-- end

local function commonpairs(cfg)
    return function(tbl)
      -- Iterator function takes the table and an index and returns the next index and associated value
      -- or nil to end iteration
      local function stateless_iter(t, k)
        local cv
        -- Implement your own key,value selection logic in place of next
        k, cv = next(cfg, k)
        if nil~= cv then
            local v = t[k]
            if nil~=v then
                return k,v
            end
        end
      end

      -- Return an iterator function, the table, starting point
      return stateless_iter, tbl, nil
  end
end

local function createMetaDefault(cfg)
    local meta = {
        -- __call = function(self,buf,pos)
        --     self.view = fb.view.New(buf, pos)
        -- end
        -- __newindex = tablereadonly,
        -- __newindex = rawset,
        __pairs = commonpairs(cfg),
    }
    return meta
end

local function createMeta(cfg)
    local meta = createMetaDefault(cfg)
    meta.__index = createIndex(cfg)
    return meta
end

--使用cache
local function createMetaCache(cfg)
    local meta = createMetaDefault(cfg)
    meta.__index = createIndexCache(cfg)
    return meta
end

--使用cache + weak,兼顾性能与内存
local function createMetaCacheWeak(cfg,obj)
    local meta = createMetaDefault(cfg)
    local index = {}
    meta.__index = index

    local metameat = {}
    metameat.__index = createIndexCacheWeak(cfg,index,obj)
    metameat.__mode = "kv"
    -- set mt
    setmetatable(index, metameat)
    return meta
end

function F.New(cfg,buf,offset,sizePrefix)
    local obj = createObj(buf,offset,sizePrefix)
    -- set mt
    setmetatable(obj, createMeta(cfg))
    return obj
end

function F.NewCache(cfg,buf,offset,sizePrefix)
    local obj = createObj(buf,offset,sizePrefix)
    -- set mt
    setmetatable(obj, createMetaCache(cfg))
    return obj
end

function F.NewCacheWeak(cfg,buf,offset,sizePrefix)
    local obj = createObj(buf,offset,sizePrefix)
    -- set mt
    setmetatable(obj, createMetaCacheWeak(cfg,obj))
    return obj
end


--产生一个配置表项
function F.NewCfg()
    local cfg = {}
    -- set mt
    setmetatable(cfg, {
        __call = F.New,
        -- __call = F.NewCacheWeak,
     })
    return cfg
end

function F.FunField(size,ntype,default)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            return self.__view:Get(ntype, o + self.__view.pos)
        end
        return default
    end
end

function F.FunFieldStruce(size,ntype)
    return function(self)
        return self.__view:Get(ntype, self.__view.pos + size)
    end
end

function F.FunFieldBool(size,ntype,default)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            return self.__view:Get(ntype, o + self.__view.pos) ~= 0
        end
        return default
    end
end

function F.FunFieldString(size)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            return self.__view:String(o + self.__view.pos)
        end
    end
end

function F.FunUnion(size)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            local obj = view.NewEmpty()
            self.__view:Union(obj, o)
            return obj
        end
    end
end

function F.FunSub(size,path,isTable)
    return function(self)
        local o = self.__view:Offset(size)
        if o ~= 0 then
            local x
            if isTable then
                x = self.__view:Indirect(self.__view.pos)
            else
                x = o + self.__view.pos
            end
            local cfg = require(path)
            local obj = cfg(self.__view.bytes, x)
            return obj
        end
    end
end


local function commonipairs(t)
    -- Iterator function
    local l = #t
    local function stateless_iter(tbl, i)
        -- Implement your own index, value selection logic
        i = i + 1
        if i <= l then
            return i, tbl[i]
        end
    end

    -- return iterator function, table, and starting point
    return stateless_iter, t, 0
end

function F.FunArray(size,ntype,ntypesize,key,default)
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

function F.FunArrayCfg(size,cfg,ntypesize,key,isTable)
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
                            local obj = cfg(self.__view.bytes, x)
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

function F.FunArraySub(size,path,ntypesize,key,isTable)
    local cfg = require(path)
    return F.FunArrayCfg(size,cfg,ntypesize,key,isTable)
end

local function getFileData(name)
    local f = assert(io.open('monsterdata.bin', 'rb'))
    local fileData = f:read("*a")
    f:close()
    return fileData
end

function F.createCfg(name,mt)
    local MonsterRoot = F.NewCfg()
    MonsterRoot.items = F.FunArrayCfg(4,mt,4,'_fb_items_arr',true)

    local fileData = getFileData(name)
    if fileData then
        local data = MonsterRoot(m.binaryArray.New(fileData), 0,true)
        return data and data.items or {}
    end
    return {}
end

return m