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
#include "Reactor/Print.hpp"
#include <iostream>
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
		//Pointer<Byte> cache = task + OFFSET(GeoemtryTask, geometryCache);
		//Pointer<Byte> vCache = cache + OFFSET(VertexCache, vertex);
		//Pointer<UInt> tagCache = Pointer<UInt>(cache + OFFSET(VertexCache, tag));
		
		UInt trianglesCount = *Pointer<UInt>(task + OFFSET(GeometryTask, trianglesCount));
		UInt primitiveCount = *Pointer<UInt>(task + OFFSET(GeometryTask, primitives));
		
		UInt trianglesIndex = 0;
		UInt primitiveIndex = 0;
		//constants = *Pointer<Pointer<Byte>>(data + OFFSET(DrawData, constants));

		// Check the cache one vertex index at a time. If a hit occurs, copy from the cache to the 'vertex' output buffer.
		// On a cache miss, process a SIMD width of consecutive indices from the input batch. They're written to the cache
		// in reverse order to guarantee that the first one doesn't get evicted and can be written out.
		Do
		{
			readInputPrimitive(triangles, trianglesIndex++);
			program(emittedTriangles);
			writePrimitives(emittedTriangles, primitiveIndex);
			primitiveIndex += primitiveCount;
			trianglesCount--;
		}
		Until(trianglesCount == 0);
		Return();
	}
}

