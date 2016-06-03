// Kernel converts floatbuffer to uint color buffer and sets color

uint interpolateColor(uint c1, uint c2, float m)
{
	int channelC1;
	int channelC2;
	// alpha
	channelC1 = (c1 >> 24) & 0xff;
	channelC2 = (c2 >> 24) & 0xff;
	int a = (int)(channelC1 + m * (channelC2 - channelC1));
	// red
	channelC1 = (c1 >> 16) & 0xff;
	channelC2 = (c2 >> 16) & 0xff;
	int r = (int)(channelC1 + m * (channelC2 - channelC1));
	// green
	channelC1 = (c1 >> 8) & 0xff;
	channelC2 = (c2 >> 8) & 0xff;
	int g = (int)(channelC1 + m * (channelC2 - channelC1));
	// blue
	channelC1 = c1 & 0xff;
	channelC2 = c2 & 0xff;
	int b = (int)(channelC1 + m * (channelC2 - channelC1));

	return (a << 24) | (r << 16) | (g << 8) | b; 
}

uint getColor(float value, __constant float* keys, __constant uint* colors, int numKeys, uint nanColor)
{
	if(isnan(value)){
		return nanColor;
	} else if(numKeys == 1){
		return colors[0];
	} else {
		int i = -1;
		while(i+1 < numKeys && keys[i+1] < value){
			i++;
		}

		if(i < 0){
			return colors[0];
		} else if(i+1 == numKeys){
			return colors[i];
		} else {
			// interpolate
			float m = (value - keys[i])/(keys[i+1] - keys[i]);
			return interpolateColor(colors[i], colors[i+1], m);
		}
	}
}


__kernel void colorize( 
            __global float* input,
			__global uint* output,
			__constant float* keys,
			__constant uint* colors,
			int numKeys,
			uint nanColor) 
{
/* - - - - - - - - - - - - - - - - - */
    int i = get_global_id(0);
	output[i] = getColor(input[i], keys, colors, numKeys, nanColor);
}


__kernel void colorize_(
			__global float* input,
			__global uint* output)
{
/* - - - - - - - - - - - - - - - - - */
    int i = get_global_id(0);
	float value = input[i];
	if(isnan(value)){
		output[i] = 0;
	} else {
		uint gray = ((uint)(255*value)) & 255;
		output[i] = (255 << 24) | (gray << 16) | (gray << 8) | gray;
	}
}




// Local Variables:
// mode: c
// tab-width: 4
// c-basic-offset: 4
// End:
