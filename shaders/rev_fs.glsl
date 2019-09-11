#version 420 core

uniform vec2 world_pos;
uniform vec2 render_target_size;
uniform float radius;

uniform vec2 arena_middle; // = vec2(-390, 2215);
uniform float arena_size; // = 140;

const float A = (arena_middle.y + arena_size / 2.0);
const float B = (arena_middle.x - arena_size / 2.0);

const float R_scaled = radius / (arena_size*0.5);

float parab(float x) {
	float temp = (x/R_scaled);
	return 1-(temp*temp);
}


float parab2(float x) {
	return parab(x)*parab(x);
}

float par(float x) {
	float temp = (x/R_scaled);
	return (temp * temp);	
}

float par2(float x) {
	return par(x) * par(x);
}

vec2 map_to_dc(vec2 screenpos) {
	return vec2(screenpos.x / (0.5 * render_target_size.x) - 1, screenpos.y / (0.5 * render_target_size.y) - 1);
}

vec2 world2screen(vec2 world) {

	float nx = (-world.y + A) / arena_size;
	float ny = (world.x - B) / arena_size;
	return vec2(nx * 2 - 1, ny * 2 - 1);
}

out vec3 color;

void main() {
  vec2 scr = world2screen(world_pos);
  vec2 fr = map_to_dc(gl_FragCoord.xy);
  float d = length(fr - scr);
  
  color = vec3(par2(d), 0, 0);
}