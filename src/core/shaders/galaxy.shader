/* The galaxy's glow, as seen from the observer: one texel of one cube face per fragment.
 *
 * Marched outward from the observer through the galaxy's density, adding the light of the
 * stars too far off to be drawn one by one and subtracting the dust in front of them.  Every
 * structure the sky shows comes out of the geometry: the arms are bright where the line of
 * sight runs along one, the rift is the dust layer seen edge on, the warp and the waves bend
 * the band off a great circle.
 *
 * The light is integrated from right beside the observer.  The stars drawn as points are the
 * few thousand brightest, a negligible share of the light of all the faint ones near by --
 * and that near light, from the disc within a kiloparsec or two, is what makes the band broad
 * and runs it right round the sky.  Leaving it out leaves a thin arc toward the centre and
 * nothing else: a galaxy seen from outside, not from within.
 *
 * Baked at the sky's own size, since the dust has structure down to tens of parsecs and a
 * sky baked smaller blurs its rift.  Done once per galaxy, not per nebula bake.
 */

#if defined(INCLUDE_VS)

void main()
{
	vec2 corner[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

	gl_Position = vec4(corner[gl_VertexID], 0.0, 1.0);
}

#endif

#if defined(INCLUDE_FS)

#define GAL_STEPS 512
#define GAL_MAX_EXTERNAL 8
/* Where the knee begins: a few times the light along the disc from near the Sun's radius. */
#define GAL_KNEE 1.5

uniform int u_Face;
uniform float u_FaceSize;
uniform vec3 u_Reddening;
/* The stars drawn as points (stars.cpp): the flux below which they are left to this glow,
 * the typical distance squared their luminosities are measured at, and the share of the
 * light kept as glow wherever they are drawn; 0 flux, none drawn. */
uniform vec3 u_Band;

uniform int u_ExternalCount;
uniform vec4 u_ExternalDir[GAL_MAX_EXTERNAL];	/* direction, angular radius */
uniform vec4 u_ExternalMajor[GAL_MAX_EXTERNAL];	/* long axis, minor over major */
uniform float u_ExternalBrightness[GAL_MAX_EXTERNAL];

out vec4 f_FragColor;

/* The share of the light of stars fainter than luminosity x: the luminosity function the
 * stars are drawn from, a power law of index 1.2 from 1 to 2000, weighed by luminosity. */
float fainter_light(float x)
{
	float top = pow(2000.0, -0.2);

	return clamp((1.0 - pow(max(x, 1.0), -0.2)) / (1.0 - top), 0.0, 1.0);
}

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

void main()
{
	vec2 st = gl_FragCoord.xy / u_FaceSize * 2.0 - 1.0;
	vec3 dir = normalize(face_direction(u_Face, st));
	/* The angle one texel subtends here, shrunk toward the face's edges and corners. */
	float pixel_angle = (2.0 / u_FaceSize) / (1.0 + dot(st, st));
	vec3 d = u_GalRot * dir;
	vec3 o = u_GalObserver;
	/* Everything, halo included, lies within a few disc edges of the centre. */
	float bound = 1.5 * u_GalEdge;
	float b = dot(o, d), c = dot(o, o) - bound * bound, disc = b * b - c;
	vec3 transmit = vec3(1.0), glow = vec3(0.0), bare = vec3(0.0), dimming;
	int i, k;

	if (disc > 0.0) {
		float t_end = -b + sqrt(disc);
		float t_start = 0.01;
		float ratio = t_end / t_start;

		/* Geometric steps: fine near the observer, where the disc's thickness is what
		 * decides everything, and long far off, where only the arms and the bulge are
		 * big enough to matter. */
		for (i = 0; i < GAL_STEPS && t_end > t_start; i++) {
			float ta = t_start * pow(ratio, float(i) / float(GAL_STEPS));
			float tb = t_start * pow(ratio, float(i + 1) / float(GAL_STEPS));
			float dt = tb - ta, t = 0.5 * (ta + tb);
			/* The dust's detail as fine as the texel is wide there, and no finer than
			 * half a step: finer than a step, a step lands in a cloud or misses it by
			 * chance, and the texels beside it by different chances -- grain. */
			gal_sample s = gal_density(o + d * t, max(t * pixel_angle, 0.5 * dt));
			float old_share = 1.0, young_share = 1.0;

			/* Only the stars too faint to be drawn as points glow: at this distance and
			 * behind this much dust, those fainter than the limit.  The young are four
			 * times as luminous, as the drawn ones are. */
			if (u_Band.x > 0.0) {
				float x = u_Band.x * t * t / (u_Band.y * max(transmit.g, 1e-6));

				/* But never all of it: the stars drawn are a budget, and a real sky has
				 * countless more too faint for any budget, which are its haze. */
				old_share = max(fainter_light(x), u_Band.z);
				young_share = max(fainter_light(0.25 * x), u_Band.z);
			}
			vec3 light = s.old * old_share * vec3(1.0, 0.86, 0.7) +
					s.young * young_share * vec3(0.7, 0.8, 1.0);

			glow += transmit * light * dt;
			bare += light * dt;
			transmit *= exp(-s.dust * dt * u_Reddening);
		}
	}

	/* Other galaxies: an exponential disc and a small bright core, seen at an angle.  They
	 * are behind the whole of this one, so its dust dims them -- which is why, like the
	 * real zone of avoidance, they are scarce along the band. */
	for (k = 0; k < u_ExternalCount; k++) {
		vec3 e = u_ExternalDir[k].xyz;
		vec3 major = u_ExternalMajor[k].xyz;
		float radius = u_ExternalDir[k].w;
		vec3 v = dir - e * dot(dir, e);
		float x, y, rr;

		if (dot(dir, e) <= 0.0)
			continue;
		x = dot(v, major) / radius;
		y = dot(v, cross(e, major)) / (radius * u_ExternalMajor[k].w);
		rr = sqrt(x * x + y * y);
		glow += transmit * u_ExternalBrightness[k] * vec3(1.0, 0.9, 0.8) *
			(exp(-4.0 * rr) + 0.6 * exp(-60.0 * rr * rr));
	}
	/* A soft knee.  From anywhere but the centre, the galaxy's centre is tens of times
	 * brighter than the rest of its band, which leaves a choice between a burnt-out bulge
	 * and a band that vanishes everywhere else.  The real Milky Way escapes it because
	 * visible light from its centre is almost wholly hidden by dust; compressing the bright
	 * end here stands in for that, and keeps the whole band on the sky.
	 *
	 * Compressed BEFORE the dust, then dimmed by it.  Compressing the dimmed light instead
	 * squeezes the bright star clouds either side of the rift down toward the rift's own
	 * level, and the rift, which should be the darkest thing in the band, comes out a pale
	 * brown stripe where its brightest part ought to be. */
	dimming = glow / max(bare, vec3(1e-6));
	bare /= 1.0 + dot(bare, vec3(0.2126, 0.7152, 0.0722)) / GAL_KNEE;
	glow = bare * dimming;
	f_FragColor = vec4(glow, transmit.g);
}

#endif
