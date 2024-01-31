#include "vec3.hpp"

vec3 operator + (vec3 const& lhs, vec3 const& rhs) {
	return vec3{ lhs.x + rhs.x,lhs.y + rhs.y,lhs.z + rhs.z };
}
vec3 operator - (vec3 const& lhs, vec3 const& rhs) {
	return vec3{ lhs.x - rhs.x,lhs.y - rhs.y,lhs.z - rhs.z };
}
vec3 operator / (vec3 const& lhs, int const& rhs) {
	return vec3{ lhs.x / rhs, lhs.y / rhs, lhs.z / rhs };
}
vec3 operator * (vec3 const& lhs, float const& rhs) {
	return vec3{ lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
}
bool vec3::operator!=(vec3 const& rhs) {
	return (this->x != rhs.x || this->y != rhs.y || this->z != rhs.z);
}
bool vec3::operator==(vec3 const& rhs) {
	return (this->x == rhs.x && this->y == rhs.y && this->z == rhs.z);
}
vec3::vec3(float x, float y, float z) {
	this->x = x; this->y = y; this->z = z;
}
vec3::vec3() {}