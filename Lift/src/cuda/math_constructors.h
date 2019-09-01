#pragma once
#include <vector_functions.h>
#include <vector_types.h>
#include <cmath>
#include <cstdlib>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "preprocessor.h"

SUTIL_INLINE SUTIL_HOSTDEVICE float3 make_float3(const glm::vec3& vec){
	return make_float3(vec.x, vec.y, vec.z);
}

SUTIL_INLINE SUTIL_HOSTDEVICE int2 make_int2(const glm::ivec2 vec) {
	return make_int2(vec.x, vec.y);
}

