#version 420 core

in gsout {
	vec2 center_pos;
	float radius;
} FS_IN;

uniform vec2 render_target_size; // as in "render target" size

out vec3 color;

vec2 map_to_dc(vec2 screenpos) {
	return vec2(screenpos.x / (0.5 * render_target_size.x) - 1, screenpos.y / (0.5 * render_target_size.y) - 1);
}

// different options for falloff

float sigmoid(float x) {
	return (1. / (1. + exp(-x)));
}

float linear(float x) {
	return 1 - (x/FS_IN.radius);
}

float circular(float x) {
	return sqrt(FS_IN.radius * FS_IN.radius - x*x)/FS_IN.radius;
}

float parab(float x) {
	float temp = (x/FS_IN.radius);
	return 1-(temp*temp);
}

float parab2(float x) {
	return parab(x)*parab(x);
}

float func(float x, float r) {
	return -x/r + 1;
}

void main(){
	const float FACTOR = 0.3;

  float d = length(map_to_dc(gl_FragCoord.xy) - FS_IN.center_pos);
  float v = func(d, FS_IN.radius); // FS_in.radius has been scaled in vertex shader
  color = FACTOR*vec3(v, v, v);
}
