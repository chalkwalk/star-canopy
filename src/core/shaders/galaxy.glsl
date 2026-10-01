/* The galaxy's density, for the glow pass.  Needs field.glsl in front of it for the noise.
 * A transcription of galaxyDensity() in galaxy.cpp, which the star sampling uses; the two
 * must be kept in step, and test_galaxy checks they are.  See galaxy.h for what
 * the terms are and why.  Kiloparsecs, in the galaxy's frame.
 */

uniform mat3 u_GalRot;		/* sky direction -> galaxy frame */
uniform vec3 u_GalObserver;
uniform vec4 u_GalDisc;		/* scale length, scale height, flare start, flare length */
uniform float u_GalEdge;
uniform vec4 u_GalArms;		/* count, tan(pitch), phase, strength */
uniform vec2 u_GalArmShape;	/* sharpness, flocculence */
uniform vec4 u_GalBar;		/* angle, length, strength, bulge strength */
uniform vec3 u_GalBulge;	/* radius, flattening; with rings, the dust elsewhere */
uniform vec4 u_GalLens;		/* radius, strength, inner ring, outer ring */
uniform vec4 u_GalDust;		/* amount, height, lognormal spread, ring radius */
uniform vec3 u_GalWarp;		/* amplitude, start, phase */
uniform vec3 u_GalWaves;	/* amplitude, wavelength, phase */

/* The dust's fractal: GAL_DUST_OCTAVES octaves of noise from about 0.4 kpc down to about
 * 6 pc -- the larger forms are the arms' dust lanes, not noise -- each GAL_DUST_GAIN of the last, taken as a lognormal -- the column density
 * interstellar gas has: most of it thin, a few clouds dense -- with a spread drawn by
 * seed (u_GalDust.z).  Octaves finer than twice the footprint fade out, as the nebula's detail
 * does, and the lognormal's mean is corrected for the octaves kept, so a coarse sample has as
 * much dust as a fine one, only smoother: a small sky is the same sky seen more coarsely.
 * 0.073 is the single noise's variance, measured; GAL_DUST_NORM the octaves' amplitudes
 * squared, summed.  The CPU twin is galaxyDust() in galaxy.cpp. */
#define GAL_DUST_OCTAVES 7
#define GAL_DUST_GAIN 0.75
#define GAL_DUST_NORM 2.2448
#define GAL_DUST_RIDGED 2

/* pow() is undefined for a negative base, which these squares have, and a cos a hair
 * below -1 makes one too; a driver may give NaN. */
float gal_sq(float x)
{
	return x * x;
}

/* One component of a vec4, chosen without indexing it by a variable: some drivers (Mesa 21's
 * for Intel) compiled n[k % 4] in the dust's loop wrongly, and the dust came out several
 * times what the CPU twin makes. */
float gal_component(vec4 n, int i)
{
	return i == 0 ? n.x : i == 1 ? n.y : i == 2 ? n.z : n.w;
}

float gal_dust_field(vec3 p, float footprint)
{
	float f = 2.6, a = 1.0, sum = 0.0, kept = 0.0, scale = 1.0 / sqrt(0.073 * GAL_DUST_NORM);
	int k;

	for (k = 0; k < GAL_DUST_OCTAVES; k++) {
		/* A footprint of 0 is full detail; dividing by it is undefined in GLSL, and Mesa 21
		 * took it for none. */
		float w = clamp(1.0 / max(f * 2.0 * footprint, 1e-6) - 1.0, 0.0, 1.0);

		if (w > 0.0) {
			vec4 n = nsky_noise(p * f + float(k) * 7.31);

			float v = gal_component(n, k - 4 * (k / 4));

			/* The finer octaves ridged, folded where the noise crosses zero and
			 * centred again (0.219 is the mean of |n|, and 1.72 scales it to the smooth
			 * octaves' spread): dust is filaments and sheets with edges, and smooth
			 * noise alone makes it round soft blobs. */
			if (k >= GAL_DUST_RIDGED)
				v = (0.219 - abs(v)) * 1.72;
			sum += a * w * v;
			kept += a * a * w * w;
		}
		a *= GAL_DUST_GAIN;
		f *= 2.07;
	}
	return exp(u_GalDust.z * sum * scale - 0.5 * u_GalDust.z * u_GalDust.z * kept / GAL_DUST_NORM);
}

