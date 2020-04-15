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
	Pointer<Byte> cache = task + OFFSET(VertexTask, vertexCache);
	Pointer<Byte> vertexCache = cache + OFFSET(VertexCache, vertex);
	Pointer<UInt> tagCache = Pointer<UInt>(cache + OFFSET(VertexCache, tag));

	UInt vertexCount = *Pointer<UInt>(task + OFFSET(VertexTask, vertexCount));

	constants = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData, constants));

	// Check the cache one vertex index at a time. If a hit occurs, copy from the cache to the 'vertex' output buffer.
	// On a cache miss, process a SIMD width of consecutive indices from the input batch. They're written to the cache
	// in reverse order to guarantee that the first one doesn't get evicted and can be written out.

	/*Do
	{
		UInt index = *batch;
		UInt cacheIndex = index & VertexCache::TAG_MASK;

		If(tagCache[cacheIndex] != index)
		{
			readInput(batch);
			program(batch, vertexCount);
			computeClipFlags();
			computeCullMask();

			writeCache(vertexCache, tagCache, batch);
		}

		Pointer<Byte> cacheEntry = vertexCache + cacheIndex * UInt((int)sizeof(Vertex));

		// For points, vertexCount is 1 per primitive, so duplicate vertex for all 3 vertices of the primitive
		for(int i = 0; i < (state.isPoint ? 3 : 1); i++)
		{
			writeVertex(vertex, cacheEntry);
			vertex += sizeof(Vertex);
		}

		batch = Pointer<UInt>(Pointer<Byte>(batch) + sizeof(uint32_t));
		vertexCount--;
	}
	Until(vertexCount == 0);
	*/
	Return();
}

}

