float4 main(float2 pos : POSITION) : POSITION {
	return float4(pos.x, pos.y, 0, 1);
}
