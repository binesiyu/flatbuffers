local m = {}

_G.flatbuffersnative = require("flatbuffersnative")

local N = _G.flatbuffersnative.N
m.N = N

local binaryArray = {}
binaryArray.New = _G.flatbuffersnative.new_binaryarray
m.binaryArray = binaryArray

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

local function createIndexCacheWeak(mt,cacheTable,obj)
    return function(_,key)
        local f = rawget(mt, key)
        if f then
            local value = f(obj)
            --值缓存起来
            rawset(cacheTable,key,value)
            return value
        end

        assert(false,key)
    end
end

local tablereadonly = function(_,key,value)
    assert(false,string.format("table is readonly key[%s] value[%s]",key,value))
end

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
        __newindex = tablereadonly,
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
    local cacheTable = {}
    meta.__index = cacheTable

    local metameat = {}
    metameat.__index = createIndexCacheWeak(cfg,cacheTable,obj)
    metameat.__mode = "kv"
    -- set mt
    setmetatable(cacheTable, metameat)
    return meta
end

local function New(cfg,buf,offset,sizePrefix)
    local obj = createObj(buf,offset,sizePrefix)
    -- set mt
    setmetatable(obj, createMeta(cfg))
    return obj
end

local function NewCache(cfg,buf,offset,sizePrefix)
    local obj = createObj(buf,offset,sizePrefix)
    -- set mt
    setmetatable(obj, createMetaCache(cfg))
    return obj
end

local function NewCacheWeak(cfg,buf,offset,sizePrefix)
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
        __call = New,
        -- __call = NewCacheWeak,
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

function F.FunFieldStruct(size,ntype)
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

local function arrayipairs(t)
    -- Iterator function
    local l = #t
    local function stateless_iter(tbl, i)
        -- Implement your own index, value selection logic
        i = i + 1
        if i <= l then
            return i, tbl(i)
        end
    end

    -- return iterator function, table, and starting point
    return stateless_iter, t, 0
end

--以int等类型为key的pairs
local function arraypairs(keyname)
    -- Iterator function
    return function(t)
        local l = #t
        local i = 0
        local function stateless_iter(tbl, _)
            -- Implement your own index, value selection logic
            i = i + 1
            if i <= l then
                local value = tbl(i)
                if value then
                    return value[keyname], value
                end
            end
        end

        -- return iterator function, table, and starting point
        return stateless_iter, t, 0
    end
end

--以string为key的pairs
local function arraypairsstring(keyname)
    -- Iterator function
    return function(t)
        local l = #t
        local i = 0
        local function stateless_iter(tbl, _)
            -- Implement your own index, value selection logic
            i = i + 1
            if i <= l then
                local value = tbl[i]
                if value then
                    return value[keyname], value
                end
            end
        end

        -- return iterator function, table, and starting point
        return stateless_iter, t, 0
    end
end

local function createFunArrayLen(self,size)
    return function()
            local o = self.__view:Offset(size)
            if o ~= 0 then
                return self.__view:VectorLen(o)
            end
            return 0
        end
end

local function createArrayMetaDefault(self,size)
    local meta = {
        -- __call = function(self,buf,pos)
        --     self.view = fb.view.New(buf, pos)
        -- end
        __newindex = tablereadonly,
        -- __newindex = rawset,
        -- __mode = "kv",
        __len = createFunArrayLen(self,size),
        __ipairs = commonipairs,
        __pairs = commonipairs
    }
    return meta
end

local function createArrayFun(size,cachekey,creatfun)
    return function(self)
        local ret = rawget(self, cachekey)
        if ret then
            return ret
        end

        local meta = createArrayMetaDefault(self,size)
        local obj = {}
        creatfun(self,meta)
        ret = setmetatable(obj, meta)
        rawset(self, cachekey, ret)
        return ret
    end
end

local function createArrayFunCache(size,cachekey,creatfun)
    return function(self)
        local ret = rawget(self, cachekey)
        if ret then
            return ret
        end

        local meta = createArrayMetaDefault(self,size)
        local obj = {}
        local cacheTableIndex = {}
        creatfun(self,meta,obj,cacheTableIndex)
        ret = setmetatable(obj, meta)
        rawset(self, cachekey, ret)
        return ret
    end
end

local metaCacheIndex = {
    __mode = "kv",
}

local function createArrayFunCacheWeak(size,cachekey,creatfun)
    return function(self)
        local ret = rawget(self, cachekey)
        if ret then
            return ret
        end

        local meta = createArrayMetaDefault(self,size)

        local obj = {}
        local cacheTable = {}
        local cacheTableIndex = {}
        setmetatable(cacheTableIndex, metaCacheIndex)

        creatfun(self,meta,cacheTable,cacheTableIndex)
        local metameat = {}
        metameat.__index = meta.__index
        metameat.__mode = "kv"

        meta.__index = cacheTable

        -- set mt
        setmetatable(cacheTable, metameat)

        ret = setmetatable(obj, meta)

        rawset(self, cachekey, ret)
        return ret
    end
end

function F.FunArray(size,ntype,ntypesize,cachekey,default)
    return createArrayFun(size,cachekey,function(self,mt)
            mt.__index = function(_, j)
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
                end
            end)
end

