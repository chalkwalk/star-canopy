/* The sky bake: one texel of one cube face per fragment, marched outward through every
 * bubble the direction passes, over the galaxy's glow.
 *
 * Drawn once per face per bake, a tile at a time, into a half float texture that is then
 * denoised into the cubemap -- the whole cost is paid when the sky is made, none of it when
 * a game draws it.
 *
 * THE LIGHT.  Each sample is lit by its bubble's cluster, through the optical depth the light
 * volume holds for that point.  What it emits follows from that depth, as a real HII region's
 * does:
 *
 *	[O III]	needs the hardest photons, which the gas uses up first -- so it lives only
 *		in the thinnest, most exposed gas nearest the stars
 *	H-alpha	the bulk of the ionised gas, out to the ionisation front
 *	[S II]	strongest in the skin just behind the front, where ionisation is partial
 *
 * so the stratification of colour -- teal inside, red at the rims -- is not painted on; it is
 * where the photons run out.  The dust meanwhile scatters the stars' visible light forward
 * (reflection nebulosity, blue) and absorbs everything behind it, bluer light more.
 *
 * The line colours are uniforms, so the same physics can be shown in the natural palette or
 * in the mapped palettes astrophotographers use.
 */

#if defined(INCLUDE_VS)

void main()
{
	vec2 corner[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

	gl_Position = vec4(corner[gl_VertexID], 0.0, 1.0);
}

#endif

#if defined(INCLUDE_FS)

uniform int u_Face;
uniform float u_FaceSize;
uniform int u_BubbleCount;
uniform float u_BubbleBound[NSKY_MAX_BUBBLES];

uniform sampler3D u_Light;
uniform float u_LightRes;
uniform float u_LightSlabs;

uniform float u_Density;	/* multiplies the gas everywhere */
uniform float u_Sigma;		/* extinction per bubble radius at density 1 */
uniform float u_StepFrac;	/* march step, as a fraction of the shell's thickness */
uniform int u_MaxSteps;		/* per bubble */

uniform vec3 u_LineColor[3];	/* [O III], H-alpha, [S II] */
uniform vec3 u_LineStrength;
/* The ionisation parameter, flux over density, above which oxygen is doubly ionised.  See
 * where the lines are weighed in main(). */
uniform float u_OxygenThreshold;
uniform vec3 u_DustAlbedo;
uniform float u_Anisotropy;	/* Henyey-Greenstein g */
uniform float u_Reflection;
uniform float u_RimShadow;	/* fine self shadowing toward the cluster; 0 is off */
/* How much more opaque the gas is to ionising ultraviolet than to visible light.  Large, in
 * reality: the ultraviolet is used up within a thin skin, which is why an ionisation front is
 * a sharp bright rim and not a gradual fade.  The light volume already carries it. */
uniform float u_IonOpacity;
uniform vec3 u_Reddening;	/* relative extinction per channel */


/* A cluster's light as if spread over a core this size, squared, in bubble radii.  A point
 * source's inverse square runs to infinity at the stars, and any gas near them burns white;
 * a real association is tens of parsecs across anyway. */
#define NSKY_CORE2 0.04


uniform float u_Exposure;

out vec4 f_FragColor;

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

/* A per texel offset for where the march starts, so the step pattern shows as a fine grain
 * rather than as contour lines across the shell.  A hash, i.e. white noise: interleaved
 * gradient noise was tried first, and its regular lattice shows as a dotted hatching wherever
 * a step crosses a hard edge, which on a dense pillar is everywhere. */
float start_jitter(vec2 xy)
{
	vec3 p = fract(vec3(xy.xyx) * 0.1031);

	p += dot(p, p.yzx + 33.33);
	return fract((p.x + p.y) * p.z);
}

/* Optical depth back to each of the bubble's clusters, from bubble b's slab of the light
 * volume. */
vec3 light_depth_at(int b, vec3 p)
{
	vec3 uvw = clamp(p / u_BubbleBound[b] * 0.5 + 0.5, vec3(0.0), vec3(1.0));
	float half_texel = 0.5 / u_LightRes;

	uvw.z = (float(b) + clamp(uvw.z, half_texel, 1.0 - half_texel)) / u_LightSlabs;
	return texture(u_Light, uvw).rgb;
}

/* With clusters of a size, the light over a penumbra about p: six taps half a cluster's
 * radius out along the axes.  The light volume's voxels each saw the cluster from a few points
 * turned at random; averaged here, that is a soft shadow and not a stack of hard ones -- and
 * the long shadows lumps cast through a thick mass, which a cluster's real size alone leaves
 * as streaks, blur away.
 *
 * Averaged as a power mean of the light, exponent 0.3.  The light's own mean lets one lit tap
 * dominate: taps that reach out of a mass into the lit cavity lifted the whole sky by a third.
 * The depth's mean lets one deep tap dominate, and darkened it by as much.  At 0.3 the sky's
 * mean brightness is within a few percent of point clusters', measured on two seeds. */
vec3 light_depth(int b, vec3 p)
{
	const vec3 tap[6] = vec3[6](vec3(1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0),
				vec3(0.0, -1.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, -1.0));
	vec3 light = vec3(0.0);
	int k;

	return light_depth_at(b, p);
}

/* Which of bubble b's capsules this ray, from origin along d in the bubble's frame, passes
 * near enough to matter -- the bits nsky_pillars() then looks at.  Each capsule is bounded by
 * a sphere: half its length plus its thickest radius, padded for everything that can move
 * its surface or draw fine steps toward it -- three radii of noise and near zone, and for a
 * pillar, which is placed in the WARPED frame, the most the fold warp can displace it.
 * Conservative: a capsule missed here would simply vanish from this texel. */
void capsule_mask(int b, vec3 origin, vec3 d, float s0, float s1)
{
	int first = int(u_PillarRange[b].x), n = int(u_PillarRange[b].y), i;
	float warp = 1.1 * u_BubbleShape[b].y;

	nsky_mask[0] = 0u;
	nsky_mask[1] = 0u;
	for (i = 0; i < n; i++) {
		vec4 a = u_PillarBase[first + i], t = u_PillarTip[first + i];
		vec3 mid = 0.5 * (a.xyz + t.xyz);
		float reach = 0.5 * length(t.xyz - a.xyz) + 3.2 * max(abs(a.w), t.w) +
				(a.w < 0.0 ? 0.0 : warp);
		float along = clamp(dot(mid - origin, d), s0, s1);
		vec3 off = origin + d * along - mid;

		if (dot(off, off) < reach * reach)
			nsky_mask[i >> 5] |= 1u << uint(i & 31);
	}
}

/* Henyey-Greenstein, normalised to 1 for isotropic scattering, as the old nebula lab found it
 * had to be: with the 4 pi left in, the scattered light is two orders of magnitude under the
 * emission and never shows. */
float phase(float cos_theta, float g)
{
	float g2 = g * g;

	return (1.0 - g2) / pow(max(1.0 + g2 - 2.0 * g * cos_theta, 1e-4), 1.5);
}

void main()
{
	vec2 st = gl_FragCoord.xy / u_FaceSize * 2.0 - 1.0;
	vec3 dir = normalize(face_direction(u_Face, st));
	/* The angle one texel subtends here: the face's texel pitch, shrunk toward the edges
	 * and corners where the cube stands further from the eye. */
	float pixel_angle = (2.0 / u_FaceSize) / (1.0 + dot(st, st));
	float jitter = start_jitter(gl_FragCoord.xy + vec2(float(u_Face) * 37.0, 0.0));
	float enter[NSKY_MAX_BUBBLES], leave[NSKY_MAX_BUBBLES];
	int order[NSKY_MAX_BUBBLES];
	vec3 transmit = vec3(1.0), glow = vec3(0.0), base, haze;
	int n = 0, i, j, k, c;
	bool opaque = false;

	/* Which bubbles this direction passes through, nearest entry first.  The model keeps
	 * the distant ones clear of the main one, so ordering by where each is entered is
	 * ordering front to back. */
	for (i = 0; i < u_BubbleCount; i++) {
		vec3 center = u_BubbleSphere[i].xyz;
		float r = u_BubbleSphere[i].w * u_BubbleBound[i];
		float bb = dot(center, dir);
		float disc = bb * bb - (dot(center, center) - r * r);
		float t0, t1, sq;

		if (disc <= 0.0)
			continue;
		sq = sqrt(disc);
		t0 = max(bb - sq, 0.0);
		t1 = bb + sq;
		if (t1 <= 0.0)
			continue;
		for (j = n; j > 0 && enter[j - 1] > t0; j--) {
			enter[j] = enter[j - 1];
			leave[j] = leave[j - 1];
			order[j] = order[j - 1];
		}
		enter[j] = t0;
		leave[j] = t1;
		order[j] = i;
		n++;
	}

	for (k = 0; k < n && !opaque; k++) {
		int b = order[k];
		int first = b * NSKY_CLUSTERS;
		float radius = u_BubbleSphere[b].w;
		mat3 rot = u_BubbleRot[b];
		vec3 origin = rot * (-u_BubbleSphere[b].xyz / radius);
		vec3 d = rot * dir;
		float thick = u_BubbleShape[b].x;
		float ds_fine = thick * u_StepFrac;
		float s_end = leave[k] / radius;
		float s = enter[k] / radius + ds_fine * jitter;

		capsule_mask(b, origin, d, s, s_end);


		for (i = 0; i < u_MaxSteps && s < s_end; i++) {
			vec3 p = origin + d * s;
			nsky_gas g;
			/* The detail is limited to what one sample can hold, and a sample is a
			 * long thin box: a texel wide, but a step long.  Filtered to the texel
			 * alone, detail far finer than the step is point sampled along the ray
			 * and aliases into speckle; a fifth of the step is the compromise
			 * between that and blurring what the texel could have resolved. */
			float rho = nsky_density(b, p, max(s * pixel_angle, 0.2 * ds_fine), g);
			float ds;

			ds = ds_fine;




			if (rho > 0.0) {
				vec3 tau = light_depth(b, p);
				vec3 src = vec3(0.0), sigma_t, att, best_dir = vec3(0.0);
				float best = 0.0;
				int best_c = 0;

				rho *= u_Density;
				for (c = 0; c < NSKY_CLUSTERS; c++) {
					vec4 cl = u_Cluster[first + c];
					vec3 lc = cl.xyz - p;
					float f = cl.w / (dot(lc, lc) + NSKY_CORE2) * exp(-tau[c]);

					if (cl.w > 0.0 && f > best) {
						best = f;
						best_c = c;
						best_dir = normalize(lc);
					}
				}

				if (u_RimShadow > 0.0 && best > 0.0) {
					/* What the light volume is too coarse to hold: the
					 * gas's own detail shadowing itself, within a shell's
					 * thickness of this point, toward whichever cluster
					 * lights it most.  It is what puts a bright rim on the
					 * lit side of every knot.  One sample, midway, at
					 * coarse detail: two, one near and one far, cost a
					 * third of the whole bake and changed the picture by
					 * about one percent. */
					nsky_gas g1;
					float between = nsky_density(b, p + best_dir * thick * 0.6,
							max(s * pixel_angle, thick * 0.25), g1);

					tau[best_c] += u_RimShadow * u_Sigma * u_IonOpacity *
						u_Density * thick * between;
				}
				for (c = 0; c < NSKY_CLUSTERS; c++) {
					vec4 cl = u_Cluster[first + c];
					vec3 lc = cl.xyz - p;
					float d2 = dot(lc, lc);
					float flux, w_o, w_h, w_s, u, o;

					if (cl.w <= 0.0)
						continue;
					/* What arrives here, times what this gas takes out of
					 * it.  The absorption coefficient belongs in the
					 * emission as much as in the shadow: every ionising
					 * photon a front absorbs comes back out as line light,
					 * so a front sharpened by more opacity gets thinner,
					 * not dimmer. */
					flux = cl.w / (d2 + NSKY_CORE2) * u_Sigma * u_IonOpacity;
					/* Which lines, by the IONISATION PARAMETER -- photons
					 * arriving per atom.  Where it is high, thin gas close
					 * to the stars, oxygen is stripped twice and [O III]
					 * glows teal; in the dense fronts it is low and hydrogen
					 * and sulphur glow red.  Weighing them this way keeps
					 * the two apart, as they are in any real HII region:
					 * the teal core and the red rim.  Weighed only by depth
					 * they overlap everywhere, and teal over red is grey. */
					u = cl.w / (d2 + NSKY_CORE2) * exp(-tau[c]) / max(rho, 0.02);
					o = smoothstep(0.5, 2.0, u / u_OxygenThreshold);
					w_h = exp(-tau[c]);
					w_o = o * w_h;
					w_s = (1.0 - o) * exp(-0.5 * tau[c]) * (1.0 - exp(-2.0 * tau[c]));
					w_h *= 1.0 - 0.8 * o;
					/* Molecular cloud is not ionised through; only its
					 * lit skin glows, and the light volume already puts
					 * that on its face.  Nothing, then, once the dust is
					 * substantial: taken off only in proportion, a lane at
					 * the dust dial's 0.6 still glowed at 0.4 -- and, three
					 * times as dense, brighter than the gas around it -- so
					 * the dark lanes were lit lanes coloured brown.  What is
					 * left glowing is the partly dusty edge: the rim. */
					src += flux * rho * nsky_dust_glow(g.dust) *
						(u_LineColor[0] * (u_LineStrength.x * w_o) +
						u_LineColor[1] * (u_LineStrength.y * w_h) +
						u_LineColor[2] * (u_LineStrength.z * w_s));
					/* Visible starlight reaches far deeper than the
					 * ultraviolet, and dust scatters it, forward most.
					 *
					 * But not into dust: dust stops visible light as the
					 * view march has it stop it, u_DustOpacity times
					 * harder, and the starlight that lights a dusty point
					 * has come through the dust around it -- this point's
					 * dust stands for the path's.  Without it a dark
					 * cloud's inside was lit nearly as clear gas is, and
					 * glowed brown, where in every reference sky dust is
					 * dark, with only its skin toward the stars lit. */
					src += flux / u_IonOpacity * rho * (0.3 + g.dust) *
						u_Reflection * u_DustAlbedo *
						phase(dot(lc * inversesqrt(max(d2, 1e-8)), d),
							u_Anisotropy) *
						exp(-tau[c] / u_IonOpacity *
							(1.0 + u_DustOpacity * g.dust));
				}


				/* A pillar's core neither glows nor scatters: starlight does not
				 * reach it.  The light volume is coarser than a pillar is wide and
				 * cannot say so -- left to it, the dust in the core scatters blue
				 * and the pillar comes out a hollow tube with a bright outline. */
				src *= 1.0 - g.core;

				/* Integrated exactly over the step for a constant source and
				 * extinction, rather than as source times step, so a dense step
				 * cannot emit more than it could ever let out. */
				sigma_t = rho * u_Sigma * (1.0 + u_DustOpacity * g.dust) * u_Reddening;
				att = exp(-sigma_t * ds);
				glow += transmit * src * (vec3(1.0) - att) / max(sigma_t, vec3(1e-6));
				transmit *= att;
			}
			s += ds;
			if (max(transmit.r, max(transmit.g, transmit.b)) < 0.002) {
				opaque = true;
				break;
			}
		}
	}
	glow *= u_Exposure;
	f_FragColor = vec4(glow, dot(transmit, vec3(1.0 / 3.0)));
}

#endif
