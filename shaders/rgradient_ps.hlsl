uniform float4 centerpos : register(c0);

float4 main(float2 vpos : TEXCOORD0) : COLOR {
	float x = length(float2(centerpos.x, centerpos.y) - vpos);
	float dl = x;
	return float4(dl, 0, 0, 1);
}
