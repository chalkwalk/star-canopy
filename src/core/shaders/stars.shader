/* The sky's stars, drawn into one cube face after the march has finished it.
 *
 * Each star is a small quad centred on its direction, sized by how far its light spreads,
 * and added onto the face in HDR.  Quads rather than GL points because a point is clipped
 * whole when its centre leaves the viewport, so a star's halo would be cut off at every
 * cube edge; a quad drawn into each face it touches is clipped per fragment, and a star on
 * an edge lands correctly on both.
 *
 * What makes these part of the nebula rather than a layer under it: each star looks up the
 * optical depth of gas between the viewer and ITS OWN DISTANCE, from the depths the march
 * recorded for its texel.  A star in front of the near wall is untouched; one behind a lane
 * is gone; one seen through a thin edge is dimmed and reddened by exactly that edge.
 *
 * The grade does not reach the stars -- they are drawn after it -- so each is moved toward
 * the grade's colour here instead, keeping its brightness, and only part of the way.  Every
 * reference sky grades its stars with its gas: their hues gather at the palette's.  But not
 * all the way, or every star is one colour and the field is a flat tint; what is left of each
 * star's own colour is what tells a hot star from a cool one.
 */

#define NSKY_TAU_LAYERS 8

#if defined(INCLUDE_VS)

uniform int u_Face;
uniform float u_FaceSize;
uniform sampler2D u_Tau0;	/* optical depth at the first four recorded distances */
uniform sampler2D u_Tau1;	/* the next three, then the whole line of sight */
uniform float u_TauDepth[NSKY_TAU_LAYERS - 1];
uniform vec3 u_Reddening;	/* relative extinction per channel */
uniform float u_Brightness;
uniform float u_Cutoff;		/* radiance below which a halo is not worth drawing */
uniform float u_HaloAngle;	/* radians */
uniform float u_Halo;		/* fraction of a star's light in its halo */
uniform float u_SpikeFlux;	/* stars above this get diffraction spikes */

in vec3 a_Dir;
in float a_Distance;
in vec3 a_Flux;

out vec2 v_Offset;		/* texels from the star's centre */
out vec3 v_Flux;		/* after extinction */
out float v_PixelAngle;
out float v_Spike;

/* Optical depth out to distance d along this texel, between the recorded layers.
 * Interpolated in log distance, since the layers are spaced geometrically. */
float tau_at(vec2 uv, float d)
{
	vec4 a = texture(u_Tau0, uv);
	vec4 b = texture(u_Tau1, uv);
	float tau[NSKY_TAU_LAYERS] = float[NSKY_TAU_LAYERS](a.x, a.y, a.z, a.w, b.x, b.y, b.z, b.w);
	int i;

	if (d <= u_TauDepth[0])
		return tau[0] * d / u_TauDepth[0];
	for (i = 1; i < NSKY_TAU_LAYERS - 1; i++)
		if (d < u_TauDepth[i])
			return mix(tau[i - 1], tau[i], log(d / u_TauDepth[i - 1]) /
					log(u_TauDepth[i] / u_TauDepth[i - 1]));
	/* Past the last recorded depth, whatever the whole line of sight holds. */
	return tau[NSKY_TAU_LAYERS - 1];
}

void main()
{
	/* Two triangles per star, the corner from the vertex's place in its six. */
	vec2 corner[6] = vec2[6](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
				vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));
	vec3 d = a_Dir;
	float ma, sc, tc, pixel_angle, lum, radius, tau;
	vec2 st;

	/* The inverse of the bake's face_direction(): which way this face looks, and where on
	 * it the star falls, per the GL cube map table. */
	if (u_Face == 0) {
		ma = d.x; sc = -d.z; tc = -d.y;
	} else if (u_Face == 1) {
		ma = -d.x; sc = d.z; tc = -d.y;
	} else if (u_Face == 2) {
		ma = d.y; sc = d.x; tc = d.z;
	} else if (u_Face == 3) {
		ma = -d.y; sc = d.x; tc = -d.z;
	} else if (u_Face == 4) {
		ma = d.z; sc = d.x; tc = -d.y;
	} else {
		ma = -d.z; sc = -d.x; tc = -d.y;
	}
	/* Behind this face, or so near its plane that it cannot reach into it: drop it. */
	if (ma < 0.3) {
		gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
		v_Offset = vec2(0.0);
		v_Flux = vec3(0.0);
		v_PixelAngle = 1.0;
		v_Spike = 0.0;
		return;
	}
	st = vec2(sc, tc) / ma;
	pixel_angle = (2.0 / u_FaceSize) / (1.0 + dot(st, st));

	/* The galaxy's own dust is already in a_Flux, worked out when the star was placed. */
	tau = tau_at(clamp(st * 0.5 + 0.5, 0.0, 1.0), a_Distance);
	v_Flux = a_Flux * u_Brightness * exp(-tau * u_Reddening);

	/* How far out the light is worth drawing: the core's three sigma, or the radius at
	 * which the halo's far wing, falling as the fourth power, drops under the cutoff. */
	lum = dot(v_Flux, vec3(0.2126, 0.7152, 0.0722));
	radius = max(2.0, pow(max(lum * u_Halo * u_HaloAngle * u_HaloAngle /
				(3.1415927 * u_Cutoff), 0.0), 0.25) / pixel_angle);
	v_Spike = smoothstep(u_SpikeFlux, u_SpikeFlux * 4.0, lum);
	if (v_Spike > 0.0)
		radius = max(radius, 24.0 * v_Spike);
	radius = min(radius, 96.0);

	v_Offset = corner[gl_VertexID % 6] * radius;
	v_PixelAngle = pixel_angle;
	gl_Position = vec4(st + v_Offset * (2.0 / u_FaceSize), 0.0, 1.0);
}

#endif

#if defined(INCLUDE_FS)

uniform float u_HaloAngle;
uniform float u_Halo;
uniform float u_Spike;		/* fraction of a spiked star's light in its spikes */

in vec2 v_Offset;
in vec3 v_Flux;
in float v_PixelAngle;
in float v_Spike;

out vec4 f_FragColor;

void main()
{
	/* Core: a gaussian a little over a texel wide -- the smallest a star can be and still
	 * land on the same total however it straddles texels.  Everything is normalised per
	 * unit solid angle, so the sum over the texels is the star's flux at any resolution. */
	const float sigma = 0.6;
	float r2 = dot(v_Offset, v_Offset);
	float texel = v_PixelAngle * v_PixelAngle;
	float theta2 = r2 * texel;
	float a2 = u_HaloAngle * u_HaloAngle;
	float spike = u_Spike * v_Spike;
	float radiance;

	radiance = (1.0 - u_Halo - spike) * exp(-r2 / (2.0 * sigma * sigma)) /
			(6.2831853 * sigma * sigma * texel);
	radiance += u_Halo / (3.1415927 * a2) / ((1.0 + theta2 / a2) * (1.0 + theta2 / a2));
	if (spike > 0.0) {
		/* Four thin rays along the face's axes, each a quarter of the spike light, one
		 * texel wide and fading out over a dozen.  Only on the brightest few, and faint:
		 * a hint of optics, not a starburst. */
		vec2 a = abs(v_Offset);
		float along = exp(-a.x / 8.0) * exp(-a.y * a.y / 0.5) +
				exp(-a.y / 8.0) * exp(-a.x * a.x / 0.5);

		radiance += spike * along / (4.0 * 8.0 * 1.2533 * texel);
	}
	f_FragColor = vec4(v_Flux * radiance, 0.0);
}

#endif
