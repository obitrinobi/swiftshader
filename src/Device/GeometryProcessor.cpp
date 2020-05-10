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

#include "GeometryProcessor.hpp"
#include "Pipeline/GeometryRoutine.hpp"
#include "Pipeline/GeometryProgram.hpp"

namespace sw {

uint32_t GeometryProcessor::States::computeHash()
{
	uint32_t *state = reinterpret_cast<uint32_t *>(this);
	uint32_t hash = 0;

	for(unsigned int i = 0; i < sizeof(States) / sizeof(uint32_t); i++)
	{
		hash ^= state[i];
	}

	return hash;
}

GeometryProcessor::GeometryProcessor()
{
}

GeometryProcessor::~GeometryProcessor()
{
}

void GeometryProcessor::setRoutineCacheSize(int cacheSize) 
{
	//TODO imlement me
}

const GeometryProcessor::State GeometryProcessor::update(const sw::Context *context)
{
	State state;

	if(context->geometryShader) 
	{
		state.shaderID = context->geometryShader->getSerialID();
		state.robustBufferAccess = context->robustBufferAccess;
		state.isPoint = context->topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		for(int i = 0; i < MAX_INTERFACE_COMPONENTS / 4; i++)
		{
			state.input[i].format = context->input[i].format;
			// TODO: get rid of attribType -- just keep the VK format all the way through, this fully determines
			// how to handle the attribute.
			state.input[i].attribType = context->geometryShader->inputs[i * 4].Type;
		}
	}
	state.hash = state.computeHash();

	return state;
}

GeometryProcessor::RoutineType GeometryProcessor::routine(const State &state,
                                                      vk::PipelineLayout const *pipelineLayout,
                                                      SpirvShader const *geometryShader,
                                                      const vk::DescriptorSet::Bindings &descriptorSets)
{
	// TODO fix me auto routine = routineCache->query(state);
	//auto routine = nullptr;
	//if(geometryShader)  // Create one
	//{
		GeometryRoutine *generator = new GeometryProgram(state, pipelineLayout, geometryShader, descriptorSets);
		generator->generate();
		auto routine = (*generator)("GeometryRoutine_%0.8X", state.shaderID);
	
		delete generator;

		//routineCache->add(state, routine);
	//}

	return routine;
}

}