struct gal_sample {
	float old;
	float young;
	float dust;
};

/* footprint: the size of the sample, kpc, below which the dust's detail fades out. */
gal_sample gal_density(vec3 p, float footprint)
{
	float r_disc = length(p.xy);
	float r = length(p);
	float phi = atan(p.y, p.x);
	float zmid, h, dz, radial, psi, arm, lane, hy, dh, clump, knot, width, segment, outer;
	gal_sample s;

	zmid = u_GalWarp.x * gal_sq(max(r_disc - u_GalWarp.y, 0.0)) * sin(phi - u_GalWarp.z);
	zmid += u_GalWaves.x * sin(6.2831853 * r_disc / u_GalWaves.y + u_GalWaves.z) *
			smoothstep(u_GalWarp.y - 3.0, u_GalWarp.y + 1.0, r_disc) *
			cos(phi - 0.5 * u_GalWaves.z);
	h = min(u_GalDisc.y * exp(max(r_disc - u_GalDisc.z, 0.0) / u_GalDisc.w), 2.5);
	dz = p.z - zmid;
	radial = exp(-r_disc / u_GalDisc.x) * (1.0 - smoothstep(0.85 * u_GalEdge, u_GalEdge, r_disc));

	/* An arm is not the same all along: it meanders a little off its spiral, it widens and
	 * narrows, and its light comes in segments a few kpc long, its star-forming complexes;
	 * toward its outer end it breaks into fragments.  Uniform, the arms were flat painted
	 * strips. */
	psi = u_GalArms.x * (phi - log(max(r_disc, 0.5)) / u_GalArms.y) + u_GalArms.z +
			0.3 * nsky_noise(p * 0.3 + 23.0).z;
	width = u_GalArmShape.x * exp(0.5 * nsky_noise(p * 0.4 + 17.0).x);
	arm = pow(max(0.5 + 0.5 * cos(psi), 0.0), width);
	/* The dust lane on the arm's inner edge, where the gas is compressed as it enters the
	 * arm: half a radian upstream, which in a trailing spiral is toward the centre, and
	 * narrower than the arm. */
	lane = pow(max(0.5 + 0.5 * cos(psi - 0.5), 0.0), 1.5 * width);
	segment = clamp(1.0 + 1.2 * nsky_noise(p * 0.35 + 11.0).y, 0.5, 1.5);
	outer = smoothstep(2.0 * u_GalDisc.x, 4.0 * u_GalDisc.x, r_disc);
	segment *= 1.0 + outer * (clamp(0.3 + 2.5 * nsky_noise(p * 0.9 + 29.0).w, 0.0, 1.8) - 1.0);
	arm *= segment;
	lane *= 0.5 + 0.5 * segment;
	if (u_GalArmShape.y > 0.0) {
		clump = clamp(0.5 + 1.5 * nsky_noise(p * 0.7).x, 0.0, 1.6);
		arm *= 1.0 + u_GalArmShape.y * (clump - 1.0);
		lane *= 1.0 + u_GalArmShape.y * (clump - 1.0);
	}
	if (u_GalBar.y > 0.0) {
		arm *= smoothstep(0.8 * u_GalBar.y, 1.3 * u_GalBar.y, r_disc);
		lane *= smoothstep(0.8 * u_GalBar.y, 1.3 * u_GalBar.y, r_disc);
	}

	/* The old disc is smooth -- a little lumpy, a fifth either way at a kiloparsec, not
	 * the eightfold star clouds of old, which from above laid a field of moguls over the
	 * whole disc and hid its arms. */
	clump = clamp(1.0 + 0.8 * nsky_noise(p * 1.1 + 3.0).z, 0.7, 1.3);
	/* A lenticular's smoother still: nothing has stirred it for billions of years. */
	clump = mix(1.0, clump, min(4.0 * u_GalArms.w, 1.0));
	/* The arms: the old disc brighter along them, and the young stars -- blue, and gathered
	 * into star clouds -- nearly all in them: strong enough to be seen, where the labs'
	 * arms, at a sixth of the old light, showed from nowhere (docs/studies/atlas.md). */
	/* And between them, in the outer disc, dead space: the old light there falls to a third,
	 * so the arms stand apart as strands, as they do in every photograph of a spiral. The
	 * inner disc keeps its light; a disc without arms, a lenticular's, keeps all of it. */
	s.old = radial * exp(-(dz * dz) / (h * h)) / h * clump *
			mix(1.0 + 1.2 * u_GalArms.w * arm, 0.3 + 2.4 * u_GalArms.w * arm,
			    outer * min(u_GalArms.w, 1.0));
	hy = 0.3 * h;
	/* The young stars in knots along the arms, some 250 pc across: the star-forming regions
	 * that bead a real spiral's arms. */
	knot = clamp(0.4 + 2.2 * nsky_noise(p * 4.0 + 7.0).w, 0.0, 2.5);
	/* Their light, sharper than the old stars' arms, draws the spiral; and it fades more
	 * slowly outward than the old disc, so the arms reach past the disc's glow into the
	 * dark, as a photograph's blue arms do. */
	s.young = exp(-r_disc / (1.6 * u_GalDisc.x)) *
			(1.0 - smoothstep(0.85 * u_GalEdge, u_GalEdge, r_disc)) *
			exp(-(dz * dz) / (hy * hy)) / hy * u_GalArms.w * pow(arm, 1.5) * 1.2 * knot * knot;

	if (u_GalBar.z > 0.0) {
		float c = cos(u_GalBar.x), sn = sin(u_GalBar.x);
		float qx = p.x * c + p.y * sn;
		float qy = -p.x * sn + p.y * c;
		float l2 = u_GalBar.y * u_GalBar.y;
		float rb2 = qx * qx / l2 + qy * qy / (0.1225 * l2) + p.z * p.z / (0.0625 * l2);

		s.old += u_GalBar.z * 0.5 * exp(-2.5 * rb2);
	}
	/* The bulge: flattened, half as deep as it is wide, as the Milky Way's boxy one is --
	 * round, it stood above and below the band as a ball of light, which no photograph
	 * of the Milky Way shows. */
	s.old += u_GalBar.w * 0.45 * exp(-length(vec3(p.xy, p.z * u_GalBulge.y)) / u_GalBulge.x);
	/* A lenticular's lens: a plateau of old light, nearly even, ending in a sharp edge; and
	 * its stellar rings, at the lens's inner edge and twice that (docs/studies/atlas.md). */
	if (u_GalLens.x > 0.0) {
		float ri = u_GalLens.x / 1.3, ro = 2.0 * ri;
		float vert = exp(-(dz * dz) / (h * h)) / h;

		s.old += vert * (u_GalLens.y * exp(-r_disc / (4.0 * u_GalDisc.x)) *
				(1.0 - smoothstep(0.92 * u_GalLens.x, u_GalLens.x, r_disc)) +
				radial * (u_GalLens.z * exp(-gal_sq((r_disc - ri) / (0.08 * ri))) +
				u_GalLens.w * exp(-gal_sq((r_disc - ro) / (0.1 * ro)))));
	}
	s.old += 0.0008 / pow(1.0 + r * r / 4.0, 1.5);

	/* The dust layer, thin, but not flat: its thickness varies by a low noise, a factor
	 * of two or so either way, and its middle rides up and down by a second, so the clouds
	 * near the observer stand out of the plane and are seen above and below the band. */
	dh = u_GalDust.y * h / u_GalDisc.y * exp(1.2 * nsky_noise(p * 0.9 + 5.0).x);
	dz -= 0.3 * nsky_noise(p * 0.5 + 9.0).y;
	/* 0.4: the old two-octave clump's mean, so the dust's amount is as it was. */
	s.dust = u_GalDust.x * 0.4 * exp(-r_disc / (1.5 * u_GalDisc.x)) *
			(1.0 - smoothstep(0.8 * u_GalEdge, u_GalEdge, r_disc)) *
			exp(-(dz * dz) / (dh * dh)) * (0.25 + 1.4 * u_GalArms.w * lane) *
			gal_dust_field(p, footprint);
	/* A lenticular's dust, what there is of it, in thin rings about the centre, and in half of
	 * them a lane along the disc too. */
	if (u_GalDust.w > 0.0) {
		float r1 = u_GalDust.w, r2 = 1.7 * u_GalDust.w;

		s.dust *= u_GalBulge.z + 4.0 * (exp(-gal_sq((r_disc - r1) / (0.12 * r1))) +
				0.6 * exp(-gal_sq((r_disc - r2) / (0.1 * r2))));
	}
	return s;
}
