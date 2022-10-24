#include <vector>
#include <cstring>
#include "lua.hpp"

struct SizedString {
    size_t size;
    const char* string;
};

class BinaryArray {
public:
    BinaryArray(uint32_t size) { data.resize(size); memset(data.data(), 0, size); }
    BinaryArray(SizedString str) { data.resize(str.size); memcpy(data.data(), str.string, str.size); }

    SizedString Slice(uint32_t startPos, uint32_t endPos) {
        if (startPos < 0) startPos = 0;
        if (startPos > data.size()) startPos = (uint32_t)data.size();
        if (endPos < startPos) endPos = startPos;
        if (endPos > data.size()) endPos = (uint32_t)data.size();
        return SizedString{ endPos - startPos, data.data() + startPos };
    }

    uint32_t Size() {
        return (uint32_t)data.size();
    }

    const char* Data(uint32_t position) {
        if (position < 0 || position + 1 > data.size()) return nullptr; // invalid args
        return data.data() + position;
    }

    std::vector<char> data;
};

struct BinaryArrayRef {
    BinaryArray* ptr;
    int count;
    
    BinaryArrayRef(BinaryArray* _ptr)
    {
        ptr = _ptr;
        count = 1;
    }
    
    void retain()
    {
        count++;
    }
    
    void release()
    {
        count--;
        if(count <= 0 && ptr != nullptr)
        {
            delete ptr;
            ptr = nullptr;
        }
    }
};

#define BA_SAFE_RELEASE(p)          do { if(p) { (p)->release(); } } while(0)
#define BA_SAFE_RELEASE_NULL(p)     do { if(p) { (p)->release(); (p) = nullptr; } } while(0)
#define BA_SAFE_RETAIN(p)           do { if(p) { (p)->retain(); } } while(0)

class View {
public:
    View(BinaryArrayRef* _binaryArrayRef, uint32_t position)
    : position(position) {
        binaryArrayRef = _binaryArrayRef;
        BA_SAFE_RETAIN(binaryArrayRef);
        binaryArray = binaryArrayRef->ptr;
    }
    
    View(BinaryArray* _binaryArray, uint32_t position)
    : position(position) {
        binaryArrayRef = nullptr;
        binaryArray = binaryArrayRef->ptr;
    }
    
    View()
    {
        binaryArrayRef = nullptr;
        binaryArray = nullptr;
        position = 0;
    }
    
    ~View()
    {
        BA_SAFE_RELEASE_NULL(binaryArrayRef);
    }
    
    uint32_t Offset(uint32_t vtableOffset) {
        uint32_t vtable = position - Get<int32_t>(position);
        uint32_t vtableEnd = Get<uint16_t>(vtable);
        return vtableOffset < vtableEnd ? Get<uint16_t>(vtable + vtableOffset) : 0;
    }

    uint32_t Indirect(uint32_t offset) {
        return offset + UnpackUInt32(offset);
    }

    SizedString String(uint32_t offset) {
        offset = Indirect(offset);
        uint32_t start = offset + sizeof(uint32_t);
        uint32_t length = UnpackUInt32(offset);
        return binaryArray->Slice(start, start + length);
    }

    uint32_t VectorLen(uint32_t offset) {
        return UnpackUInt32(Indirect(offset + position));
    }

    uint32_t Vector(uint32_t offset) {
        offset += position;
        uint32_t x = offset + Get<uint32_t>(offset);
        x += sizeof(uint32_t);
        return x;
    }
    
    void Union(View* v2, uint32_t offset) {
        offset += position;
        v2->updateBinaryArray(binaryArrayRef,offset + Get<uint32_t>(offset));
    }

    void updateBinaryArray(BinaryArrayRef* _binaryArrayRef, uint32_t position)
    {
        BA_SAFE_RELEASE_NULL(binaryArrayRef);
        
        binaryArrayRef = _binaryArrayRef;
        if(binaryArrayRef != nullptr)
        {
            binaryArrayRef->retain();
            binaryArray = binaryArrayRef->ptr;
            this->position = position;
        }
        else
        {
            binaryArray = nullptr;
            this->position = 0;
        }
    }
    void updateBinaryArray(BinaryArray* _binaryArray, uint32_t position)
    {
        binaryArray = _binaryArray;
        this->position = position;
    }
    
