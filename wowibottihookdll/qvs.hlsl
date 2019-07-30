struct VSInput {
	float2 pos : POSITION;
	float2 uv : TEXCOORD0;
};

struct VSOutput {
	float4 pos : POSITION;
	float2 uv : TEXCOORD0;
};

VSOutput main(in VSInput INPUT) {
	VSOutput o;
	o.pos = float4(INPUT.pos.x, INPUT.pos.y, 0, 1);
	o.uv = INPUT.uv;

	return o;
}
