local m = {}

_G.flatbuffersnative = require("flatbuffersnative")

m.Builder = require("flatbuffers.builder").New
m.N = _G.flatbuffersnative.N

m.binaryArray = {}
m.binaryArray.New = _G.flatbuffersnative.new_binaryarray

m.view = {}
m.view.New = _G.flatbuffersnative.new_view

return m