    template<typename T>
    T Get(uint32_t offset) {
        return *((T*)binaryArray->Data(offset));
    }

    uint32_t UnpackUInt32(uint32_t offset) {
        return *((uint32_t*)binaryArray->Data(offset));
    }
    
    BinaryArrayRef* binaryArrayRef;
    BinaryArray* binaryArray;
    uint32_t position;
};
template<typename T> struct CompareToHelper {
  static int CompareTo(const T a, const T b) {
      if (b > a) return -1;
      else if(b < a) return 1;
      return 0;
  }
};

template<typename T>
static int bsearchnum(lua_State* L,int vectorLocation,int sizeTable,int size,bool isTable,T key, View* view) {
  int span = view->Get<int>(vectorLocation - 4);//lvector length
  int start = 0;
  View viewObj;
  while (span != 0) {
    int middle = span / 2;
    int index = start + middle;
    int tableOffset = vectorLocation + sizeTable * (index);
    if(isTable)
    {
        tableOffset = view->Indirect(tableOffset);
    }
    viewObj.updateBinaryArray(view->binaryArray,tableOffset);
    uint32_t offset = viewObj.Offset(size);
    int comp = CompareToHelper<T>::CompareTo(viewObj.Get<T>(offset+tableOffset),key);
    if (comp > 0) {
      span = middle;
    } else if (comp < 0) {
      middle++;
      start += middle;
      span -= middle;
    } else {
        lua_pushinteger(L,tableOffset);
        lua_pushinteger(L,index);
        return 2;
    }
  }
    lua_pushinteger(L,-1);
    return 1;
}

static int bsearchstring(lua_State* L,int vectorLocation,int sizeTable,int size,bool isTable,const char* key, View* view) {
  int span = view->Get<int>(vectorLocation - 4);//lvector length
  int start = 0;
  View viewObj;
  while (span != 0) {
    int middle = span / 2;
    int index = start + middle;
    int tableOffset = vectorLocation + sizeTable * (index);
    if(isTable)
    {
        tableOffset = view->Indirect(tableOffset);
    }
    viewObj.updateBinaryArray(view->binaryArray,tableOffset);
    uint32_t offset = viewObj.Offset(size);
    int comp = strcmp(viewObj.String(offset+tableOffset).string,key);
    if (comp > 0) {
      span = middle;
    } else if (comp < 0) {
      middle++;
      start += middle;
      span -= middle;
    } else {
        lua_pushinteger(L,tableOffset);
        lua_pushinteger(L,index);
        return 2;
    }
  }
  lua_pushinteger(L,-1);
  return 1;
}

enum ENumType {
    ENumType_NORMAL = 0,
    ENumType_Bool,
    ENumType_Uint8,
    ENumType_Uint16,
    ENumType_Uint32,
    ENumType_Uint64,
    ENumType_Int8,
    ENumType_Int16,
    ENumType_Int32,
    ENumType_Int64,
    ENumType_Float32,
    ENumType_Float64,
    ENumType_MAX,
};

class NumType {
public:
    NumType(ENumType eType,const char* name, int bytewidth, const char* packFmt)
    : type(eType)
    , name(name)
    , bytewidth(bytewidth)
    , packFmt(packFmt) {}
    ENumType type;
    const char* name;
    int bytewidth;
    const char* packFmt;
};



BinaryArray* check_binaryarray(lua_State* L, int n) {
    return (*((BinaryArrayRef**)luaL_checkudata(L, n, "ba_mt")))->ptr;
}

BinaryArrayRef* check_binaryarrayref(lua_State* L, int n) {
    return *((BinaryArrayRef**)luaL_checkudata(L, n, "ba_mt"));
}

View* check_view(lua_State* L, int n) {
    return *(View**)luaL_checkudata(L, n, "view_mt");
}

NumType* check_num_type(lua_State* L, int n, const char* name) {
    return *(NumType**)luaL_checkudata(L, n, name);
}

NumType* test_num_type(lua_State* L, int n, const char* name) {
    void* userdata = luaL_testudata(L, n, name);
    if (userdata != nullptr) return *(NumType**)userdata;
    return nullptr;
}

