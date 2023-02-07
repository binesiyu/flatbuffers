/*
 * Copyright 2014 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// independent from idl_parser, since this code is not needed for most clients

#include <string>
#include <unordered_set>
#include <algorithm>

#include "flatbuffers/code_generators.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

namespace flatbuffers {
namespace lua {

// Hardcode spaces per indentation.
const CommentConfig def_comment = { nullptr, "--", nullptr };
const char *Indent = "    ";
const char *Comment = "-- ";
const char *End = "end\n";
const char *EndFunc = "end\n";
const char *SelfData = "self.__view";
const char *SelfDataPos = "self.__view.pos";
const char *SelfDataBytes = "self.__view.bytes";

std::string MakeCamel2(const std::string &in, bool first = true);

std::string MakeCamel2(const std::string &in, bool first) { 
  (void)first;
  return in;
}

static void toCamelCase(std::string & str)
{
  int len = str.length();
  for (int i = 0; i<len; i++)
  {
    if (str[i] >= 'A' && str[i] <= 'Z' && (i == 0 || i == len - 1 || (str[i+1] >= 'A' && str[i+1] <= 'Z')))
        str[i] = str[i] + 32;
      else
          break;
  }
  //return str;
}

/*
static void toCamelCase(std::string & s)
{
    char previous = 'a';
    auto f = [&](char current){
        char result = (std::isblank(previous) && std::isalpha(current)) ? std::toupper(current) : std::tolower(current);
        previous = current;
        return result;
    };
    std::transform(s.begin(),s.end(),s.begin(),f);
}
*/
bool isStructsRoot(const StructDef &struct_def)
{
    return struct_def.isroot;
    //return hasEnding(name,Root);
}

StructDef* getStructRoot(const StructDef &struct_def) {
  // Generate the Init method that sets the field in a pre-existing
  // accessor object. This is to allow object reuse.
  //InitializeExisting(struct_def, code_ptr);
  for (auto it = struct_def.fields.vec.begin();
       it != struct_def.fields.vec.end(); ++it) {
    auto &field = **it;
    if (field.deprecated) continue;

    
    if (field.name == "data_list")
    {
        if (field.value.type.base_type == BASE_TYPE_VECTOR)
        {
            return field.value.type.VectorType().struct_def;
        }
        else if(field.value.type.base_type == BASE_TYPE_STRUCT)
        {
            return field.value.type.struct_def;
        }
    }
  }
    
    return nullptr;
}


class LuaGenerator : public BaseGenerator {
 public:
  LuaGenerator(const Parser &parser, const std::string &path,
               const std::string &file_name)
      : BaseGenerator(parser, path, file_name, "" /* not used */,
                      "" /* not used */, "lua") {
    static const char *const keywords[] = {
      "and",      "break",  "do",   "else", "elseif", "end",  "false", "for",
      "function", "goto",   "if",   "in",   "local",  "nil",  "not",   "or",
      "repeat",   "return", "then", "true", "until",  "while"
    };
    keywords_.insert(std::begin(keywords), std::end(keywords));
  }

  // Most field accessors need to retrieve and test the field offset first,
  // this is the prefix code for that.
  std::string OffsetPrefix(const FieldDef &field) {
    return std::string(Indent) + "local o = " + SelfData + ":Offset(" +
           NumToString(field.value.offset) + ")\n" + Indent +
           "if o ~= 0 then\n";
  }

  // Begin a class declaration.
  void BeginClass(const StructDef &struct_def, std::string *code_ptr) {
    std::string &code = *code_ptr;
    std::string name = NormalizedName(struct_def);
    toCamelCase(name);
    // std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    code += "local " + NormalizedName(struct_def) + " = F.NewCfg('" + name + "')\n";
    //code += "local " + NormalizedMetaName(struct_def) +
    //        " = {} -- the class metatable\n";
    code += "\n";
  }

  // Begin enum code with a class declaration.
  void BeginEnum(const std::string &class_name, std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += "local " + class_name + " = {\n";
  }

  std::string EscapeKeyword(const std::string &name) const {
    return keywords_.find(name) == keywords_.end() ? name : "_" + name;
  }

