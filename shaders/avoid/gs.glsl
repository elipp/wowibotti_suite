#version 420 core

#define M_PI 3.1415926535897932384626433832795
#define TWOPI (M_PI * 2.0)

#define NUM_TRIANGLES 36
#define NVERT (NUM_TRIANGLES*3)

layout(points) in;
layout(triangle_strip, max_vertices = 256) out;

in float radius[];
in int falloff_gs[];

out gsout {
	vec2 center_pos;
	float radius;
	flat int falloff;
} GSOUT;

void main(){

	vec4 pp = gl_in[0].gl_Position;

	for (int i = 0; i < NUM_TRIANGLES; ++i) {
		gl_Position = pp;
		GSOUT.center_pos = pp.xy;
		GSOUT.radius = radius[0];
		GSOUT.falloff = falloff_gs[0];
		EmitVertex();
		
		float a1 = (TWOPI / float(NUM_TRIANGLES)) * i;
		vec4 d1 = radius[0] * vec4(cos(a1), sin(a1), 0, 0);
		gl_Position = pp + d1;
		GSOUT.center_pos = pp.xy;
		GSOUT.radius = radius[0];
		GSOUT.falloff = falloff_gs[0];
		EmitVertex();
		
		float a2 = (TWOPI / float(NUM_TRIANGLES)) * (i + 1);
		vec4 d2 = radius[0] * vec4(cos(a2), sin(a2), 0, 0);
		gl_Position = pp + d2;
		GSOUT.center_pos = pp.xy;
		GSOUT.radius = radius[0]; 
		GSOUT.falloff = falloff_gs[0];
		EmitVertex();
		
		EndPrimitive();
	}
}