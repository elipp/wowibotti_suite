#version 420 core

layout(location = 0) in vec2 pos;

uniform float arena_size; // = 140;
uniform vec2 player_pos;

vec2 world2screen2(vec2 world) {
	vec2 d = player_pos - world;
	return vec2(d.y/(0.5*arena_size), -d.x/(0.5*arena_size));
}

void main(){
	gl_Position = vec4(world2screen2(pos.xy), 0., 1.0);
}