NumType* get_num_type(lua_State* L, int n) {
    void* userdata = lua_touserdata(L, n);
    if (userdata != nullptr) return *(NumType**)userdata;
    return nullptr;
}

static int num_type_unpack(lua_State* L) {
    BinaryArray* ba = check_binaryarray(L, 2);
    uint32_t offset = (uint32_t)luaL_checkinteger(L, 3);
    void* ptr = (void*)ba->Data(offset);

    NumType* num_type = get_num_type(L,1);
    if(num_type)
    {
        switch(num_type->type)
        {
            case ENumType_Bool:
                lua_pushinteger(L, *((uint8_t*)ptr));
                return 1;
            case ENumType_Uint8:
                lua_pushinteger(L, *((uint8_t*)ptr));
                return 1;
            case ENumType_Uint16:
                lua_pushinteger(L, *((uint16_t*)ptr));
                return 1;
            case ENumType_Uint32:
                lua_pushinteger(L, *((uint32_t*)ptr));
                return 1;
            case ENumType_Uint64:
                lua_pushinteger(L, *((uint64_t*)ptr));
                return 1;
            case ENumType_Int8:
                lua_pushinteger(L, *((int8_t*)ptr));
                return 1;
            case ENumType_Int16:
                lua_pushinteger(L, *((int16_t*)ptr));
                return 1;
            case ENumType_Int32:
                lua_pushinteger(L, *((int32_t*)ptr));
                return 1;
            case ENumType_Int64:
                lua_pushinteger(L, *((int64_t*)ptr));
                return 1;
            case ENumType_Float32:
                lua_pushinteger(L, *((float*)ptr));
                return 1;
            case ENumType_Float64:
                lua_pushinteger(L, *((double*)ptr));
                return 1;
            default:
                lua_pushliteral(L, "incorrect argument type");
                lua_error(L);
                return 0;
        }
    }
    lua_pushliteral(L, "incorrect argument");
    lua_error(L);
    return 0;
}

static int num_type_gc(lua_State* L) {
    NumType* num_type = nullptr;
    if ((num_type = test_num_type(L, 1, "Bool")) ||
        (num_type = test_num_type(L, 1, "Uint8")) ||
        (num_type = test_num_type(L, 1, "Uint16")) ||
        (num_type = test_num_type(L, 1, "Uint32")) ||
        (num_type = test_num_type(L, 1, "Uint64")) ||
        (num_type = test_num_type(L, 1, "Float32")) ||
        (num_type = test_num_type(L, 1, "Float64")) ||
        (num_type = test_num_type(L, 1, "Int8")) ||
        (num_type = test_num_type(L, 1, "Int16")) ||
        (num_type = test_num_type(L, 1, "Int32")) ||
        (num_type = test_num_type(L, 1, "Int64"))) {
        delete num_type;
    }
    return 0;
}

static void register_num_type(lua_State* L, const char* name, NumType* num_type) {
    luaL_Reg num_type_reg[] = {
        { "Unpack", num_type_unpack },
        { "__gc", num_type_gc },
        { nullptr, nullptr }
    };

    luaL_newmetatable(L, name); // [mt]
    luaL_setfuncs(L, num_type_reg, 0); // [mt]

    lua_pushinteger(L, num_type->bytewidth); // [mt, bytewidth]
    lua_setfield(L, -2, "bytewidth"); // [mt]

    lua_pushstring(L, num_type->packFmt); // [mt, packFmt]
    lua_setfield(L, -2, "packFmt"); // [mt]

    lua_setfield(L, -1, "__index"); // []
}

static void num_type_new(lua_State* L,ENumType eType, const char* name, int bytewidth, const char* packFmt) {
    // assume stack top is a table of num_type [num_type_table]
    NumType** udata = (NumType**)lua_newuserdata(L, sizeof(NumType*));
    *udata = new NumType(eType,name, bytewidth, packFmt);
    register_num_type(L, name, *udata);
    luaL_getmetatable(L, name);
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, name); // [num_type_table]
}

