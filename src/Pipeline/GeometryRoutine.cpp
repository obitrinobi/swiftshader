// Copyright 2016 The SwiftShader Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "GeometryRoutine.hpp"

#include "Constants.hpp"
#include "SpirvShader.hpp"
#include "Device/Renderer.hpp"

namespace sw {

GeometryRoutine::GeometryRoutine(
    const GeometryProcessor::State &state,
    vk::PipelineLayout const *pipelineLayout,
    SpirvShader const *spirvShader)
    : routine(pipelineLayout)
    , state(state)
    , spirvShader(spirvShader)
{
	if(spirvShader) 
	{
		spirvShader->emitProlog(&routine);
	}
}


GeometryRoutine::~GeometryRoutine()
{
}

void GeometryRoutine::generate()
{
	if(spirvShader) 
	{
		Pointer<Byte> cache = task + OFFSET(VertexTask, vertexCache);
		Pointer<Byte> vertexCache = cache + OFFSET(VertexCache, vertex);
		Pointer<UInt> tagCache = Pointer<UInt>(cache + OFFSET(VertexCache, tag));

		UInt vertexCount = *Pointer<UInt>(task + OFFSET(VertexTask, vertexCount));

		constants = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData, constants));

		// Check the cache one vertex index at a time. If a hit occurs, copy from the cache to the 'vertex' output buffer.
		// On a cache miss, process a SIMD width of consecutive indices from the input batch. They're written to the cache
		// in reverse order to guarantee that the first one doesn't get evicted and can be written out.

		Do
		{
			//UInt index = *batch;
			//UInt cacheIndex = index & VertexCache::TAG_MASK;

			//If(tagCache[cacheIndex] != index)
			{
				readInput(batch);
				program(batch, vertexCount);
			//	computeClipFlags();
			//	computeCullMask();

			//	writeCache(vertexCache, tagCache, batch);
			}

			//Pointer<Byte> cacheEntry = vertexCache + cacheIndex * UInt((int)sizeof(Vertex));

			// For points, vertexCount is 1 per primitive, so duplicate vertex for all 3 vertices of the primitive
			//for(int i = 0; i < (state.isPoint ? 3 : 1); i++)
			//{
			//	writeVertex(vertex, cacheEntry);
			//	vertex += sizeof(Vertex);
			//}

			batch = Pointer<UInt>(Pointer<Byte>(batch) + sizeof(uint32_t));
			vertexCount--;
		}
		Until(vertexCount == 0);
	
		Return();
	}
}

void GeometryRoutine::readInput(Pointer<UInt> &batch)
{
	for(int i = 0; i < MAX_INTERFACE_COMPONENTS; i += 4)
	{
		if(spirvShader->inputs[i + 0].Type != SpirvShader::ATTRIBTYPE_UNUSED ||
		   spirvShader->inputs[i + 1].Type != SpirvShader::ATTRIBTYPE_UNUSED ||
		   spirvShader->inputs[i + 2].Type != SpirvShader::ATTRIBTYPE_UNUSED ||
		   spirvShader->inputs[i + 3].Type != SpirvShader::ATTRIBTYPE_UNUSED)
		{
			Pointer<Byte> input = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData, input) + sizeof(void *) * (i / 4));
			UInt stride = *Pointer<UInt>(data + OFFSET(DrawData, stride) + sizeof(uint32_t) * (i / 4));
			Int baseVertex = *Pointer<Int>(data + OFFSET(DrawData, baseVertex));
			UInt robustnessSize(0);
			if(state.robustBufferAccess)
			{
				robustnessSize = *Pointer<UInt>(data + OFFSET(DrawData, robustnessSize) + sizeof(uint32_t) * (i / 4));
			}

			auto value = readStream(input, stride, state.input[i / 4], batch, state.robustBufferAccess, robustnessSize, baseVertex);
			routine.inputs[i + 0] = value.x;
			routine.inputs[i + 1] = value.y;
			routine.inputs[i + 2] = value.z;
			routine.inputs[i + 3] = value.w;
		}
	}
}


