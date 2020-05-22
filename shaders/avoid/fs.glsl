#version 420 core

in gsout {
	vec2 center_pos;
	float radius;
} FS_IN;

uniform vec2 render_target_size; // as in "render target" size

#define RADIUS (FS_IN.radius)

out vec3 color;

vec2 map_to_dc(vec2 screenpos) {
	return vec2(screenpos.x / (0.5 * render_target_size.x) - 1, screenpos.y / (0.5 * render_target_size.y) - 1);
}

// different options for falloff

float sigmoid(float x) {
	return (1. / (1. + exp(-x)));
}

float linear(float x) {
	return 1 - x / RADIUS;
}

float circular(float x) {
	return sqrt(RADIUS * RADIUS - x*x)/RADIUS;
}

float parab(float x) {
	float t = (x / RADIUS);
	return 1 - (t*t);
}

float parab2(float x) {
	return parab(x)*parab(x);
}

float constant(float x) {
	return 1.0;
}


void main(){
  const float FACTOR = 1;

  float d = length(map_to_dc(gl_FragCoord.xy) - FS_IN.center_pos);
  float v = constant(d); // FS_in.radius has been scaled in vertex shader
  color = FACTOR*vec3(v, 0, 0);
}