static void new_num_types_table(lua_State* L) {
    lua_newtable(L);

    num_type_new(L,ENumType_Bool,  "Bool", 1, "<b");
    num_type_new(L,ENumType_Uint8,  "Uint8", 1, "<I1");
    num_type_new(L,ENumType_Uint16,  "Uint16", 2, "<I2");
    num_type_new(L,ENumType_Uint32,  "Uint32", 4, "<I4");
    num_type_new(L,ENumType_Uint64,  "Uint64", 8, "<I8");
    num_type_new(L,ENumType_Int8,  "Int8", 1, "<i1");
    num_type_new(L,ENumType_Int16,  "Int16", 2, "<i2");
    num_type_new(L,ENumType_Int32,  "Int32", 4, "<i4");
    num_type_new(L,ENumType_Int64,  "Int64", 8, "<i8");
    num_type_new(L,ENumType_Float32,  "Float32", 4, "<f");
    num_type_new(L,ENumType_Float64,  "Float64", 8, "<d");

    lua_pushliteral(L, "Uint32");
    lua_gettable(L, -2);
    lua_setfield(L, -2, "UOffsetT");

    lua_pushliteral(L, "Uint16");
    lua_gettable(L, -2);
    lua_setfield(L, -2, "VOffsetT");

    lua_pushliteral(L, "Int32");
    lua_gettable(L, -2);
    lua_setfield(L, -2, "SOffsetT");
}

static int baref_new(lua_State* L,BinaryArray* ptr)
{
    BinaryArrayRef** udata = (BinaryArrayRef**)lua_newuserdata(L, sizeof(BinaryArrayRef*));
    *udata = new BinaryArrayRef(ptr);
    luaL_getmetatable(L, "ba_mt");
    lua_setmetatable(L, -2);
    return 1;
}

static int baref_newref(lua_State* L,BinaryArrayRef* ref)
{
    BinaryArrayRef** udata = (BinaryArrayRef**)lua_newuserdata(L, sizeof(BinaryArrayRef*));
    *udata = ref;
    BA_SAFE_RETAIN(ref);
    luaL_getmetatable(L, "ba_mt");
    lua_setmetatable(L, -2);
    return 1;
}

static int ba_new(lua_State* L) {
    if (lua_isstring(L, 1)) {
        SizedString str;
        str.string = lua_tolstring(L, 1, &str.size);
        
        BinaryArray* ptr = new BinaryArray(str);
        return baref_new(L, ptr);
    }
    else if (lua_isnumber(L, 1)) {
        uint32_t size = (uint32_t)lua_tointeger(L, 1);

        BinaryArray* ptr = new BinaryArray(size);
        return baref_new(L, ptr);
    }
    lua_pushliteral(L, "incorrect argument");
    lua_error(L);
    return 0;
}

static int ba_slice(lua_State* L) {
    BinaryArray* ba = check_binaryarray(L, 1);
    uint32_t startPos = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t endPos = (uint32_t)luaL_checkinteger(L, 3);
    SizedString ret = ba->Slice(startPos, endPos);
    lua_pushlstring(L, ret.string, ret.size);
    return 1;
}


static int ba_size(lua_State* L) {
    BinaryArray* ba = check_binaryarray(L, 1);
    uint32_t size = ba->Size();
    lua_pushinteger(L, size);
    return 1;
}

static int ba_gc(lua_State* L) {
    BinaryArrayRef* ba_ref = check_binaryarrayref(L, 1);
    BA_SAFE_RELEASE_NULL(ba_ref);
    return 0;
}

static void register_binaryarray(lua_State* L) {
    luaL_Reg binaryarray_reg[] = {
        { "Slice", ba_slice },
        { "__len", ba_size },
        { "__gc", ba_gc },
        { nullptr, nullptr }
    };

    luaL_newmetatable(L, "ba_mt");
    luaL_setfuncs(L, binaryarray_reg, 0);
    lua_setfield(L, -1, "__index");
}

static int view_new(lua_State* L) {
    BinaryArrayRef* baref = check_binaryarrayref(L, 1);
    uint32_t position = (uint32_t)luaL_checkinteger(L, 2);
    View** udata = (View**)lua_newuserdata(L, sizeof(View*));
    *udata = new View(baref, position);
    luaL_getmetatable(L, "view_mt");
    lua_setmetatable(L, -2);
    return 1;
}

