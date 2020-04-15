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
#include "Common/Debug.hpp"

namespace sw {

	GeometryProcessor::GeometryProcessor(Context *context) : context(context)
	{
	//routineCache = nullptr;
	//setRoutineCacheSize(1024);
	}

	GeometryProcessor::~GeometryProcessor()
	{
		//delete routineCache;
		//routineCache = nullptr;
	}

	void GeometryProcessor::setFloatConstant(unsigned int index, const float value[4])
	{
		if(index < VERTEX_UNIFORM_VECTORS)
		{
			c[index][0] = value[0];
			c[index][1] = value[1];
			c[index][2] = value[2];
			c[index][3] = value[3];
		}
		else
			ASSERT(false);
	}

	void GeometryProcessor::setIntegerConstant(unsigned int index, const int integer[4])
	{
		if(index < 16)
		{
			i[index][0] = integer[0];
			i[index][1] = integer[1];
			i[index][2] = integer[2];
			i[index][3] = integer[3];
		}
		else
			ASSERT(false);
	}

	void GeometryProcessor::setBooleanConstant(unsigned int index, int boolean)
	{
		if(index < 16)
		{
			b[index] = boolean != 0;
		}
		else
			ASSERT(false);
	}
}