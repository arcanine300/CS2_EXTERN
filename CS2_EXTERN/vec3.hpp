#include "MemoryManager.hpp"
#include "ImGui/imgui.h"

typedef union vec3 {
	struct { float x, y, z; };
	friend vec3 operator + (vec3 const& lhs, vec3 const& rhs);
	friend vec3 operator - (vec3 const& lhs, vec3 const& rhs);
	friend vec3 operator / (vec3 const& lhs, int const& rhs);
	friend vec3 operator * (vec3 const& lhs, float const& rhs);
	bool operator != (vec3 const& rhs);
	bool operator == (vec3 const& rhs);
	vec3();
	vec3(float x, float y, float z);
} vec3;

static bool worldToScreen(const vec3& world, vec3& screen, const ViewMatrix& vm) {
	float w = vm[3][0] * world.x + vm[3][1] * world.y + vm[3][2] * world.z + vm[3][3];

	if (w < 0.001f) {
		return false;
	}

	const float x = world.x * vm[0][0] + world.y * vm[0][1] + world.z * vm[0][2] + vm[0][3];
	const float y = world.x * vm[1][0] + world.y * vm[1][1] + world.z * vm[1][2] + vm[1][3];

	w = 1.f / w;
	float nx = x * w;
	float ny = y * w;

	const ImVec2 size = ImGui::GetIO().DisplaySize;
	screen.x = (size.x * 0.5f * nx) + (nx + size.x * 0.5f);
	screen.y = -(size.y * 0.5f * ny) + (ny + size.y * 0.5f);

	return true;
}