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

namespace sw {
	class GeometryProcessor
	{
	public:
	    GeometryProcessor(Context *context);

	    virtual ~GeometryProcessor();

	    void setFloatConstant(unsigned int index, const float value[4]);
	    void setIntegerConstant(unsigned int index, const int integer[4]);
	    void setBooleanConstant(unsigned int index, int boolean);

	private:
	    Context *const context;
	    // Shader constants
	    float4 c[VERTEX_UNIFORM_VECTORS + 1];  // One extra for indices out of range, c[VERTEX_UNIFORM_VECTORS] = {0, 0, 0, 0}
	    int4 i[16];
	    bool b[16];
	};
}

#endif  // sw_GeometryProcessor_hpp