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
		Do
		{

			v[0] = routine.buildInOutputs[counter++];
			v[1] = routine.buildInOutputs[counter++];
			v[2] = routine.buildInOutputs[counter++];
			v[3] = routine.buildInOutputs[counter++];
			X[vIndex] = v.x.x;
			Y[vIndex] = v.y.y;
			Z[vIndex] = v.z.z;
			W[vIndex] = v.w.w;
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

		Array<Float4> pos(3);
		Array<Float> X(3);
		Array<Float> Y(3);
		Array<Float> Z(3);
		Array<Float> W(3);
		
		*Pointer<Int4>(&pos[0]) = *Pointer<Int4>(v0 + OFFSET(Vertex, position), 16);
		*Pointer<Int4>(&pos[1]) = *Pointer<Int4>(v1 + OFFSET(Vertex, position), 16);
		*Pointer<Int4>(&pos[2]) = *Pointer<Int4>(v2 + OFFSET(Vertex, position), 16);
			
	
		//X[0] = pos.x;
		/*X[1] = *Pointer<Float>(v1 + OFFSET(Vertex, x));
		X[2] = *Pointer<Float>(v2 + OFFSET(Vertex, x));

		Y[0] = *Pointer<Float>(v0 + OFFSET(Vertex, y));
		Y[1] = *Pointer<Float>(v1 + OFFSET(Vertex, y));
		Y[2] = *Pointer<Float>(v2 + OFFSET(Vertex, y));

		Z[0] = *Pointer<Float>(v0 + OFFSET(Vertex, z));
		Z[1] = *Pointer<Float>(v1 + OFFSET(Vertex, z));
		Z[2] = *Pointer<Float>(v2 + OFFSET(Vertex, z));

		W[0] = *Pointer<Float>(v0 + OFFSET(Vertex, w));
		W[1] = *Pointer<Float>(v1 + OFFSET(Vertex, w));
		W[2] = *Pointer<Float>(v2 + OFFSET(Vertex, w));*/
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
			//v.x = Float(pos[i].x);
			//v.y = pos[i].y;
			//v.z = pos[i].z;
			//v.w = pos[i].w;
			*Pointer<Int4>(&gl_In[i * 7]) = *Pointer<Int4>(&pos[i]);
			//gl_In[i*7] = Float(pos[i][0]);
			//gl_In[i*7+1] = v.y;
			//gl_In[i*7+2] = v.z;
			//gl_In[i*7+3] = v.w;
			//gl_In[i*7+4] = w.x;
			//gl_In[i*7+5] = w.y;
			//gl_In[i*7+6] = w.z;
		}
	}

}


}  // namespace sw
