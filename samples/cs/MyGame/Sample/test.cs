// <auto-generated>
//  automatically generated by the FlatBuffers compiler, do not modify
// </auto-generated>

namespace MyGame.Sample
{

using global::System;
using global::System.Collections.Generic;
using global::FlatBuffers;

public struct test : IFlatbufferObject
{
  private Table __p;
  public ByteBuffer ByteBuffer { get { return __p.bb; } }
  public static void ValidateVersion() { FlatBufferConstants.FLATBUFFERS_2_0_0(); }
  public static test GetRootAstest(ByteBuffer _bb) { return GetRootAstest(_bb, new test()); }
  public static test GetRootAstest(ByteBuffer _bb, test obj) { return (obj.__assign(_bb.GetInt(_bb.Position) + _bb.Position, _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __p = new Table(_i, _bb); }
  public test __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public MyGame.Sample.value? Values(int j) { int o = __p.__offset(4); return o != 0 ? (MyGame.Sample.value?)(new MyGame.Sample.value()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int ValuesLength { get { int o = __p.__offset(4); return o != 0 ? __p.__vector_len(o) : 0; } }
  public MyGame.Sample.value? ValuesByKey(string key) { int o = __p.__offset(4); return o != 0 ? MyGame.Sample.value.__lookup_by_key(__p.__vector(o), key, __p.bb) : null; }
  public MyGame.Sample.valueint? Valuesint(int j) { int o = __p.__offset(6); return o != 0 ? (MyGame.Sample.valueint?)(new MyGame.Sample.valueint()).__assign(__p.__indirect(__p.__vector(o) + j * 4), __p.bb) : null; }
  public int ValuesintLength { get { int o = __p.__offset(6); return o != 0 ? __p.__vector_len(o) : 0; } }
  public MyGame.Sample.valueint? ValuesintByKey(int key) { int o = __p.__offset(6); return o != 0 ? MyGame.Sample.valueint.__lookup_by_key(__p.__vector(o), key, __p.bb) : null; }

  public static Offset<MyGame.Sample.test> Createtest(FlatBufferBuilder builder,
      VectorOffset valuesOffset = default(VectorOffset),
      VectorOffset valuesintOffset = default(VectorOffset)) {
    builder.StartTable(2);
    test.AddValuesint(builder, valuesintOffset);
    test.AddValues(builder, valuesOffset);
    return test.Endtest(builder);
  }

  public static void Starttest(FlatBufferBuilder builder) { builder.StartTable(2); }
  public static void AddValues(FlatBufferBuilder builder, VectorOffset valuesOffset) { builder.AddOffset(0, valuesOffset.Value, 0); }
  public static VectorOffset CreateValuesVector(FlatBufferBuilder builder, Offset<MyGame.Sample.value>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateValuesVectorBlock(FlatBufferBuilder builder, Offset<MyGame.Sample.value>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartValuesVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static void AddValuesint(FlatBufferBuilder builder, VectorOffset valuesintOffset) { builder.AddOffset(1, valuesintOffset.Value, 0); }
  public static VectorOffset CreateValuesintVector(FlatBufferBuilder builder, Offset<MyGame.Sample.valueint>[] data) { builder.StartVector(4, data.Length, 4); for (int i = data.Length - 1; i >= 0; i--) builder.AddOffset(data[i].Value); return builder.EndVector(); }
  public static VectorOffset CreateValuesintVectorBlock(FlatBufferBuilder builder, Offset<MyGame.Sample.valueint>[] data) { builder.StartVector(4, data.Length, 4); builder.Add(data); return builder.EndVector(); }
  public static void StartValuesintVector(FlatBufferBuilder builder, int numElems) { builder.StartVector(4, numElems, 4); }
  public static Offset<MyGame.Sample.test> Endtest(FlatBufferBuilder builder) {
    int o = builder.EndTable();
    return new Offset<MyGame.Sample.test>(o);
  }
  public static void FinishtestBuffer(FlatBufferBuilder builder, Offset<MyGame.Sample.test> offset) { builder.Finish(offset.Value); }
  public static void FinishSizePrefixedtestBuffer(FlatBufferBuilder builder, Offset<MyGame.Sample.test> offset) { builder.FinishSizePrefixed(offset.Value); }
}


}