static int view_new_empty(lua_State* L) {
    View** udata = (View**)lua_newuserdata(L, sizeof(View*));
    *udata = new View();
    luaL_getmetatable(L, "view_mt");
    lua_setmetatable(L, -2);
    return 1;
}

static int view_offset(lua_State* L) {
    View* view = check_view(L, 1);
    uint32_t vtableOffset = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t ret = view->Offset(vtableOffset);
    lua_pushinteger(L, ret);
    return 1;
}

static int view_indirect(lua_State* L) {
    View* view = check_view(L, 1);
    uint32_t offset = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t ret = view->Indirect(offset);
    lua_pushinteger(L, ret);
    return 1;
}

static int view_string(lua_State* L) {
    View* view = check_view(L, 1);
    uint32_t offset = (uint32_t)luaL_checkinteger(L, 2);
    SizedString ret = view->String(offset);
    lua_pushlstring(L, ret.string, ret.size);
    return 1;
}

static int view_vector_len(lua_State* L) {
    View* view = check_view(L, 1);
    uint32_t offset = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t ret = view->VectorLen(offset);
    lua_pushinteger(L, ret);
    return 1;
}

static int view_vector(lua_State* L) {
    View* view = check_view(L, 1);
    uint32_t offset = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t ret = view->Vector(offset);
    lua_pushinteger(L, ret);
    return 1;
}

static int view_union(lua_State* L) {
    View* view = check_view(L, 1);
    View* view2 = check_view(L, 2);
    uint32_t offset = (uint32_t)luaL_checkinteger(L, 3);
    view->Union(view2, offset);
    return 0;
}

static int view_get(lua_State* L) {
    View* view = check_view(L, 1);
    uint32_t offset = (uint32_t)luaL_checkinteger(L, 3);

    NumType* num_type = get_num_type(L,2);
    if(num_type)
    {
        switch(num_type->type)
        {
            case ENumType_Bool:
                lua_pushinteger(L, view->Get<uint8_t>(offset));
                return 1;
            case ENumType_Uint8:
                lua_pushinteger(L, view->Get<uint8_t>(offset));
                return 1;
            case ENumType_Uint16:
                lua_pushinteger(L, view->Get<uint16_t>(offset));
                return 1;
            case ENumType_Uint32:
                lua_pushinteger(L, view->Get<uint32_t>(offset));
                return 1;
            case ENumType_Uint64:
                lua_pushinteger(L, view->Get<uint64_t>(offset));
                return 1;
            case ENumType_Int8:
                lua_pushinteger(L, view->Get<int8_t>(offset));
                return 1;
            case ENumType_Int16:
                lua_pushinteger(L, view->Get<int16_t>(offset));
                return 1;
            case ENumType_Int32:
                lua_pushinteger(L, view->Get<int32_t>(offset));
                return 1;
            case ENumType_Int64:
                lua_pushinteger(L, view->Get<int64_t>(offset));
                return 1;
            case ENumType_Float32:
                lua_pushinteger(L, view->Get<float>(offset));
                return 1;
            case ENumType_Float64:
                lua_pushinteger(L, view->Get<double>(offset));
                return 1;
            default:
                lua_pushliteral(L, "incorrect argument type");
                lua_error(L);
                return 0;
        }
    }

    lua_pushliteral(L, "incorrect argument");
    lua_error(L);
    return 0;
}

static int view_search_num(lua_State* L) {
    View* view = check_view(L, 1);
    int vectorLocation = (int)luaL_checkinteger(L, 2);
    int sizeTable = (int)luaL_checkinteger(L, 3);
    int size = (int)luaL_checkinteger(L, 4);
    bool isTable = lua_toboolean(L,5);
    lua_Integer value = luaL_checkinteger(L, 7);
    
    NumType* num_type = get_num_type(L,6);
    if(num_type)
    {
        switch(num_type->type)
        {
            case ENumType_Bool:
                return bsearchnum<uint8_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Uint8:
                return bsearchnum<uint8_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Uint16:
                return bsearchnum<uint16_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Uint32:
                return bsearchnum<uint32_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Uint64:
                return bsearchnum<uint64_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Int8:
                return bsearchnum<int8_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Int16:
                return bsearchnum<int16_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Int32:
                return bsearchnum<int32_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Int64:
                return bsearchnum<int64_t>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Float32:
                return bsearchnum<float>(L,vectorLocation,sizeTable,size,isTable,value,view);
            case ENumType_Float64:
                return bsearchnum<double>(L,vectorLocation,sizeTable,size,isTable,value,view);
            default:
                lua_pushliteral(L, "incorrect argument type");
                lua_error(L);
                return 0;
        }
    }

    lua_pushliteral(L, "incorrect argument");
    lua_error(L);
    return 0;
}

