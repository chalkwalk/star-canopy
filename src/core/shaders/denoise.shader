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
 *
 * Last, the grade, which is where the sky gets its colour.  The physics gives each line its
 * own fixed hue, so the sky is two or three colours mixed in different amounts wherever it is
 * looked at, and its channels rise and fall together from the shadows to the highlights.
 * Every reference sky measured, sampled in bands of lightness, has one hue family instead,
 * and its colour moves with its lightness: saturated in the shadows and the
 * midtones, paling toward cream or white in the highlights.  So the colour of each texel is
 * moved toward a ramp indexed by its lightness AS DISPLAYED, through the display curve the
 * look was judged through, keeping its luminance: the grade changes hue and never brightness.
 *
 * Then a shoulder on the brightest channel.  Past the curve's knee a channel clips to white,
 * and a clipped highlight has lost its hue; eased toward a ceiling instead, every channel
 * scaled together, the brightest gas stays the colour it was graded.  Stars are drawn after
 * this and keep theirs.
 *
 * Dust gets a ramp of its own.  By lightness alone a dark cloud is just dark, and the ramp is
 * near grey at its dark end, so every dark cloud came out neutral black -- ink on the glow.
 * In the reference skies the dust is a different colour from the gas at the same lightness,
 * dark brown against green, its lit edges cream and orange.  The march says how much of each
 * texel's view the dust took, and the grade moves that far toward the dust's ramp.
 *
 * And the galaxy's light may be spared.  It is graded with the rest by default, the sky one
 * palette; with less, the band keeps some of its own colour: what the grade made of the
 * galaxy's light is taken back out and the light itself, as the march recorded it, put in.
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
uniform sampler2D u_GradeInfo;	/* per marched texel: dust's share of the view, the galaxy's light */
uniform float u_GradeGalaxy;	/* how far the galaxy is graded, 0..1 */
uniform int u_Supersample;	/* the march's texels per face texel, along each side */
uniform float u_Strength;	/* how different in brightness a neighbour may be, relative */
#define RAMP_STOPS 8		/* must match kRampStops */
uniform vec3 u_Ramp[RAMP_STOPS];	/* colour by displayed lightness, luminance 1 each */
uniform vec3 u_DustRamp[RAMP_STOPS];	/* the same, for dust */
uniform float u_Grade;		/* how far toward the ramp, 0..1 */
uniform float u_Shoulder;	/* the brightest channel's ceiling; 0 is none */
uniform float u_DisplayGain;	/* the gain the display curve is applied with */
uniform int u_Face;		/* which cube face this is */
/* A second palette over part of the sky: its ramp; the share of the sky it covers, 0 for none;
 * where a smooth field of three waves -- each a direction and a frequency in u_HueWave, a phase
 * in u_HuePhase -- passes u_Ramp2Threshold, over u_Ramp2Soft either side; and the displayed
 * lightness by which it has given way to the first. */
uniform vec3 u_Ramp2[RAMP_STOPS];
uniform float u_Ramp2Share;
uniform float u_Ramp2Threshold;
uniform float u_Ramp2Soft;
uniform float u_Ramp2Top;
uniform vec4 u_HueWave[3];
uniform vec3 u_HuePhase;

out vec4 f_FragColor;

float luma(vec3 c)
{
	return dot(c, vec3(0.2126, 0.7152, 0.0722));
}

/* Lightness as displayed, 0..1: the filmic curve the look was judged through -- Space Nerds
 * In Space's, where the sky was first made, at its gain of 1.18.  See look.h. */
float displayed(float y)
{
	float x = max(y - 0.004, 0.0);

	return clamp(u_DisplayGain * (x * (6.2 * x + 0.5)) / (x * (6.2 * x + 1.7) + 0.06), 0.0, 1.0);
}

/* The bake's face_direction(): the direction through (s, t) of face f. */
vec3 face_direction(int face, vec2 st)
{
	if (face == 0)
		return vec3(1.0, -st.y, -st.x);
	if (face == 1)
		return vec3(-1.0, -st.y, st.x);
	if (face == 2)
		return vec3(st.x, 1.0, st.y);
	if (face == 3)
		return vec3(st.x, -1.0, -st.y);
	if (face == 4)
		return vec3(st.x, -st.y, 1.0);
	return vec3(-st.x, -st.y, -1.0);
}

/* How much of the second palette in this direction, 0..1. */
float second_weight(void)
{
	vec2 st = gl_FragCoord.xy / vec2(textureSize(u_Marched, 0) / u_Supersample) * 2.0 - 1.0;
	vec3 dir = normalize(face_direction(u_Face, st));
	float s = 0.0;
	int k;

	for (k = 0; k < 3; k++)
		s += sin(u_HueWave[k].w * dot(dir, u_HueWave[k].xyz) + u_HuePhase[k]);
	s /= 1.8;
	return smoothstep(u_Ramp2Threshold - u_Ramp2Soft, u_Ramp2Threshold + u_Ramp2Soft, s);
}

vec3 grade(vec3 c, vec4 info)
{
	vec3 ramp;
	float dust = info.x;
	float y = luma(c), f, m, k;
	vec3 gas, dark;
	int i;

	if (u_Grade > 0.0 && y > 0.0) {
		/* The ramp's stops are the centres of equal bands of lightness. */
		f = clamp(displayed(y) * float(RAMP_STOPS) - 0.5, 0.0, float(RAMP_STOPS - 1));
		i = min(int(f), RAMP_STOPS - 2);
		gas = mix(u_Ramp[i], u_Ramp[i + 1], f - float(i));
		/* The second palette where the field puts it, giving way in the highlights -- and in
		 * the near black of the empty sky, which is the first's dark tint all over, so the
		 * edge of the field's region shows only in the gas: two objects of their own colour
		 * on one sky, not a sky cut in two. */
		if (u_Ramp2Share > 0.0)
			gas = mix(gas, mix(u_Ramp2[i], u_Ramp2[i + 1], f - float(i)), second_weight() *
					smoothstep(0.03, 0.12, displayed(y)) *
					(1.0 - smoothstep(u_Ramp2Top - 0.15, u_Ramp2Top, displayed(y))));
		dark = mix(u_DustRamp[i], u_DustRamp[i + 1], f - float(i));
		ramp = mix(gas, dark, clamp(dust, 0.0, 1.0));
		c = mix(c, y * ramp, u_Grade);
		/* The galaxy's light, back as its own colour, luminance kept. */
		c = max(c + u_Grade * (1.0 - u_GradeGalaxy) *
				(info.yzw - luma(info.yzw) * ramp), vec3(0.0));
	}
	m = max(c.r, max(c.g, c.b));
	k = 0.5 * u_Shoulder;
	if (u_Shoulder > 0.0 && m > k)
		c *= (k + (m - k) / (1.0 + (m - k) / (u_Shoulder - k))) / m;
	return c;
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

/* The same for what the grade needs to know. */
vec4 face_info(ivec2 p)
{
	vec4 sum = vec4(0.0);
	int x, y;

	for (y = 0; y < u_Supersample; y++)
		for (x = 0; x < u_Supersample; x++)
			sum += texelFetch(u_GradeInfo, p * u_Supersample + ivec2(x, y), 0);
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
	vec4 info = vec4(0.0);
	int x, y;

	if (u_Strength <= 0.0) {
		f_FragColor = vec4(grade(centre.rgb, face_info(at)), centre.a);
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
			info += face_info(p) * w;
			total += w;
		}
	}
	sum /= total;
	f_FragColor = vec4(grade(sum.rgb, info / total), sum.a);
}

#endif
