texture2D TEX : register(t0);

sampler s = sampler_state {
	Texture = <TEX>;
};

float4 main(float2 tex : TEXCOORD0) : COLOR {
	return tex2D(s, tex);
//	return float4(tex.x, tex.y, 0, 1);
}
