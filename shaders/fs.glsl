#version 420 core

in gsout {
	vec2 center_pos;
	float radius;
} FSIN;

uniform vec2 target_size;

out vec3 color;

float sigmoid(float x) {
	return (1. / (1. + exp(-x)));
}

vec2 map_to_dc(vec2 screenpos) {
	return vec2(screenpos.x / (0.5 * target_size.x) - 1, screenpos.y / (0.5 * target_size.y) - 1);
}

float linear(float x) {
	return 1 - (x/FSIN.radius);
}

float circular(float x) {
	return sqrt(FSIN.radius * FSIN.radius - x*x)/FSIN.radius;
}

float parab(float x) {
	float temp = (x/FSIN.radius);
	return 1-(temp*temp);
}

float parab2(float x) {
	return parab(x)*parab(x);
}

void main(){
  float d = length(map_to_dc(gl_FragCoord.xy) - FSIN.center_pos);
  color = vec3(parab2(d), 0, 0);
}