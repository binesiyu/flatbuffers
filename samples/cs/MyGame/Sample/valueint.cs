// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

namespace MyGame.Sample
{

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct valueint : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static valueint GetRootAsvalueint(ByteBuffer _bb) { return GetRootAsvalueint(_bb, new valueint()); }
  public static valueint GetRootAsvalueint(ByteBuffer _bb, valueint obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public valueint __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public int Keyname { get { int o = __p.__offset(4); return o != 0 ? __p.bb.GetInt(o + __p.bb_pos) : (int)0; } }
  public string StringValue { get { int o = __p.__offset(6); return o != 0 ? __p.__string(o + __p.bb_pos) : null; } }
#if ENABLE_SPAN_T
  public Span<byte> GetStringValueBytes() { return __p.__vector_as_span<byte>(6, 1); }
#else
  public ArraySegment<byte>? GetStringValueBytes() { return __p.__vector_as_arraysegment(6); }
#endif
  public byte[] GetStringValueArray() { return __p.__vector_as_array<byte>(6); }

  public static Offset<MyGame.Sample.valueint> Createvalueint(FlatBufferBuilder builder,
      int keyname = 0,
      StringOffset stringValueOffset = default(StringOffset)) {
    builder.StartTable(2);
    valueint.AddStringValue(builder, stringValueOffset);
    valueint.AddKeyname(builder, keyname);
    return valueint.Endvalueint(builder);
  }

  public static void Startvalueint(FlatBufferBuilder builder) { builder.StartTable(2); }
  public static void AddKeyname(FlatBufferBuilder builder, int keyname) { builder.AddInt(0, keyname, 0); }
  public static void AddStringValue(FlatBufferBuilder builder, StringOffset stringValueOffset) { builder.AddOffset(1, stringValueOffset.Value, 0); }
  public static Offset<MyGame.Sample.valueint> Endvalueint(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<MyGame.Sample.valueint>(o);
  }

  public static VectorOffset CreateSortedVectorOfvalueint(FlatBufferBuilder builder, Offset<valueint>[] offsets) {
    Array.Sort(offsets, (Offset<valueint> o1, Offset<valueint> o2) => builder.DataBuffer.GetInt(Table.__offset(4, o1.Value, builder.DataBuffer)).CompareTo(builder.DataBuffer.GetInt(Table.__offset(4, o2.Value, builder.DataBuffer))));
    return builder.CreateVectorOfTables(offsets);
  }

  public static valueint? __lookup_by_key(int vectorLocation, int key, ByteBuffer bb) {
    int span = bb.GetInt(vectorLocation - 4);//lvector length
    int start = 0;
    while (span != 0) {
      int middle = span / 2;
      int tableOffset = Table.__indirect(vectorLocation + 4 * (start + middle), bb);
      int comp = bb.GetInt(Table.__offset(4, bb.Length - tableOffset, bb)).CompareTo(key);
      if (comp > 0) {
        span = middle;
      } else if (comp < 0) {
        middle++;
        start += middle;
        span -= middle;
      } else {
        return new valueint().__assign(tableOffset, bb);
      }
    }
    return null;
  }
}


}
