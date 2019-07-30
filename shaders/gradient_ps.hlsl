uniform float4 centerpos : register(c0); // z component is radius

float func(float x, float r) {
	return -x/r + 1;
}

float4 main(float2 vpos : TEXCOORD0) : COLOR {
	float x = length(float2(centerpos.x, centerpos.y) - vpos);
	float dl = func(x, centerpos.z);
	return float4(dl*dl, 0, 0, 1);
}
