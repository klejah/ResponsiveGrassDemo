/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

layout(binding=0, offset=0) uniform atomic_uint visibleBladeCount;

struct IndirectStruct
{
    uint count;
	uint primCount;
	uint firstIndex;
	uint baseVertex;
	uint baseIndex;
};

layout(std430, binding=1) writeonly buffer indirect {
    IndirectStruct ind[];
};

layout(local_size_x=1, local_size_y=1, local_size_z=1) in;

void main()
{
    ind[0].count = atomicCounter(visibleBladeCount);
}