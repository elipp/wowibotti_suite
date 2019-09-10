#version 420 core

in gsout {
	vec2 center_pos;
	float radius;
} FSIN;


out vec3 color;

float sigmoid(float x) {
	return (1. / (1. + exp(-x)));
}

vec2 map_to_dc(vec2 screenpos) {
	return vec2(screenpos.x / 256. - 1, screenpos.y / 256. - 1);
}

float linear(float x) {
	return 1 - (x/FSIN.radius);
}

void main(){
  float d = length(map_to_dc(gl_FragCoord.xy) - FSIN.center_pos);
  color = vec3(linear(d), 0, 0);
}