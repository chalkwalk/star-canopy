/* A cube face's march, averaged down and denoised before its stars are drawn on it.
 *
 * The march may be taken at a multiple of the face's resolution, several rays to a texel.
 * Each texel is then the plain mean of its block: a box average, not a filtered lookup,
 * since the point of the extra rays is to average what varies WITHIN a texel -- a lit front
 * that one ray catches and the next misses -- and any interpolation between texels would
 * only be sampling the field again.
 *
 * The march takes steps of random length, so that wherever it undersamples a thin structure
 * the error comes out as fine grain rather than as contour lines; this takes the grain out
 * again.  A 5x5 bilateral filter: neighbours weighed by distance and by how close their
 * brightness is, so the grain is averaged away while the edges of clouds and fronts, where
 * brightness changes sharply, are not blurred across.
 *
 * First, though, the texel is clamped to the range of its eight neighbours.  Much of the
 * grain is single texels, bright or dark, and a bilateral filter takes a lone outlier for an
 * edge and leaves it alone; the clamp removes exactly those and nothing wider.  At 2048 a face a texel is a twentieth
 * of a degree, so five of them is still finer than anything the eye can read as blur.
 *
 * Alpha, the transmittance, is filtered the same way.
 */

#if defined(INCLUDE_VS)

void main()
{
	vec2 corner[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

	gl_Position = vec4(corner[gl_VertexID], 0.0, 1.0);
}

#endif

#if defined(INCLUDE_FS)

uniform sampler2D u_Marched;
uniform int u_Supersample;	/* the march's texels per face texel, along each side */
uniform float u_Strength;	/* how different in brightness a neighbour may be, relative */

out vec4 f_FragColor;

float luma(vec3 c)
{
	return dot(c, vec3(0.2126, 0.7152, 0.0722));
}




/* Face texel p, as the mean of its block of marched texels. */
vec4 face_texel(ivec2 p)
{
	vec4 sum = vec4(0.0);
	int x, y;

	for (y = 0; y < u_Supersample; y++)
		for (x = 0; x < u_Supersample; x++)
			sum += texelFetch(u_Marched, p * u_Supersample + ivec2(x, y), 0);
	return sum / float(u_Supersample * u_Supersample);
}

void main()
{
	ivec2 at = ivec2(gl_FragCoord.xy);
	ivec2 size = textureSize(u_Marched, 0) / u_Supersample;
	vec4 centre = face_texel(at);
	vec4 lo = vec4(1e30), hi = vec4(-1e30);
	float l0;
	vec4 sum = vec4(0.0);
	float total = 0.0;
	int x, y;

	if (u_Strength <= 0.0) {
		f_FragColor = centre;
		return;
	}
	for (y = -1; y <= 1; y++) {
		for (x = -1; x <= 1; x++) {
			vec4 c;

			if (x == 0 && y == 0)
				continue;
			c = face_texel(clamp(at + ivec2(x, y), ivec2(0), size - 1));
			lo = min(lo, c);
			hi = max(hi, c);
		}
	}
	centre = clamp(centre, lo, hi);
	l0 = luma(centre.rgb);
	for (y = -2; y <= 2; y++) {
		for (x = -2; x <= 2; x++) {
			ivec2 p = clamp(at + ivec2(x, y), ivec2(0), size - 1);
			vec4 c = x == 0 && y == 0 ? centre : face_texel(p);
			/* Relative brightness difference, so the same filter serves the faint
			 * gas and the bright fronts alike. */
			float dl = (luma(c.rgb) - l0) / (max(l0, luma(c.rgb)) + 1e-3);
			float w = exp(-float(x * x + y * y) / 3.0) *
					exp(-dl * dl / (u_Strength * u_Strength));

			sum += c * w;
			total += w;
		}
	}
	sum /= total;
	f_FragColor = sum;
}

#endif
