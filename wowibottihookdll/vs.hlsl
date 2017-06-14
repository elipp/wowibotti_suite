float4 main(float2 position : POSITION) : POSITION0 {
	return float4(position.xy, 0, 1);
}