function F.FunArrayCfg(size,cfg,ntypesize,cachekey,isTable,keyname,key,iskeynum,keytype)
    if key then
        if iskeynum then
            return createArrayFun(size,cachekey,function(self,mt,cacheTable,cacheTableIndex)
                mt.__index = function(_, j)
                    if type(j) == 'number' then
                        local o = self.__view:Offset(size)
                        if o ~= 0 then
                            local x = self.__view:Vector(o)
                            local index

                            -- x = x + ((j-1) * ntypesize)
                            x,index = self.__view:LookUpNum(x,ntypesize,key,isTable,keytype,j)
                            if x >= 0 then
                                local obj = cfg(self.__view.bytes, x)
                                --值缓存起来
                                if cacheTable then
                                    rawset(cacheTable,j,obj)
                                end
                                if cacheTableIndex then
                                    rawset(cacheTableIndex,index,obj)
                                end
                                return obj
                            end
                        end
                    else
                        return rawget(self, j)
                    end
                end
                --用call指代原来的__index
                mt.__call = function(_, j)
                    if type(j) == 'number' then
                        --优先从缓存里读取
                        if cacheTableIndex and cacheTableIndex[j] then
                            return cacheTableIndex[j]
                        end

                        local o = self.__view:Offset(size)
                        if o ~= 0 then
                            local x = self.__view:Vector(o)
                            x = x + ((j-1) * ntypesize)
                            if isTable then
                                x = self.__view:Indirect(x)
                            end
                            local obj = cfg(self.__view.bytes, x)
                            --值缓存起来
                            if cacheTable then
                                local keyvalue = obj[keyname]
                                -- if not rawget(cacheTable,keyvalue) then
                                    rawset(cacheTable,keyvalue,obj)
                                -- end
                            end
                            if cacheTableIndex then
                                rawset(cacheTableIndex,j,obj)
                            end
                            return obj
                        end
                    else
                        return rawget(self, j)
                    end
                end
                mt.__ipairs = arrayipairs
                mt.__pairs = arraypairs(keyname)
            end)
        else
            return createArrayFun(size,cachekey,function(self,mt,cacheTable,cacheTableIndex)
                mt.__index = function(_, j)
                    if type(j) == 'number' then
                        --优先从缓存里读取
                        if cacheTableIndex and cacheTableIndex[j] then
                            return cacheTableIndex[j]
                        end
                        local o = self.__view:Offset(size)
                        if o ~= 0 then
                            local x = self.__view:Vector(o)
                            x = x + ((j-1) * ntypesize)
                            if isTable then
                                x = self.__view:Indirect(x)
                            end
                            local obj = cfg(self.__view.bytes, x)
                            --值缓存起来
                            if cacheTable then
                                local keyvalue = obj[keyname]
                                -- if not rawget(cacheTable,keyvalue) then
                                    rawset(cacheTable,keyvalue,obj)
                                -- end
                            end
                            if cacheTableIndex then
                                rawset(cacheTableIndex,j,obj)
                            end
                            return obj
                        end
                    else
                        local o = self.__view:Offset(size)
                        if o ~= 0 then
                            local x = self.__view:Vector(o)

                            -- x = x + ((j-1) * ntypesize)
                            x = self.__view:LookUpString(x,ntypesize,key,isTable,j)
                            if x >= 0 then
                                local obj = cfg(self.__view.bytes, x)
                                --值缓存起来
                                if cacheTable then
                                    rawset(cacheTable,j,obj)
                                end
                                return obj
                            end
                            return rawget(self, j)
                        end
                        return rawget(self, j)
                    end
                end
                mt.__pairs = arraypairsstring(keyname)
            end)
        end
    else
        return createArrayFun(size,cachekey,function(self,mt,cacheTable)
            mt.__index = function(_, j)
                if type(j) == 'number' then
                    local o = self.__view:Offset(size)
                    if o ~= 0 then
                        local x = self.__view:Vector(o)
                        x = x + ((j-1) * ntypesize)
                        if isTable then
                            x = self.__view:Indirect(x)
                        end
                        local obj = cfg(self.__view.bytes, x)
                        --值缓存起来
                        if cacheTable then
                            rawset(cacheTable,j,obj)
                        end
                        return obj
                    end
                else
                    return rawget(self, j)
                end
            end
        end)
    end
end

function F.FunArraySub(size,path,ntypesize,cachekey,isTable,keyname,key,iskeynum,keytype)
    local cfg = require(path)
    return F.FunArrayCfg(size,cfg,ntypesize,cachekey,isTable,keyname,key,iskeynum,keytype)
end

local function getFileData(name)
    local f = assert(io.open('monsterdata.bin', 'rb'))
    local fileData = f:read("*a")
    f:close()
    return fileData
end

function F.createCfg(name,root)
    local fileData = getFileData(name)
    if fileData then
        local data = root(binaryArray.New(fileData), 0,true)
        return data and data.items or {}
    end
    return {}
end


-------是否开启缓存配置
--默认配置不使用任何缓存
local useCache = 3 --默认配置不使用任何缓存
--只缓存表的创建,保证表是唯一的
if useCache == 1 then
    New = New --各个字段的缓存情况
    createArrayFun = createArrayFunCache--各个struct/table有缓存配置
--全缓存
elseif useCache == 2 then
    New = NewCache
    createArrayFun = createArrayFunCache
--表缓存使用weak机制
elseif useCache == 3 then
    New = NewCache
    createArrayFun = createArrayFunCacheWeak
end
return m
