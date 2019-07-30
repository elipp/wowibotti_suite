texture2D TEX : register(t0);

sampler s = sampler_state {
	Texture = <TEX>;
//	MinFilter = None;
//	MaxFilter = None;
//	MipFilter = None;
};

float4 main(float2 tex : TEXCOORD0) : COLOR {
	return tex2D(s, tex);
}
