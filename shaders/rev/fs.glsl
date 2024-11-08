#version 420 core

uniform vec2 world_pos;
uniform vec2 render_target_size;
uniform float radius;

uniform float arena_size;
uniform vec2 player_pos;

const float R_scaled = radius / (arena_size*0.5);

float linear(float x) {
	return (x/R_scaled);
}


float parab(float x) {
	float temp = (x/R_scaled);
	return 1-(temp*temp);
}


float parab2(float x) {
	return parab(x)*parab(x);
}

float parab_rev(float x) {
	float temp = (x/R_scaled);
	return (temp * temp);
}

float parab_rev2(float x) {
	return parab_rev(x) * parab_rev(x);
}

// round this also...?
vec2 map_to_dc(vec2 screenpos) {
	return vec2(screenpos.x / (0.5 * render_target_size.x) - 1, screenpos.y / (0.5 * render_target_size.y) - 1);
}

vec2 world2screen2(vec2 world) {
	vec2 d = player_pos - world;
	return vec2(d.y/(0.5*arena_size), -d.x/(0.5*arena_size));
}


float func(float x, float r) {
	return -x/r + 1;
}

out vec3 color;

void main() {
	const float FACTOR = 0.3;

  vec2 scr = world2screen2(world_pos);
  vec2 fr = map_to_dc(gl_FragCoord.xy);
  float d = length(fr - scr);
  float v = 1 - func(d, R_scaled);

  color = FACTOR*vec3(v, 0, 0);
}
