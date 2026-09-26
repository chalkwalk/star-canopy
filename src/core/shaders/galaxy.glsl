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
uniform vec2 u_GalDust;		/* amount, height */
uniform vec3 u_GalWarp;		/* amplitude, start, phase */
uniform vec3 u_GalWaves;	/* amplitude, wavelength, phase */

struct gal_sample {
	float old;
	float young;
	float dust;
};

gal_sample gal_density(vec3 p)
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
	s.old += u_GalBar.w * 0.6 * exp(-r / 1.0);
	s.old += 0.0008 / pow(1.0 + r * r / 4.0, 1.5);

	/* Two octaves, the finer one strong, so the dust breaks into lanes and filaments rather
	 * than lying in soft sheets. */
	clump = clamp(0.3 + 1.0 * nsky_noise(p * 1.3).y + 0.9 * nsky_noise(p * 4.1 + 11.0).w,
			0.0, 2.5);
	dh = u_GalDust.y * h / u_GalDisc.y;
	s.dust = u_GalDust.x * exp(-r_disc / (1.5 * u_GalDisc.x)) *
			(1.0 - smoothstep(0.8 * u_GalEdge, u_GalEdge, r_disc)) *
			exp(-(dz * dz) / (dh * dh)) * (0.3 + 1.2 * arm) * clump;
	return s;
}
