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

#ifndef sw_GeometryRoutine_hpp
#define sw_GeometryRoutine_hpp

#include "ShaderCore.hpp"
#include "SpirvShader.hpp"
#include "Device/GeometryProcessor.hpp"


namespace vk {
class PipelineLayout;
}

namespace sw {

class GeometryRoutinePrototype : public GeometryRoutineFunction
{
public:
	GeometryRoutinePrototype()
	    : vertex(Arg<0>())
	    , batch(Arg<1>())
	    , task(Arg<2>())
	    , data(Arg<3>())
	{}
	virtual ~GeometryRoutinePrototype() {}

protected:
	Pointer<Byte> vertex;
	Pointer<UInt> batch;
	Pointer<Byte> task;
	Pointer<Byte> data;
};

class GeometryRoutine : public GeometryRoutinePrototype
{
public:
	GeometryRoutine(
	    const GeometryProcessor::State &state,
	    vk::PipelineLayout const *pipelineLayout,
	    SpirvShader const *spirvShader);
	virtual ~GeometryRoutine();

	void generate();

protected:
	Pointer<Byte> constants;

	Int clipFlags;
	Int cullMask;

	SpirvRoutine routine;

	const GeometryProcessor::State &state;
	SpirvShader const *const spirvShader;

private:
	virtual void program(Pointer<UInt> &batch, UInt &vertexCount) = 0;

	typedef GeometryProcessor::State::Input Stream;

	void readInput(Pointer<UInt> &batch);
	Vector4f readStream(Pointer<Byte> &buffer, UInt &stride, const Stream &stream, Pointer<UInt> &batch,
	                    bool robustBufferAccess, UInt &robustnessSize, Int baseVertex);
	/*	void computeClipFlags();
	void computeCullMask();
	void writeCache(Pointer<Byte> &vertexCache, Pointer<UInt> &tagCache, Pointer<UInt> &batch);
	void writeVertex(const Pointer<Byte> &vertex, Pointer<Byte> &cacheEntry);*/
};

}  // namespace sw

#endif  // sw_GeometryRoutine_hpp