  std::string NormalizedName(const Definition &definition) const {
    return EscapeKeyword(definition.name);
  }

  std::string NormalizedName(const EnumVal &ev) const {
    return EscapeKeyword(ev.name);
  }

  std::string NormalizedMetaName(const Definition &definition) const {
      return EscapeKeyword(definition.name);// + "_mt";
  }

  // A single enum member.
  void EnumMember(const EnumDef &enum_def, const EnumVal &ev,
                  std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += std::string(Indent) + NormalizedName(ev) + " = " +
            enum_def.ToString(ev) + ",\n";
  }

  // End enum code.
  void EndEnum(std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += "}\n";
  }

  void GenerateNewObjectPrototype(const StructDef &struct_def,
                                  std::string *code_ptr) {
    std::string &code = *code_ptr;

    code += NormalizedName(struct_def) + ".__New = F.New("  + NormalizedName(struct_def) + ")\n\n";
  }


  // Initialize an existing object with other data, to avoid an allocation.
  void InitializeExisting(const StructDef &struct_def, std::string *code_ptr) {
    std::string &code = *code_ptr;

    //GenReceiver(struct_def, code_ptr);
    code += "function " + NormalizedName(struct_def) + ":";
    code += "Init(buf, pos)\n";
    code +=
        std::string(Indent) + SelfData + " = fb.view.New(buf, pos)\n";
    code += EndFunc;
  }