void GeometryRoutine::writePrimitives(Pointer<Byte> &emittedTriangles, UInt index)
{
	//*Pointer<Int4>(vertex + OFFSET(Vertex, position))
	UInt primitivesCount = *Pointer<UInt>(task + OFFSET(GeometryTask, primitives));
	rr::Print("PrimitiveCount:{0}\n", primitivesCount);
	UInt vertexCount12 = *Pointer<UInt>(task + OFFSET(GeometryTask, vertices));
	UInt primitives = primitivesCount;	
	UInt counter = 0;
	UInt primitiveCounter = 0;
	Do
	{
		UInt vertexCount = *Pointer<UInt>(task + OFFSET(GeometryTask, vertices))/primitives;
		Array<Pointer<Byte>> triangle_offsets(3);
		triangle_offsets[0] = emittedTriangles + (index + primitiveCounter)*sizeof(Triangle) + OFFSET(Triangle, v0);
		triangle_offsets[1] = emittedTriangles + (index + primitiveCounter)*sizeof(Triangle) + OFFSET(Triangle, v1);
		triangle_offsets[2] = emittedTriangles + (index + primitiveCounter)*sizeof(Triangle) + OFFSET(Triangle, v2);
		
		primitiveCounter++;
		
		UInt vIndex = 0;
		Float4 v(.0f);
		Do
		{
			//*Pointer<Int4>(triangle_offsets[vIndex] + OFFSET(Vertex,position)) 
			Vector4f v(.0f, .0f, .0f, .0f);
			v.x = routine.buildInOutputs[counter++];
			v.y = routine.buildInOutputs[counter++];
			v.z = routine.buildInOutputs[counter++];
			v.w = routine.buildInOutputs[counter++];
			

			*Pointer<Float>(triangle_offsets[vIndex] + OFFSET(Vertex, x)) = v.x.x;
			*Pointer<Float>(triangle_offsets[vIndex] + OFFSET(Vertex, y)) = v.y.x;
			*Pointer<Float>(triangle_offsets[vIndex] + OFFSET(Vertex, z)) = v.z.x;
			*Pointer<Float>(triangle_offsets[vIndex] + OFFSET(Vertex, w)) = v.w.x;

			// Projection and viewport transform.
			Float4 w = As<Float4>(As<Int4>(v.w) | (As<Int4>(CmpEQ(v.w, Float4(0.0f))) & As<Int4>(Float4(1.0f))));
			Float4 rhw = Float4(1.0f) / w;

			Vector4f proj1(.0f, .0f, .0f, .0f);
			//proj1.x = As<Float4>(RoundInt(*Pointer<Float4>(data + OFFSET(DrawData, X0xF)) + v.x * rhw * *Pointer<Float4>(data + OFFSET(DrawData, WxF))));
			//proj1.y = As<Float4>(RoundInt(*Pointer<Float4>(data + OFFSET(DrawData, Y0xF)) + v.y * rhw * *Pointer<Float4>(data + OFFSET(DrawData, HxF))));
			//proj1.z = v.z * rhw;
			//proj1.w = rhw;
			transpose4x4(proj1.x, proj1.y, proj1.z, proj1.w);
			//Int px = As<Int>(proj1.x.x);
			//transpose4x4(proj1.x, proj1.y, proj1.z, proj1.w);
			//*Pointer<Float>(triangle_offsets[vIndex] + OFFSET(Vertex, projected.x)) = proj1.x.x;
			//*Pointer<Float>(triangle_offsets[vIndex] + OFFSET(Vertex, projected.y)) = proj1.y.x;
			//*Pointer<Float>(triangle_offsets[vIndex] + OFFSET(Vertex, projected.z)) = proj1.z.x;
			//*Pointer<Float>(triangle_offsets[vIndex] + OFFSET(Vertex, projected.w)) = proj1.w.x;
			
			*Pointer<Float4>(triangle_offsets[vIndex] + OFFSET(Vertex, projected),16) = proj1.w;
			//*Pointer<Float4>(vertexCache + sizeof(Vertex) * cacheIndex2 + OFFSET(Vertex, projected), 16) = proj.z;
			//*Pointer<Float4>(vertexCache + sizeof(Vertex) * cacheIndex1 + OFFSET(Vertex, projected), 16) = proj.y;
			//*Pointer<Float4>(vertexCache + sizeof(Vertex) * cacheIndex0 + OFFSET(Vertex, projected), 16) = proj.x;

			vIndex++;
			vertexCount--;
		}Until(vertexCount == 0);
		primitivesCount--;
	}Until(primitivesCount == 0);
}
void GeometryRoutine::readInputPrimitive(Pointer<Byte> &triangles, UInt index)
{
	if(spirvShader->hasVariable("gl_in")) 
	{
		auto gl_inId = spirvShader->getObjectId("gl_in");
		auto &gl_In = routine.getVariable(gl_inId);
		
		
		//const bool point = state.isDrawPoint;
		//const bool sprite = state.pointSprite;
		//const bool line = state.isDrawLine;
		//const bool triangle = state.isDrawSolidTriangle || sprite;
		//const bool solidTriangle = state.isDrawSolidTriangle;

		const int V0 = OFFSET(Triangle, v0);
		const int V1 = OFFSET(Triangle, v1); //(triangle || line) ? OFFSET(Triangle, v1) : OFFSET(Triangle, v0);
		const int V2 = OFFSET(Triangle, v2); //triangle ? OFFSET(Triangle, v2) : (line ? OFFSET(Triangle, v1) : OFFSET(Triangle, v0));
		
		Pointer<Byte> v0 = triangles + index*sizeof(Triangle) + V0;
		Pointer<Byte> v1 = triangles + index*sizeof(Triangle) + V1;
		Pointer<Byte> v2 = triangles + index*sizeof(Triangle) + V2;
		Array<Pointer<Byte>> triangle_offsets(3);
		triangle_offsets[0] = v0;
		triangle_offsets[1] = v1;
		triangle_offsets[2] = v2;

		for(int i = 0; i < MAX_INTERFACE_COMPONENTS; i+=4)
		{
			auto stream = state.input[i / 4];
			Vector4f res(0.f, 0.f, 0.f, 0.f);
			if(spirvShader->inputs[i].Type != SpirvShader::ATTRIBTYPE_UNUSED)
			{
				vk::Format format(stream.format);
				int componentCount = format.componentCount();
				for(int j = 0; j < componentCount; j++) 
				{
					res[j] = *Pointer<Float>(triangle_offsets[i / 4] + OFFSET(Vertex, v[j]));
				}
			} 
			routine.inputs[i] = res.x;
			routine.inputs[i+1] = res.y;
			routine.inputs[i+2] = res.z;
			routine.inputs[i+3] = res.w;
			
		}

		for(unsigned int i = 0; i < 3; i ++) 
		{
			Vector4f v;
			
			v.x = *Pointer<Float>(triangle_offsets[i] + OFFSET(Vertex, x));
			v.y = *Pointer<Float>(triangle_offsets[i] + OFFSET(Vertex, y));
			v.z = *Pointer<Float>(triangle_offsets[i] + OFFSET(Vertex, z));
			v.w = *Pointer<Float>(triangle_offsets[i] + OFFSET(Vertex, w));
			gl_In[i*7] = v.x;
			gl_In[i*7+1] = v.y;
			gl_In[i*7+2] = v.z;
			gl_In[i*7+3] = v.w;
			//gl_In[i*7+4] = w.x;
			//gl_In[i*7+5] = w.y;
			//gl_In[i*7+6] = w.z;
		}
	}

}


}  // namespace sw
