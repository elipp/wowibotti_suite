#version 420 core

layout(location = 0) in vec3 pos;

// pos.z is actually radius


out float radius;

void main(){
	radius = pos.z;
	gl_Position = vec4(pos.xy, 0.0, 1.0);
}