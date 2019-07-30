struct VSInput {
	float2 pos : POSITION;
};

struct VSOutput {
	float4 pos : POSITION;
	float2 vpos : TEXCOORD0;
};

VSOutput main(in VSInput INPUT) {
	VSOutput o;
	o.pos = float4(INPUT.pos.x, INPUT.pos.y, 0, 1);
	o.vpos = INPUT.pos;

	return o;
}
