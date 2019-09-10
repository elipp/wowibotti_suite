#version 420 core

layout(location = 0) in vec3 pos;

// pos.z is actually radius

uniform vec2 arena_middle; // = vec2(-390, 2215);
uniform float arena_size; // = 140;

const float A = (arena_middle.y + arena_size / 2.0);
const float B = (arena_middle.x - arena_size / 2.0);

vec2 world2screen(vec2 world) {

	float nx = (-world.y + A) / arena_size;
	float ny = (world.x - B) / arena_size;
	return vec2(nx * 2 - 1, ny * 2 - 1);
}

out float radius;

void main(){
	radius = pos.z/(0.5*arena_size);
	//gl_Position = vec4(pos.xy, 0.0, 1.0);
	gl_Position = vec4(world2screen(pos.xy), 0., 1.0);
}