static int view_search_string(lua_State* L) {
    View* view = check_view(L, 1);

    int vectorLocation = (int)luaL_checkinteger(L, 2);
    int sizeTable = (int)luaL_checkinteger(L, 3);
    int size = (int)luaL_checkinteger(L, 4);
    bool isTable = lua_toboolean(L,5);
    const char* key = luaL_checkstring(L, 6);
    
    return bsearchstring(L,vectorLocation,sizeTable,size,isTable,key,view);
}

static int view_index(lua_State* L) {
    View* view = check_view(L, 1);
    const char* key = luaL_checkstring(L, 2);
    if (strcmp(key, "pos") == 0) {
        lua_pushinteger(L, view->position);
        return 1;
    } else if (strcmp(key, "bytes") == 0) {
        return baref_newref(L,view->binaryArrayRef);
    } else {
        luaL_getmetatable(L, "view_mt_mt");
        lua_getfield(L, -1, key);
        lua_replace(L, -2);
        return 1;
    }
}

static int view_gc(lua_State* L) {
    View* view = check_view(L, 1);
    delete view;
    return 0;
}

static void register_view(lua_State* L) {
    luaL_Reg view_mt_mt_reg[] = {
        { "Offset", view_offset },
        { "Indirect", view_indirect },
        { "String", view_string },
        { "VectorLen", view_vector_len },
        { "Vector", view_vector },
        { "Union", view_union },
        { "Get", view_get },
        { "LookUpNum", view_search_num },
        { "LookUpString", view_search_string },
        { nullptr, nullptr }
    };

    luaL_Reg view_mt_reg[] = {
        { "Offset", view_offset },
        { "Indirect", view_indirect },
        { "String", view_string },
        { "VectorLen", view_vector_len },
        { "Vector", view_vector },
        { "Union", view_union },
        { "Get", view_get },
        { "LookUpNum", view_search_num },
        { "LookUpString", view_search_string },
        { "__index", view_index },
        { "__gc", view_gc },
        { nullptr, nullptr }
    };

    luaL_newmetatable(L, "view_mt_mt");
    luaL_setfuncs(L, view_mt_mt_reg, 0);
    lua_pop(L, 1);

    luaL_newmetatable(L, "view_mt");
    luaL_setfuncs(L, view_mt_reg, 0);
    lua_pop(L, 1);
}

extern "C" {

LUALIB_API int luaopen_flatbuffersnative(lua_State* L)
{
    register_binaryarray(L);
    register_view(L);

	lua_newtable(L); // [flatbuffers]

	lua_pushliteral(L, "flatbuffers"); // [flatbuffers, name]
	lua_setfield(L, -2, "_NAME"); // [flatbuffers]

	lua_pushliteral(L, "v0.1"); // [flatbuffers, version]
	lua_setfield(L, -2, "_VERSION"); // [flatbuffers]

	lua_pushcfunction(L, ba_new); // [flatbuffers, new_binaryarray]
	lua_setfield(L, -2, "new_binaryarray"); // [flatbuffers]

	new_num_types_table(L); // [flatbuffers, num_types]
	lua_setfield(L, -2, "N"); // [flatbuffers]

	lua_pushcfunction(L, view_new); // [flatbuffers, new_view]
	lua_setfield(L, -2, "new_view"); // [flatbuffers]

    lua_pushcfunction(L, view_new_empty); // [flatbuffers, view_new_empty]
    lua_setfield(L, -2, "new_view_empty"); // [flatbuffers]
	return 1;
}

}
