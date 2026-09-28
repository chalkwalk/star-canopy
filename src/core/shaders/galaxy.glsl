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
uniform vec3 u_GalDust;		/* amount, height, lognormal spread */
uniform vec3 u_GalWarp;		/* amplitude, start, phase */
uniform vec3 u_GalWaves;	/* amplitude, wavelength, phase */

/* The dust's fractal: GAL_DUST_OCTAVES octaves of noise from about 0.8 kpc down to about
 * 12 pc, each GAL_DUST_GAIN of the last, taken as a lognormal -- the column density
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

float gal_dust_field(vec3 p, float footprint)
{
	float f = 1.3, a = 1.0, sum = 0.0, kept = 0.0, scale = 1.0 / sqrt(0.073 * GAL_DUST_NORM);
	int k;

	for (k = 0; k < GAL_DUST_OCTAVES; k++) {
		float w = clamp(1.0 / (f * 2.0 * footprint) - 1.0, 0.0, 1.0);

		if (w > 0.0) {
			vec4 n = nsky_noise(p * f + float(k) * 7.31);

			float v = n[k - 4 * (k / 4)];

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
	float zmid, h, dz, radial, psi, arm, hy, dh, clump;
	gal_sample s;

	zmid = u_GalWarp.x * pow(max(r_disc - u_GalWarp.y, 0.0), 2.0) * sin(phi - u_GalWarp.z);
	zmid += u_GalWaves.x * sin(6.2831853 * r_disc / u_GalWaves.y + u_GalWaves.z) *
			smoothstep(u_GalWarp.y - 3.0, u_GalWarp.y + 1.0, r_disc) *
			cos(phi - 0.5 * u_GalWaves.z);
	h = min(u_GalDisc.y * exp(max(r_disc - u_GalDisc.z, 0.0) / u_GalDisc.w), 2.5);
	dz = p.z - zmid;
	radial = exp(-r_disc / u_GalDisc.x) * (1.0 - smoothstep(0.85 * u_GalEdge, u_GalEdge, r_disc));

	psi = u_GalArms.x * (phi - log(max(r_disc, 0.5)) / u_GalArms.y) + u_GalArms.z;
	arm = pow(0.5 + 0.5 * cos(psi), u_GalArmShape.x);
	if (u_GalArmShape.y > 0.0) {
		clump = clamp(0.5 + 1.5 * nsky_noise(p * 0.7).x, 0.0, 1.6);
		arm *= 1.0 + u_GalArmShape.y * (clump - 1.0);
	}
	if (u_GalBar.y > 0.0)
		arm *= smoothstep(0.8 * u_GalBar.y, 1.3 * u_GalBar.y, r_disc);

	/* Star clouds: the disc's light is lumpy on the scale of a kiloparsec, which is what
	 * mottles a band seen from inside; the young stars more so. */
	clump = clamp(0.6 + 0.9 * nsky_noise(p * 1.1 + 3.0).z, 0.2, 1.6);
	s.old = radial * exp(-(dz * dz) / (h * h)) / h * (1.0 + 0.6 * u_GalArms.w * arm) * clump;
	hy = 0.3 * h;
	s.young = radial * exp(-(dz * dz) / (hy * hy)) / hy * u_GalArms.w * arm * 0.25 *
			clump * clump;

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
	s.old += u_GalBar.w * 0.45 * exp(-length(vec3(p.xy, p.z * 2.0)) / 1.0);
	s.old += 0.0008 / pow(1.0 + r * r / 4.0, 1.5);

	/* The dust layer, thin, but not flat: its thickness varies by a low noise, a factor
	 * of two or so either way, and its middle rides up and down by a second, so the clouds
	 * near the observer stand out of the plane and are seen above and below the band. */
	dh = u_GalDust.y * h / u_GalDisc.y * exp(1.2 * nsky_noise(p * 0.9 + 5.0).x);
	dz -= 0.3 * nsky_noise(p * 0.5 + 9.0).y;
	/* 0.4: the old two-octave clump's mean, so the dust's amount is as it was. */
	s.dust = u_GalDust.x * 0.4 * exp(-r_disc / (1.5 * u_GalDisc.x)) *
			(1.0 - smoothstep(0.8 * u_GalEdge, u_GalEdge, r_disc)) *
			exp(-(dz * dz) / (dh * dh)) * (0.3 + 1.2 * arm) * gal_dust_field(p, footprint);
	return s;
}
