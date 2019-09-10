#version 420 core

layout(location = 0) in vec3 pos;

// pos.z is actually radius

const vec2 am = vec2(-390, 2215);
const float as = 140;

const float A = (am.y + as / 2.0);
const float B = (am.x - as / 2.0);

vec2 world2screen(vec2 world) {

	float nx = (-world.y + A) / as;
	float ny = (world.x - B) / as;
	return vec2(nx * 2 - 1, ny * 2 - 1);
}

out float radius;

void main(){
	radius = pos.z/(0.5*as);
	//gl_Position = vec4(pos.xy, 0.0, 1.0);
	gl_Position = vec4(world2screen(pos.xy), 0., 1.0);
}