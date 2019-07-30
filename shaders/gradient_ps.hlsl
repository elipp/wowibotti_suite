uniform float4 centerpos : register(c0);

float4 main(float2 vpos : TEXCOORD0) : COLOR {
	float x = length(float2(centerpos.x, centerpos.y) - vpos);
	float dl = 0.5*(cos(4 * x) + 1);
	return float4(dl, 0, 0, 1);
}