  // Get the length of a vector.
  void GetVectorLen(const StructDef &struct_def, const FieldDef &field,
                    std::string *code_ptr) {
    std::string &code = *code_ptr;

    GenReceiver(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field)) + "Length()\n";
    code += OffsetPrefix(field);
    code +=
        std::string(Indent) + Indent + "return " + SelfData + ":VectorLen(o)\n";
    code += std::string(Indent) + End;
    code += std::string(Indent) + "return 0\n";
    code += EndFunc;
  }

  // Get the value of a struct's scalar.
  void GetScalarFieldOfStruct(const StructDef &struct_def,
                              const FieldDef &field, std::string *code_ptr) {
    std::string &code = *code_ptr;
    std::string getter = GenGetter(field.value.type);
    GenReceiverEx(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field));
    code += " = F.FunFieldStruct(";
    code += NumToString(field.value.offset);
    code += ",N.";
    code += MakeCamel(GenTypeGet(field.value.type));
    code += ")\n\n";
  }

  // Get the value of a table's scalar.
  void GetScalarFieldOfTable(const StructDef &struct_def, const FieldDef &field,
                             std::string *code_ptr) {
    std::string &code = *code_ptr;
    GenReceiverEx(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field));

    auto is_bool = field.value.type.base_type == BASE_TYPE_BOOL;
    if (is_bool) {
        code += " = F.FunFieldBool(";
    }
    else {
        code += " = F.FunField(";
    }

    code += NumToString(field.value.offset);
    code += ",N.";
    code += MakeCamel(GenTypeGet(field.value.type));

    std::string default_value;
    if (is_bool) {
      default_value = field.value.constant == "0" ? "false" : "true";
    } else {
      default_value = field.value.constant;
    }
    code += "," + default_value + ")\n\n";
  }

  // Get a struct by initializing an existing struct.
  // Specific to Struct.
  void GetStructFieldOfStruct(const StructDef &struct_def,
                              const FieldDef &field, std::string *code_ptr) {
    std::string &code = *code_ptr;
    GenReceiver(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field));
    code += "()\n";
    code += "local _temp = function(obj)\n";
    code += std::string(Indent) + "obj(" + SelfDataBytes + ", " +
            SelfDataPos + " + ";
    code += NumToString(field.value.offset) + ")\n";
    code += std::string(Indent) + "return obj\n";
    code += EndFunc;

    code += "return _temp\n";
    code += EndFunc;
  }

  // Get a struct by initializing an existing struct.
  // Specific to Table.
  void GetStructFieldOfTable(const StructDef &struct_def, const FieldDef &field,
                             std::string *code_ptr,const StructDef* strufct_value) {
    std::string &code = *code_ptr;
    GenReceiverEx(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field));
    if(struct_def.isroot)
    {
      code += " = F.FunSubCfg(";
    }
    else
    {
      code += " = F.FunSub(";
    }
    code += NumToString(field.value.offset);
    if(struct_def.isroot && nullptr != strufct_value)
    {
      code += ",";
      code += NormalizedName(*strufct_value);
      code += ",";
    }
    else
    {
      code += ",'";
      code += TypeNameWithNamespace(field);
      code += "',";
    }

    if (field.value.type.struct_def->fixed) {
        code += "false";
    } else {
        code += "true";
    }
    code += ")\n\n";
  }

  // Get the value of a string.
  void GetStringField(const StructDef &struct_def, const FieldDef &field,
                      std::string *code_ptr) {
    std::string &code = *code_ptr;
    GenReceiverEx(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field));

    code += " = F.FunFieldString(";
    code += NumToString(field.value.offset);
    code += ")\n\n";
  }

  // Get the value of a union from an object.
  void GetUnionField(const StructDef &struct_def, const FieldDef &field,
                     std::string *code_ptr) {
    std::string &code = *code_ptr;
    GenReceiverEx(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field));

    code += " = F.FunUnion(";
    code += NumToString(field.value.offset);
    code += ")\n\n";
  }

  // Get the value of a vector's struct member.
  void GetMemberOfVectorOfStruct(const StructDef &struct_def,
                                 const FieldDef &field, std::string *code_ptr,const StructDef* strufct_value) {
    std::string &code = *code_ptr;
    auto vectortype = field.value.type.VectorType();

    std::string arr_key = "_fb_" + MakeCamel2(NormalizedName(field)) + "_arr";
    GenReceiverEx(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field));
    if(struct_def.isroot)
    {
        code += " = F.FunArrayCfg(";
    }
    else
    {
        code += " = F.FunArraySub(";
    }
    
    code += NumToString(field.value.offset);
   
    if(struct_def.isroot && nullptr != strufct_value)
    {
        code += ",";
        code += NormalizedName(*strufct_value);
        code += ",";
    }
    else
    {
        code += ",'";
        code += TypeNameWithNamespace(field);
        code += "',";
    }
    code += NumToString(InlineSize(vectortype));
    code += ",'";
    code += arr_key;
    code += "',";
    if (!(vectortype.struct_def->fixed)) {
      code += "true";
    } else {
      code += "false";
    }
    if (vectortype.struct_def->has_key) {
        flatbuffers::FieldDef *key_field = nullptr;
        for (auto it = vectortype.struct_def->fields.vec.begin();
             it != vectortype.struct_def->fields.vec.end(); ++it) {
          auto &fieldDef = **it;
          if (fieldDef.deprecated) continue;
          if (fieldDef.key)
          {
            key_field = &fieldDef;
              break;
          }
        }

        if(key_field)
        {
          code += ",'";
          code += MakeCamel2(NormalizedName(*key_field));
          code += "',";
          code += NumToString(key_field->value.offset);
          code += ",";
          if (!IsString(key_field->value.type)) {
            code += "true";
            code += ",N.";
            code += MakeCamel(GenTypeGet(key_field->value.type));
          } else {
            code += "false";
          }
        }
    }
    code += ")\n\n";

  }

  // Get the value of a vector's non-struct member. Uses a named return
  // argument to conveniently set the zero value for the result.
  void GetMemberOfVectorOfNonStruct(const StructDef &struct_def,
                                    const FieldDef &field,
                                    std::string *code_ptr) {
    std::string &code = *code_ptr;
    auto vectortype = field.value.type.VectorType();

    std::string arr_key = "_fb_" + MakeCamel2(NormalizedName(field)) + "_arr";
    GenReceiverEx(struct_def, code_ptr);
    code += MakeCamel2(NormalizedName(field));
    code += " = F.FunArray(";
    code += NumToString(field.value.offset);
    code += ",N.";
    code += MakeCamel(GenTypeGet(field.value.type));
    code += ",";
    code += NumToString(InlineSize(vectortype));
    code += ",'";
    code += arr_key;
    code += "',";
    if (IsString(vectortype)) {
      code += "''";
    } else {
      code += "0";
    }
    code += ")\n\n";

  }


  // Begin the creator function signature.
  void BeginBuilderArgs(const StructDef &struct_def, std::string *code_ptr) {
    std::string &code = *code_ptr;

    code += "function " + NormalizedName(struct_def) + ".Create" +
            NormalizedName(struct_def);
    code += "(builder";
  }

  // Recursively generate arguments for a constructor, to deal with nested
  // structs.
  void StructBuilderArgs(const StructDef &struct_def, const char *nameprefix,
                         std::string *code_ptr) {
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      auto &field = **it;
      if (IsStruct(field.value.type)) {
        // Generate arguments for a struct inside a struct. To ensure names
        // don't clash, and to make it obvious these arguments are constructing
        // a nested struct, prefix the name with the field name.
        StructBuilderArgs(*field.value.type.struct_def,
                          (nameprefix + (NormalizedName(field) + "_")).c_str(),
                          code_ptr);
      } else {
        std::string &code = *code_ptr;
        code += std::string(", ") + nameprefix;
        code += MakeCamel(NormalizedName(field), false);
      }
    }
  }

  // End the creator function signature.
  void EndBuilderArgs(std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += ")\n";
  }

  // Recursively generate struct construction statements and instert manual
  // padding.
  void StructBuilderBody(const StructDef &struct_def, const char *nameprefix,
                         std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += std::string(Indent) + "builder:Prep(" +
            NumToString(struct_def.minalign) + ", ";
    code += NumToString(struct_def.bytesize) + ")\n";
    for (auto it = struct_def.fields.vec.rbegin();
         it != struct_def.fields.vec.rend(); ++it) {
      auto &field = **it;
      if (field.padding)
        code += std::string(Indent) + "builder:Pad(" +
                NumToString(field.padding) + ")\n";
      if (IsStruct(field.value.type)) {
        StructBuilderBody(*field.value.type.struct_def,
                          (nameprefix + (NormalizedName(field) + "_")).c_str(),
                          code_ptr);
      } else {
        code +=
            std::string(Indent) + "builder:Prepend" + GenMethod(field) + "(";
        code += nameprefix + MakeCamel(NormalizedName(field), false) + ")\n";
      }
    }
  }

  void EndBuilderBody(std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += std::string(Indent) + "return builder:Offset()\n";
    code += EndFunc;
  }

  // Get the value of a table's starting offset.
  void GetStartOfTable(const StructDef &struct_def, std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += "function " + NormalizedName(struct_def) + ".Start";
    code += "(builder) ";
    code += "builder:StartObject(";
    code += NumToString(struct_def.fields.vec.size());
    code += ") end\n";
  }

  // Set the value of a table's field.
  void BuildFieldOfTable(const StructDef &struct_def, const FieldDef &field,
                         const size_t offset, std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += "function " + NormalizedName(struct_def) + ".Add" +
            MakeCamel(NormalizedName(field));
    code += "(builder, ";
    code += MakeCamel(NormalizedName(field), false);
    code += ") ";
    code += "builder:Prepend";
    code += GenMethod(field) + "Slot(";
    code += NumToString(offset) + ", ";
    // todo: i don't need to cast in Lua, but am I missing something?
    //    if (!IsScalar(field.value.type.base_type) && (!struct_def.fixed)) {
    //      code += "N.UOffsetTFlags.py_type";
    //      code += "(";
    //      code += MakeCamel(NormalizedName(field), false) + ")";
    //    } else {
    code += MakeCamel(NormalizedName(field), false);
    //    }
    code += ", " + field.value.constant;
    code += ") end\n";
  }

  // Set the value of one of the members of a table's vector.
  void BuildVectorOfTable(const StructDef &struct_def, const FieldDef &field,
                          std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += "function " + NormalizedName(struct_def) + ".Start";
    code += MakeCamel(NormalizedName(field));
    code += "Vector(builder, numElems) return builder:StartVector(";
    auto vector_type = field.value.type.VectorType();
    auto alignment = InlineAlignment(vector_type);
    auto elem_size = InlineSize(vector_type);
    code += NumToString(elem_size);
    code += ", numElems, " + NumToString(alignment);
    code += ") end\n";
  }

  // Get the offset of the end of a table.
  void GetEndOffsetOnTable(const StructDef &struct_def, std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += "function " + NormalizedName(struct_def) + ".End";
    code += "(builder) ";
    code += "return builder:EndObject() end\n";
  }

  // Generate the receiver for function signatures.
  void GenReceiver(const StructDef &struct_def, std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += "function " + NormalizedMetaName(struct_def) + ":";
  }

  void GenReceiverEx(const StructDef &struct_def, std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += NormalizedMetaName(struct_def) + ".";
  }

  // Generate a struct field, conditioned on its child type(s).
  void GenStructAccessor(const StructDef &struct_def, const FieldDef &field,
                         std::string *code_ptr,const StructDef* strufct_value = nullptr) {
    GenComment(field.doc_comment, code_ptr, &def_comment);
    if (IsScalar(field.value.type.base_type)) {
      if (struct_def.fixed) {
        GetScalarFieldOfStruct(struct_def, field, code_ptr);
      } else {
        GetScalarFieldOfTable(struct_def, field, code_ptr);
      }
    } else {
      switch (field.value.type.base_type) {
        case BASE_TYPE_STRUCT:
          if (struct_def.fixed) {
            GetStructFieldOfStruct(struct_def, field, code_ptr);
          } else {
            GetStructFieldOfTable(struct_def, field, code_ptr,strufct_value);
          }
          break;
        case BASE_TYPE_STRING:
          GetStringField(struct_def, field, code_ptr);
          break;
        case BASE_TYPE_VECTOR: {
          auto vectortype = field.value.type.VectorType();
          if (vectortype.base_type == BASE_TYPE_STRUCT) {
            GetMemberOfVectorOfStruct(struct_def, field, code_ptr,strufct_value);
          } else {
            GetMemberOfVectorOfNonStruct(struct_def, field, code_ptr);
          }
          break;
        }
        case BASE_TYPE_UNION: GetUnionField(struct_def, field, code_ptr); break;
        default: FLATBUFFERS_ASSERT(0);
      }
    }
    // if (IsVector(field.value.type)) {
      // GetVectorLen(struct_def, field, code_ptr);
    // }
  }

  // Generate table constructors, conditioned on its members' types.
  void GenTableBuilders(const StructDef &struct_def, std::string *code_ptr) {
    GetStartOfTable(struct_def, code_ptr);

    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      auto &field = **it;
      if (field.deprecated) continue;

      auto offset = it - struct_def.fields.vec.begin();
      BuildFieldOfTable(struct_def, field, offset, code_ptr);
      if (IsVector(field.value.type)) {
        BuildVectorOfTable(struct_def, field, code_ptr);
      }
    }

    GetEndOffsetOnTable(struct_def, code_ptr);
  }

  // Generate struct or table methods.
  void GenStruct(const StructDef &struct_def, std::string *code_ptr) {
    if (struct_def.generated) return;

    GenComment(struct_def.doc_comment, code_ptr, &def_comment);
    BeginClass(struct_def, code_ptr);

    // GenerateNewObjectPrototype(struct_def, code_ptr);

    if (!struct_def.fixed) {
      // Generate a special accessor for the table that has been declared as
      // the root type.
    }

    // Generate the Init method that sets the field in a pre-existing
    // accessor object. This is to allow object reuse.
    //InitializeExisting(struct_def, code_ptr);
    for (auto it = struct_def.fields.vec.begin();
         it != struct_def.fields.vec.end(); ++it) {
      auto &field = **it;
      if (field.deprecated) continue;

      GenStructAccessor(struct_def, field, code_ptr);
    }

      /*
    if (struct_def.fixed) {
      // create a struct constructor function
      GenStructBuilder(struct_def, code_ptr);
    } else {
      // Create a set of functions that allow table construction.
      GenTableBuilders(struct_def, code_ptr);
    }
       */
  }

    // Generate struct or table methods.
    void GenRoot(const StructDef &struct_def, std::string *code_ptr,const StructDef* strufct_value) {
      if (struct_def.generated) return;
      std::string &code = *code_ptr;
      code += std::string(Comment) + "root cfg\n";
      BeginClass(struct_def, code_ptr);
      // Generate the Init method that sets the field in a pre-existing
      // accessor object. This is to allow object reuse.
      //InitializeExisting(struct_def, code_ptr);
      for (auto it = struct_def.fields.vec.begin();
           it != struct_def.fields.vec.end(); ++it) {
        auto &field = **it;
        if (field.deprecated) continue;

        GenStructAccessor(struct_def, field, code_ptr,strufct_value);
      }
    }
    
  // Generate enum declarations.
  void GenEnum(const EnumDef &enum_def, std::string *code_ptr) {
    if (enum_def.generated) return;

    GenComment(enum_def.doc_comment, code_ptr, &def_comment);
    BeginEnum(NormalizedName(enum_def), code_ptr);
    for (auto it = enum_def.Vals().begin(); it != enum_def.Vals().end(); ++it) {
      auto &ev = **it;
      GenComment(ev.doc_comment, code_ptr, &def_comment, Indent);
      EnumMember(enum_def, ev, code_ptr);
    }
    EndEnum(code_ptr);
  }

  // Returns the function name that is able to read a value of the given type.
  std::string GenGetter(const Type &type) {
    switch (type.base_type) {
      case BASE_TYPE_STRING: return std::string(SelfData) + ":String(";
      case BASE_TYPE_UNION: return std::string(SelfData) + ":Union(";
      case BASE_TYPE_VECTOR: return GenGetter(type.VectorType());
      default:
        return std::string(SelfData) + ":Get(N." +
               MakeCamel(GenTypeGet(type)) + ", ";
    }
  }

  // Returns the method name for use with add/put calls.
  std::string GenMethod(const FieldDef &field) {
    return IsScalar(field.value.type.base_type)
               ? MakeCamel(GenTypeBasic(field.value.type))
               : (IsStruct(field.value.type) ? "Struct" : "UOffsetTRelative");
  }

  std::string GenTypeBasic(const Type &type) {
    // clang-format off
    static const char *ctypename[] = {
      #define FLATBUFFERS_TD(ENUM, IDLTYPE, \
              CTYPE, JTYPE, GTYPE, NTYPE, PTYPE, ...) \
        #PTYPE,
        FLATBUFFERS_GEN_TYPES(FLATBUFFERS_TD)
      #undef FLATBUFFERS_TD
    };
    // clang-format on
    return ctypename[type.base_type];
  }

  std::string GenTypePointer(const Type &type) {
    switch (type.base_type) {
      case BASE_TYPE_STRING: return "string";
      case BASE_TYPE_VECTOR: return GenTypeGet(type.VectorType());
      case BASE_TYPE_STRUCT: return type.struct_def->name;
      case BASE_TYPE_UNION:
        // fall through
      default: return "*fb.Table";
    }
  }

  std::string GenTypeGet(const Type &type) {
    return IsScalar(type.base_type) ? GenTypeBasic(type) : GenTypePointer(type);
  }

  std::string GetNamespace(const Type &type) {
    return type.struct_def->defined_namespace->GetFullyQualifiedName(
        type.struct_def->name);
  }

  std::string TypeName(const FieldDef &field) {
    return GenTypeGet(field.value.type);
  }

  std::string TypeNameWithNamespace(const FieldDef &field) {
      std::string name = GetNamespace(field.value.type);
      toCamelCase(name);
      // std::transform(name.begin(), name.end(), name.begin(), ::tolower);
      return name;
  }

  // Create a struct with a builder and the struct's arguments.
  void GenStructBuilder(const StructDef &struct_def, std::string *code_ptr) {
    BeginBuilderArgs(struct_def, code_ptr);
    StructBuilderArgs(struct_def, "", code_ptr);
    EndBuilderArgs(code_ptr);

    StructBuilderBody(struct_def, "", code_ptr);
    EndBuilderBody(code_ptr);
  }

  bool generate() {
    if (!generateEnums()) return false;
    if (!generateStructs()) return false;
    return true;
  }

 private:
  bool generateEnums() {
    for (auto it = parser_.enums_.vec.begin(); it != parser_.enums_.vec.end();
         ++it) {
      auto &enum_def = **it;
      std::string enumcode;
      GenEnum(enum_def, &enumcode);
      if (!SaveType(enum_def, enumcode, false)) return false;
    }
    return true;
  }
    typedef std::map<std::string,StructDef*> MAP_DEF_STRING;
    typedef std::map<StructDef*,StructDef*> MAP_DEF;
  bool generateStructs() {
    MAP_DEF_STRING mapType;
    MAP_DEF mapTypeDef;
    for (auto it = parser_.structs_.vec.begin();
           it != parser_.structs_.vec.end(); ++it) {
        auto &struct_def = **it;
        mapType[struct_def.name] = *it;
    }
      
    for (auto it = parser_.structs_.vec.begin();
             it != parser_.structs_.vec.end(); ++it) {
      auto &struct_def = **it;
      if(isStructsRoot(struct_def))
      {
          auto struct_type = getStructRoot(struct_def);
          if(struct_type != nullptr)
          {
              mapTypeDef[struct_type] = *it;
          }
      }
    }
      
    for (auto it = parser_.structs_.vec.begin();
         it != parser_.structs_.vec.end(); ++it) {
      auto &struct_def = **it;
        if(isStructsRoot(struct_def))
        {
            continue;
        }
        std::string declcode;
        auto itroot = mapTypeDef.find(*it);
        if(itroot != mapTypeDef.end())
        {
            GenStruct(struct_def, &declcode);
            auto struct_def_root = itroot->second;
            GenRoot(*struct_def_root, &declcode,*it);
            if (!SaveType(struct_def, declcode, true,struct_def_root)) return false;
        }
        else
        {
            GenStruct(struct_def, &declcode);
            if (!SaveType(struct_def, declcode, true)) return false;
        }
      
    }
    return true;
  }

  // Begin by declaring namespace and imports.
  void BeginFile(const std::string &name_space_name, const bool needs_imports,
                 std::string *code_ptr) {
    std::string &code = *code_ptr;
    code += std::string(Comment) + FlatBuffersGeneratedWarning() + "\n\n";
    code += std::string(Comment) + "namespace: " + name_space_name + "\n\n";
    if (needs_imports) {
      code += "local fb = require('flatbuffers')\nlocal N = fb.N\nlocal F=fb.F\n\n";
    }
  }

  // Save out the generated code for a Lua Table type.
  bool SaveType(const Definition &def, const std::string &classcode,
                bool needs_imports,const Definition *root = nullptr) {
    if (!classcode.length()) return true;

    std::string namespace_dir = path_;
    auto &namespaces = def.defined_namespace->components;
    for (auto it = namespaces.begin(); it != namespaces.end(); ++it) {
      if (it != namespaces.begin()) namespace_dir += kPathSeparator;
      namespace_dir += *it;
      // std::string init_py_filename = namespace_dir + "/__init__.py";
      // SaveFile(init_py_filename.c_str(), "", false);
    }

    std::string code = "";
    BeginFile(LastNamespacePart(*def.defined_namespace), needs_imports, &code);
    code += classcode;
    code += "\n";
      
    std::string name = NormalizedName(def);
    toCamelCase(name);
    // std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    if(root)
    {
        code +=
            "return F.createCfg('" + name + "'," + NormalizedName(*root) + ") " + Comment + "return the Cfg\n";
    }
    else
    {
    code +=
        "return " + NormalizedName(def) + " " + Comment + "return the module\n";
    }
    std::string filename =
        NamespaceDir(*def.defined_namespace) + name + ".lua";
    return SaveFile(filename.c_str(), code, false);
  }

 private:
  std::unordered_set<std::string> keywords_;
};

}  // namespace lua

bool GenerateLua(const Parser &parser, const std::string &path,
                 const std::string &file_name) {
  lua::LuaGenerator generator(parser, path, file_name);
  return generator.generate();
}

}  // namespace flatbuffers
