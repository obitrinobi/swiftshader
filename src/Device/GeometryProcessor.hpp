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

#ifndef sw_GeometryProcessor_hpp
#define sw_GeometryProcessor_hpp

#include "Context.hpp"
#include "Memset.hpp"
#include "RoutineCache.hpp"
#include "Pipeline/SpirvShader.hpp"

namespace sw {
struct DrawData;
struct Primitive;
struct Triangle;
struct GeometryTask
{
	unsigned int trianglesCount;
	unsigned int vertices;
	unsigned int primitives;

	//TODO cache
	//VertexCache vertexCache;
};

using GeometryRoutineFunction = FunctionT<void(Triangle *triangles, Triangle *emittedTriangles, GeometryTask* geometryTask, DrawData *const)>;

class GeometryProcessor
{
public:
	struct States : Memset<States>
	{
		States()
		    : Memset(this, 0)
		{}

		uint32_t computeHash();

		uint64_t shaderID;

		struct Input
		{
			operator bool() const  // Returns true if stream contains data
			{
				return format != VK_FORMAT_UNDEFINED;
			}

			VkFormat format;  // TODO(b/148016460): Could be restricted to VK_FORMAT_END_RANGE
			unsigned int attribType : BITS(SpirvShader::ATTRIBTYPE_LAST);
		};

		Input input[MAX_INTERFACE_COMPONENTS / 4];
		bool robustBufferAccess : 1;
		bool isPoint : 1;
	};
	struct State : States
	{
		bool operator==(const State &state) const;

		uint32_t hash;
	};
	GeometryProcessor();
	virtual ~GeometryProcessor();
	using RoutineType = GeometryRoutineFunction::RoutineType;

protected:
	const State update(const sw::Context *context);
	RoutineType routine(const State &state, vk::PipelineLayout const *pipelineLayout,
	                    SpirvShader const *geometryShader, const vk::DescriptorSet::Bindings &descriptorSets);
	void setRoutineCacheSize(int cacheSize);

private:
	// TODO implement
	// using RoutineCacheType = RoutineCacheT<State, VertexRoutineFunction::CFunctionType>;
	// RoutineCacheType *routineCache;
};

}  // namespace sw

#endif  // sw_GeometryProcessor_hpp