Vector4f GeometryRoutine::readStream(Pointer<Byte> &buffer, UInt &stride, const Stream &stream, Pointer<UInt> &batch,
                                   bool robustBufferAccess, UInt &robustnessSize, Int baseVertex)
{
	Vector4f v;
	// Because of the following rule in the Vulkan spec, we do not care if a very large negative
	// baseVertex would overflow all the way back into a valid region of the index buffer:
	// "Out-of-bounds buffer loads will return any of the following values :
	//  - Values from anywhere within the memory range(s) bound to the buffer (possibly including
	//    bytes of memory past the end of the buffer, up to the end of the bound range)."
	UInt4 offsets = (*Pointer<UInt4>(As<Pointer<UInt4>>(batch)) + As<UInt4>(Int4(baseVertex))) * UInt4(stride);

	Pointer<Byte> source0 = buffer + offsets.x;
	Pointer<Byte> source1 = buffer + offsets.y;
	Pointer<Byte> source2 = buffer + offsets.z;
	Pointer<Byte> source3 = buffer + offsets.w;

	vk::Format format(stream.format);

	UInt4 zero(0);
	if(robustBufferAccess)
	{
		// TODO(b/141124876): Optimize for wide-vector gather operations.
		UInt4 limits = offsets + UInt4(format.bytes());
		Pointer<Byte> zeroSource = As<Pointer<Byte>>(&zero);
		source0 = IfThenElse(limits.x <= robustnessSize, source0, zeroSource);
		source1 = IfThenElse(limits.y <= robustnessSize, source1, zeroSource);
		source2 = IfThenElse(limits.z <= robustnessSize, source2, zeroSource);
		source3 = IfThenElse(limits.w <= robustnessSize, source3, zeroSource);
	}

	int componentCount = format.componentCount();
	bool normalized = !format.isUnnormalizedInteger();
	bool isNativeFloatAttrib = (stream.attribType == SpirvShader::ATTRIBTYPE_FLOAT) || normalized;
	bool bgra = false;

	switch(stream.format)
	{
		case VK_FORMAT_R32_SFLOAT:
		case VK_FORMAT_R32G32_SFLOAT:
		case VK_FORMAT_R32G32B32_SFLOAT:
		case VK_FORMAT_R32G32B32A32_SFLOAT:
		{
			if(componentCount == 0)
			{
				// Null stream, all default components
			}
			else
			{
				if(componentCount == 1)
				{
					v.x.x = *Pointer<Float>(source0);
					v.x.y = *Pointer<Float>(source1);
					v.x.z = *Pointer<Float>(source2);
					v.x.w = *Pointer<Float>(source3);
				}
				else
				{
					v.x = *Pointer<Float4>(source0);
					v.y = *Pointer<Float4>(source1);
					v.z = *Pointer<Float4>(source2);
					v.w = *Pointer<Float4>(source3);

					transpose4xN(v.x, v.y, v.z, v.w, componentCount);
				}
			}
		}
		break;
		case VK_FORMAT_B8G8R8A8_UNORM:
			bgra = true;
			// [[fallthrough]]
		case VK_FORMAT_R8_UNORM:
		case VK_FORMAT_R8G8_UNORM:
		case VK_FORMAT_R8G8B8A8_UNORM:
		case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
			v.x = Float4(*Pointer<Byte4>(source0));
			v.y = Float4(*Pointer<Byte4>(source1));
			v.z = Float4(*Pointer<Byte4>(source2));
			v.w = Float4(*Pointer<Byte4>(source3));

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);

			if(componentCount >= 1) v.x *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleByte));
			if(componentCount >= 2) v.y *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleByte));
			if(componentCount >= 3) v.z *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleByte));
			if(componentCount >= 4) v.w *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleByte));
			break;
		case VK_FORMAT_R8_UINT:
		case VK_FORMAT_R8G8_UINT:
		case VK_FORMAT_R8G8B8A8_UINT:
		case VK_FORMAT_A8B8G8R8_UINT_PACK32:
			v.x = As<Float4>(Int4(*Pointer<Byte4>(source0)));
			v.y = As<Float4>(Int4(*Pointer<Byte4>(source1)));
			v.z = As<Float4>(Int4(*Pointer<Byte4>(source2)));
			v.w = As<Float4>(Int4(*Pointer<Byte4>(source3)));

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);
			break;
		case VK_FORMAT_R8_SNORM:
		case VK_FORMAT_R8G8_SNORM:
		case VK_FORMAT_R8G8B8A8_SNORM:
		case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
			v.x = Float4(*Pointer<SByte4>(source0));
			v.y = Float4(*Pointer<SByte4>(source1));
			v.z = Float4(*Pointer<SByte4>(source2));
			v.w = Float4(*Pointer<SByte4>(source3));

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);

			if(componentCount >= 1) v.x = Max(v.x * *Pointer<Float4>(constants + OFFSET(Constants, unscaleSByte)), Float4(-1.0f));
			if(componentCount >= 2) v.y = Max(v.y * *Pointer<Float4>(constants + OFFSET(Constants, unscaleSByte)), Float4(-1.0f));
			if(componentCount >= 3) v.z = Max(v.z * *Pointer<Float4>(constants + OFFSET(Constants, unscaleSByte)), Float4(-1.0f));
			if(componentCount >= 4) v.w = Max(v.w * *Pointer<Float4>(constants + OFFSET(Constants, unscaleSByte)), Float4(-1.0f));
			break;
		case VK_FORMAT_R8_SINT:
		case VK_FORMAT_R8G8_SINT:
		case VK_FORMAT_R8G8B8A8_SINT:
		case VK_FORMAT_A8B8G8R8_SINT_PACK32:
			v.x = As<Float4>(Int4(*Pointer<SByte4>(source0)));
			v.y = As<Float4>(Int4(*Pointer<SByte4>(source1)));
			v.z = As<Float4>(Int4(*Pointer<SByte4>(source2)));
			v.w = As<Float4>(Int4(*Pointer<SByte4>(source3)));

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);
			break;
		case VK_FORMAT_R16_SNORM:
		case VK_FORMAT_R16G16_SNORM:
		case VK_FORMAT_R16G16B16A16_SNORM:
			v.x = Float4(*Pointer<Short4>(source0));
			v.y = Float4(*Pointer<Short4>(source1));
			v.z = Float4(*Pointer<Short4>(source2));
			v.w = Float4(*Pointer<Short4>(source3));

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);

			if(componentCount >= 1) v.x = Max(v.x * *Pointer<Float4>(constants + OFFSET(Constants, unscaleShort)), Float4(-1.0f));
			if(componentCount >= 2) v.y = Max(v.y * *Pointer<Float4>(constants + OFFSET(Constants, unscaleShort)), Float4(-1.0f));
			if(componentCount >= 3) v.z = Max(v.z * *Pointer<Float4>(constants + OFFSET(Constants, unscaleShort)), Float4(-1.0f));
			if(componentCount >= 4) v.w = Max(v.w * *Pointer<Float4>(constants + OFFSET(Constants, unscaleShort)), Float4(-1.0f));
			break;
		case VK_FORMAT_R16_SINT:
		case VK_FORMAT_R16G16_SINT:
		case VK_FORMAT_R16G16B16A16_SINT:
			v.x = As<Float4>(Int4(*Pointer<Short4>(source0)));
			v.y = As<Float4>(Int4(*Pointer<Short4>(source1)));
			v.z = As<Float4>(Int4(*Pointer<Short4>(source2)));
			v.w = As<Float4>(Int4(*Pointer<Short4>(source3)));

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);
			break;
		case VK_FORMAT_R16_UNORM:
		case VK_FORMAT_R16G16_UNORM:
		case VK_FORMAT_R16G16B16A16_UNORM:
			v.x = Float4(*Pointer<UShort4>(source0));
			v.y = Float4(*Pointer<UShort4>(source1));
			v.z = Float4(*Pointer<UShort4>(source2));
			v.w = Float4(*Pointer<UShort4>(source3));

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);

			if(componentCount >= 1) v.x *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleUShort));
			if(componentCount >= 2) v.y *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleUShort));
			if(componentCount >= 3) v.z *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleUShort));
			if(componentCount >= 4) v.w *= *Pointer<Float4>(constants + OFFSET(Constants, unscaleUShort));
			break;
		case VK_FORMAT_R16_UINT:
		case VK_FORMAT_R16G16_UINT:
		case VK_FORMAT_R16G16B16A16_UINT:
			v.x = As<Float4>(Int4(*Pointer<UShort4>(source0)));
			v.y = As<Float4>(Int4(*Pointer<UShort4>(source1)));
			v.z = As<Float4>(Int4(*Pointer<UShort4>(source2)));
			v.w = As<Float4>(Int4(*Pointer<UShort4>(source3)));

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);
			break;
		case VK_FORMAT_R32_SINT:
		case VK_FORMAT_R32G32_SINT:
		case VK_FORMAT_R32G32B32_SINT:
		case VK_FORMAT_R32G32B32A32_SINT:
			v.x = *Pointer<Float4>(source0);
			v.y = *Pointer<Float4>(source1);
			v.z = *Pointer<Float4>(source2);
			v.w = *Pointer<Float4>(source3);

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);
			break;
		case VK_FORMAT_R32_UINT:
		case VK_FORMAT_R32G32_UINT:
		case VK_FORMAT_R32G32B32_UINT:
		case VK_FORMAT_R32G32B32A32_UINT:
			v.x = *Pointer<Float4>(source0);
			v.y = *Pointer<Float4>(source1);
			v.z = *Pointer<Float4>(source2);
			v.w = *Pointer<Float4>(source3);

			transpose4xN(v.x, v.y, v.z, v.w, componentCount);
			break;
		case VK_FORMAT_R16_SFLOAT:
		case VK_FORMAT_R16G16_SFLOAT:
		case VK_FORMAT_R16G16B16A16_SFLOAT:
		{
			if(componentCount >= 1)
			{
				UShort x0 = *Pointer<UShort>(source0 + 0);
				UShort x1 = *Pointer<UShort>(source1 + 0);
				UShort x2 = *Pointer<UShort>(source2 + 0);
				UShort x3 = *Pointer<UShort>(source3 + 0);

				v.x.x = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(x0) * 4);
				v.x.y = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(x1) * 4);
				v.x.z = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(x2) * 4);
				v.x.w = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(x3) * 4);
			}

			if(componentCount >= 2)
			{
				UShort y0 = *Pointer<UShort>(source0 + 2);
				UShort y1 = *Pointer<UShort>(source1 + 2);
				UShort y2 = *Pointer<UShort>(source2 + 2);
				UShort y3 = *Pointer<UShort>(source3 + 2);

				v.y.x = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(y0) * 4);
				v.y.y = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(y1) * 4);
				v.y.z = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(y2) * 4);
				v.y.w = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(y3) * 4);
			}

			if(componentCount >= 3)
			{
				UShort z0 = *Pointer<UShort>(source0 + 4);
				UShort z1 = *Pointer<UShort>(source1 + 4);
				UShort z2 = *Pointer<UShort>(source2 + 4);
				UShort z3 = *Pointer<UShort>(source3 + 4);

				v.z.x = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(z0) * 4);
				v.z.y = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(z1) * 4);
				v.z.z = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(z2) * 4);
				v.z.w = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(z3) * 4);
			}

			if(componentCount >= 4)
			{
				UShort w0 = *Pointer<UShort>(source0 + 6);
				UShort w1 = *Pointer<UShort>(source1 + 6);
				UShort w2 = *Pointer<UShort>(source2 + 6);
				UShort w3 = *Pointer<UShort>(source3 + 6);

				v.w.x = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(w0) * 4);
				v.w.y = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(w1) * 4);
				v.w.z = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(w2) * 4);
				v.w.w = *Pointer<Float>(constants + OFFSET(Constants, half2float) + Int(w3) * 4);
			}
		}
		break;
		case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
			bgra = true;
			// [[fallthrough]]
		case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
		{
			Int4 src;
			src = Insert(src, *Pointer<Int>(source0), 0);
			src = Insert(src, *Pointer<Int>(source1), 1);
			src = Insert(src, *Pointer<Int>(source2), 2);
			src = Insert(src, *Pointer<Int>(source3), 3);
			v.x = Float4((src << 22) >> 22);
			v.y = Float4((src << 12) >> 22);
			v.z = Float4((src << 02) >> 22);
			v.w = Float4(src >> 30);

			v.x = Max(v.x * Float4(1.0f / 0x1FF), Float4(-1.0f));
			v.y = Max(v.y * Float4(1.0f / 0x1FF), Float4(-1.0f));
			v.z = Max(v.z * Float4(1.0f / 0x1FF), Float4(-1.0f));
			v.w = Max(v.w, Float4(-1.0f));
		}
		break;
		case VK_FORMAT_A2R10G10B10_SINT_PACK32:
			bgra = true;
			// [[fallthrough]]
		case VK_FORMAT_A2B10G10R10_SINT_PACK32:
		{
			Int4 src;
			src = Insert(src, *Pointer<Int>(source0), 0);
			src = Insert(src, *Pointer<Int>(source1), 1);
			src = Insert(src, *Pointer<Int>(source2), 2);
			src = Insert(src, *Pointer<Int>(source3), 3);
			v.x = As<Float4>((src << 22) >> 22);
			v.y = As<Float4>((src << 12) >> 22);
			v.z = As<Float4>((src << 02) >> 22);
			v.w = As<Float4>(src >> 30);
		}
		break;
		case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
			bgra = true;
			// [[fallthrough]]
		case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
		{
			Int4 src;
			src = Insert(src, *Pointer<Int>(source0), 0);
			src = Insert(src, *Pointer<Int>(source1), 1);
			src = Insert(src, *Pointer<Int>(source2), 2);
			src = Insert(src, *Pointer<Int>(source3), 3);

			v.x = Float4(src & Int4(0x3FF));
			v.y = Float4((src >> 10) & Int4(0x3FF));
			v.z = Float4((src >> 20) & Int4(0x3FF));
			v.w = Float4((src >> 30) & Int4(0x3));

			v.x *= Float4(1.0f / 0x3FF);
			v.y *= Float4(1.0f / 0x3FF);
			v.z *= Float4(1.0f / 0x3FF);
			v.w *= Float4(1.0f / 0x3);
		}
		break;
		case VK_FORMAT_A2R10G10B10_UINT_PACK32:
			bgra = true;
			// [[fallthrough]]
		case VK_FORMAT_A2B10G10R10_UINT_PACK32:
		{
			Int4 src;
			src = Insert(src, *Pointer<Int>(source0), 0);
			src = Insert(src, *Pointer<Int>(source1), 1);
			src = Insert(src, *Pointer<Int>(source2), 2);
			src = Insert(src, *Pointer<Int>(source3), 3);

			v.x = As<Float4>(src & Int4(0x3FF));
			v.y = As<Float4>((src >> 10) & Int4(0x3FF));
			v.z = As<Float4>((src >> 20) & Int4(0x3FF));
			v.w = As<Float4>((src >> 30) & Int4(0x3));
		}
		break;
		default:
			UNSUPPORTED("stream.format %d", int(stream.format));
	}

	if(bgra)
	{
		// Swap red and blue
		Float4 t = v.x;
		v.x = v.z;
		v.z = t;
	}

	if(componentCount < 1) v.x = Float4(0.0f);
	if(componentCount < 2) v.y = Float4(0.0f);
	if(componentCount < 3) v.z = Float4(0.0f);
	if(componentCount < 4) v.w = isNativeFloatAttrib ? As<Float4>(Float4(1.0f)) : As<Float4>(Int4(1));

	return v;
}


}

