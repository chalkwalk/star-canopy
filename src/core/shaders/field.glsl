/* The sky's gas: where it is and how dense, for every pass that needs to know.
 *
 * Concatenated in front of both the light pass and the bake, so the light volume and the
 * view march cannot disagree about where the shell is.  See scene.h for the
 * bubble and its units: everything here is in a bubble's own frame, where it is a unit
 * sphere.
 *
 * Two levels, and the split between them is the whole design:
 *
 *	nsky_coarse()	the shell itself -- its fold, its thickness, its holes, and the
 *			faint gas left in the cavity.  Smooth, cheap, and all the light
 *			volume sees, since shadowing at the scale of the shell is what makes
 *			the ionisation front.
 *
 *	nsky_density()	the coarse shell eroded by detail, octave by octave, down to the
 *			size of the pixel the sample lands in and no further.
 *
 * The octave limit is what lets one bubble be both the wall ten degrees away and the far
 * wall a hundred and seventy: the near wall's pixel is small in the bubble's units and gets
 * many octaves, the far wall's is large and gets few, and neither aliases.  A baked density
 * volume cannot do that -- its resolution is the same everywhere, so the near wall is either
 * blurred or the volume is enormous.
 */

#define NSKY_MAX_BUBBLES 8
#define NSKY_CLUSTERS 3		/* per bubble; must match kMaxClusters */
#define NSKY_MAX_OCTAVES 9
/* Must match kNoisePeriod. */
#define NSKY_NOISE_PERIOD 8.0
/* The most the contrast may thicken the shell; see nsky_coarse(). */
#define NSKY_CONTRAST_CAP 3.0

uniform sampler3D u_Noise;

uniform vec4 u_BubbleSphere[NSKY_MAX_BUBBLES];	/* centre in sky units, radius */
uniform mat3 u_BubbleRot[NSKY_MAX_BUBBLES];	/* sky offset -> bubble frame */
/* thickness, fold, keep, noise offset */
uniform vec4 u_BubbleShape[NSKY_MAX_BUBBLES];
/* Each bubble's clusters, NSKY_CLUSTERS apiece: position in the bubble frame, luminosity.
 * A luminosity of zero is an unused slot. */
uniform vec4 u_Cluster[NSKY_MAX_BUBBLES * NSKY_CLUSTERS];

/* Pillars: base and tip in the warped bubble frame, with their radii, and each bubble's range
 * of them as (first, count).  See struct Pillar. */
#define NSKY_MAX_PILLARS 56
uniform vec4 u_PillarBase[NSKY_MAX_PILLARS];
uniform vec4 u_PillarTip[NSKY_MAX_PILLARS];
uniform vec2 u_PillarRange[NSKY_MAX_BUBBLES];
uniform float u_PillarDensity;	/* relative to the shell's */
uniform float u_CloudDensity;	/* the dark clouds adrift in the cavity, likewise */

uniform float u_FoldScale;	/* fold warp frequency, cycles per bubble radius */
uniform float u_OuterSharpness;	/* how much harder the shell's outer edge is than its inner */
uniform float u_HoleScale;	/* frequency of the holes and thickness variation */
uniform float u_CavityDensity;	/* faint gas filling the cavity */
/* Each bubble's dense side, unnormalised; zero for none.  See nsky_blister(). */
uniform vec3 u_BlisterAxis[NSKY_MAX_BUBBLES];
uniform float u_Blister;	/* how far the far side is blown out, 0..1 */
/* The main bubble's form: 0 a thin shell, 1 a thick billowing mass.  See nsky_coarse(). */
uniform int u_Form;
uniform float u_MassInner;	/* the mass's inner surface, in bubble radii */
uniform float u_MassLobes;	/* how far its lobes reach in from that, in bubble radii */
uniform float u_MassScale;	/* its billows, cycles per bubble radius */
uniform float u_MassDensity;	/* its density, relative to the shell's */
uniform float u_MassWarp;	/* how far the fold warp bends it, 0..1 */
/* Each bubble's squeeze along its own axes (1 or more) and outer edge hardness relative to
 * u_OuterSharpness.  See struct Bubble. */
uniform vec4 u_BubbleForm[NSKY_MAX_BUBBLES];
uniform float u_BubbleDensity[NSKY_MAX_BUBBLES];	/* multiplies each bubble's gas */
uniform float u_DetailScale;	/* the coarsest detail octave, cycles per bubble radius */
uniform float u_DetailGain;	/* amplitude kept per octave */
uniform float u_Erosion;	/* how far detail eats into the shell, 0..1 */
uniform float u_Filament;	/* 0 billowed detail, 1 ridged filaments */
uniform float u_Hardness;	/* 0 soft eroded edges, 1 abrupt ones */
uniform float u_Contrast;	/* spread of the shell's column density; 0 is uniform */
uniform float u_DustAmount;	/* how much of the shell is dark molecular cloud, 0..1 */
uniform float u_DustScale;	/* size of the dark lanes, cycles per bubble radius */
uniform float u_DustOpacity;	/* extra extinction of the dust */
uniform float u_ClusterSize;	/* a cluster's radius, bubble radii; 0 a point */
uniform int u_DustVein;		/* 0 lanes of flat dark, 1 soft veins, 2 physical; see nsky_coarse() */

/* How big a sample is, in bubble radii: the finest detail it can hold without aliasing is
 * about twice this.  Set by nsky_density() for the view march; the light pass leaves it at
 * its default, which is coarse, since the light volume holds nothing finer anyway. */
float nsky_footprint = 0.02;

/* Which of the current bubble's capsules are worth measuring at all, as a bit per capsule
 * in its range.  The view march sets it once per ray from each capsule's bounding sphere
 * (see capsule_mask() in the bake): most capsules are nowhere near most rays, and measuring
 * every one at every sample was half the bake.  All set by default, for the light pass,
 * whose rays go everywhere. */
uint nsky_mask[2] = uint[2](0xffffffffu, 0xffffffffu);

/* The weight an octave of this feature size gets at the current footprint: 1 well above it,
 * fading to 0 at twice it.  The same rule nsky_detail() uses. */
float nsky_octave(float feature)
{
	return clamp(feature / (2.0 * nsky_footprint) - 1.0, 0.0, 1.0);
}

/* What the coarse field says about a point. */
struct nsky_gas {
	vec3 q;		/* the warped point, which the detail is read at too */
	float shell;	/* the shell's own gas, dust included */
	float cavity;	/* the faint fill of the cavity */
	float dust;	/* how much of the shell here is dark molecular cloud, 0..1 */
	float min_feature;	/* the finest detail worth adding here, bubble radii */
	float near_pillar;	/* 1 within a few radii of a pillar, where strides must be short */
	float near_width;	/* the radius of the nearest such, which sets how short */
	float core;		/* how deep inside a pillar's unlit core, 0..1 */
};

/* One fetch: four independent single octave noises, each about -0.7..0.7. */
vec4 nsky_noise(vec3 p)
{
	return texture(u_Noise, p * (1.0 / NSKY_NOISE_PERIOD)) * 2.0 - 1.0;
}

/* How deep inside the nearest of bubble b's pillars q is: 1 in the core, 0 outside.  A
 * tapered capsule: the distance to its axis over the radius there, with rounded ends.  core
 * is the same measure taken tighter, for how much of the pillar is dark molecular gas
 * rather than lit skin.
 *
 * The same for the dark clouds adrift in the cavity, which are capsules too, marked by a
 * negative base radius, and returned apart in drift: they are measured at p, the UNWARPED
 * point, having no wall to follow, and the holes in the wall do not apply to them. */
float nsky_pillars(int b, vec3 q, vec3 p, out float core, out float width, out float near,
			out float drift, out float near_width)
{
	int first = int(u_PillarRange[b].x), n = int(u_PillarRange[b].y), i;
	float inside = 0.0;

	core = 0.0;
	width = 1.0;
	near = 0.0;
	drift = 0.0;
	near_width = 1.0;
	for (i = 0; i < n; i++) {
		vec4 a, t;
		bool adrift;

		if (((nsky_mask[i >> 5] >> uint(i & 31)) & 1u) == 0u)
			continue;
		a = u_PillarBase[first + i];
		t = u_PillarTip[first + i];
		adrift = a.w < 0.0;
		vec3 x = adrift ? p : q;
		vec3 ab = t.xyz - a.xyz;
		float h = clamp(dot(x - a.xyz, ab) / dot(ab, ab), 0.0, 1.0);
		float radius = mix(abs(a.w), t.w, h);
		float d = length(x - a.xyz - ab * h) / radius;
		float amount;
		vec4 n1, n2;

		/* Lumpy, not a tube: a clean capsule reads as a wax finger however it is lit.
		 * But lumpy on the column's own terms.  The noise is read in a frame squashed
		 * along the axis, so its lumps run three times longer than they are wide --
		 * striations down the column and a few big knots, as a real pillar has --
		 * where isotropic lumps the size of its width crumple it until the eye cannot
		 * find the column at all.  Only near the surface is it worth the fetches. */
		if (d < 1.8) {
			/* Measured from the capsule's own base, and over its base radius, which
			 * is constant along it.  Over the local radius instead, which tapers, a
			 * step along the axis rescales the whole coordinate -- a large one, it
			 * being a position -- and sweeps the noise past, which bands the capsule
			 * with stripes across its length. */
			vec3 along = ab / length(ab);
			vec3 rel = x - a.xyz;
			vec3 w = (rel - along * (dot(rel, along) * 0.67)) / abs(a.w);

			n1 = nsky_noise(w * 0.9 + float(i) * 7.3);
			n2 = nsky_noise(w * 2.3 + float(i) * 3.1);
			/* Clouds more ragged than pillars: a pillar is shaped by the light
			 * streaming past it, a dark cloud only by its own turbulence -- so a
			 * third, finer octave, and more of each. */
			/* Each octave faded out as it nears the sample's size, as the shell's
			 * detail is; unfiltered, the finest of them is far below the step near a
			 * cloud and aliases into grain along its edge. */
			float r0 = abs(a.w);
			float w1 = nsky_octave(r0 / 0.9), w2 = nsky_octave(r0 / 2.3);

			if (adrift) {
				float w3 = nsky_octave(r0 / 5.7);

				d += 0.9 * w1 * n1.x + 0.45 * w2 * n2.y;
				if (w3 > 0.0)
					d += 0.22 * w3 * nsky_noise(w * 5.7 + float(i) * 1.7).z;
			} else {
				d += 0.4 * w1 * n1.x + 0.12 * w2 * n2.y;
			}
		}
		if (d < 1.3)
			width = min(width, radius);
		/* How near, smoothly, so the march's step can ease into and out of the fine
		 * steps a capsule needs rather than switching at a boundary that shows. */
		if (d < 3.0 && radius < near_width * (3.0 - d)) {
			near = max(near, 1.0 - smoothstep(2.0, 3.0, d));
			near_width = min(near_width, radius);
		}

		/* A wide ramp, so the erosion has a gradient to eat into and the edges come
		 * out ragged rather than a smooth tube. */
		/* A cloud's edge is narrower than a pillar's: near the camera a wide ramp is
		 * simply a blur. */
		amount = adrift ? 1.0 - smoothstep(0.6, 1.1, d) : 1.0 - smoothstep(0.2, 1.3, d);
		if (adrift)
			drift = max(drift, amount);
		else
			inside = max(inside, amount);
		core = max(core, 1.0 - smoothstep(0.3, 0.85, d));
	}
	return inside;
}

/* How much of the gas survives at the direction dir from the bubble's centre, given a
 * noise to rag the edge with: 1 on the dense side, down to a faint haze on the open side.
 *
 * Real HII regions are rarely whole bubbles.  The stars are born at the edge of a molecular
 * cloud, the ionised gas breaks out wherever the cloud is thinnest, and what is left is a
 * blister: a dense, bright wall on the cloud's side and gas streaming away on the other.
 * The Orion Nebula is one, seen from the open side.  Seen from inside, the dense wall is the
 * part of the sky the light is on and the rest is open to the stars, which is the one place
 * in the sky a picture of it has to look -- rather than a bright wall all the way round.
 *
 * The haze left is the outflow, and it is what stops the open side reading as a cut. */
float nsky_blister(int b, vec3 dir, float rag)
{
	vec3 axis = u_BlisterAxis[b];
	float len = length(axis), side, edge;

	if (u_Blister <= 0.0 || len < 1e-4)
		return 1.0;
	side = dot(dir, axis / len) + 0.6 * rag;
	/* 0 opens nothing, 1 leaves a cap about a third of the sky across. */
	edge = mix(-1.9, 0.45, u_Blister);
	return mix(0.04, 1.0, smoothstep(edge - 0.3, edge + 0.3, side));
}

nsky_gas nsky_coarse(int b, vec3 p)
{
	vec4 shape = u_BubbleShape[b];
	vec3 o = vec3(shape.w, shape.w * 1.618, shape.w * 2.414);
	vec3 wp = p * u_FoldScale + o;
	vec3 w = nsky_noise(wp).xyz + 0.5 * nsky_noise(wp * 2.07 + 13.1).xyz;
	nsky_gas g;
	vec4 n;
	float r, thick, s, keep, lane, contrast_noise, open, rin = 1.0, clump;

	/* Displace the POINT, not the result.  Warping where the shell is measured is what
	 * folds it: a sheet pushed about by a smooth field buckles into ridges and troughs,
	 * and wherever a fold runs along the line of sight the sheet is seen edge on and the
	 * path through it is long -- which is what a filament in a real shell is. */
	g.q = p + w * shape.y;
	/* A mass is modelled, not folded: the fold's swirl, which gives a thin shell its
	 * filaments, covers a thick one in deep waves, where the references' masses are blocked
	 * out like clay.  Mostly straightened. */
	if (u_Form == 1 && b == 0)
		g.q = p + w * shape.y * u_MassWarp;
	g.min_feature = 0.0;
	g.near_pillar = 0.0;
	g.near_width = 1.0;
	g.core = 0.0;
	r = length(g.q * u_BubbleForm[b].xyz);

	n = nsky_noise(g.q * u_HoleScale + o * 0.37);
	thick = shape.x * (0.6 + 0.8 * clamp(n.y + 0.5, 0.0, 1.0));
	s = (r - 1.0) / thick;
	/* Harder on the outside than the in.  Vela's filaments have a sharp edge away from the
	 * remnant's centre and fade over several times that toward it, which is the shock
	 * running outward into cold gas with the hot interior behind it. */
	g.shell = exp(-s * s * (s > 0.0 ? max(u_OuterSharpness * u_BubbleForm[b].w, 1.0) : 1.0));
	/* Or, for the main bubble, a thick mass of rounded lumps round the glowing cavity.
	 *
	 * The reference skyboxes surround the eye with cloud: stretched, their darkest sky is the
	 * faintly lit faces of billows almost everywhere, with the light breaking through in a
	 * few places.  A thin shell cannot be that -- its holes show the smooth haze behind, not
	 * more cloud, and every part of it is seen lit from the eye's side.  So here the gas fills
	 * a thick layer from an inner surface out to the bubble's edge.  The inner surface is
	 * pushed in by a low noise into lobes that reach over the cavity, in front of its glow,
	 * as part of the one mass and not as clouds of their own.  Its lumps are a smooth noise
	 * of a few octaves, in the density and on the surface.  Cleared about the viewer: gas at
	 * the eye is fog over the whole sky. */
	if (u_Form == 1 && b == 0) {
		vec3 view = u_BubbleRot[b] * (-u_BubbleSphere[b].xyz / u_BubbleSphere[b].w);
		/* The lobes from a noise of the point, not of its direction alone: of the direction,
		 * the inner surface moves in and out the same all along each ray from the centre,
		 * and every lobe is a prism standing on it, straight-sided, that from off the
		 * centre shows as streaks and sawteeth.  Of the point, a lobe is a rounded mass. */
		vec3 lq = g.q * 1.6 + o * 0.23, bq = g.q * u_MassScale + o * 0.41;
		float lobe = nsky_noise(lq).x + 0.5 * nsky_noise(lq * 2.1 + 3.7).y;
		float billow = 0.0, amp = 1.0, dens;
		int k;

		/* Smooth lumps, two octaves: the absolute value of a noise, which is round lumps
		 * with creases between, reads as cauliflower, and more octaves as crumple. */
		for (k = 0; k < 2; k++) {
			billow += amp * 1.6 * nsky_noise(bq).x;
			amp *= 0.4;
			bq = bq * 2.07 + 7.3;
		}
		rin = clamp(u_MassInner - u_MassLobes * max(lobe + 0.15, 0.0), 0.1, 0.95);
		dens = smoothstep(-0.02, 0.1, r - rin + 0.12 * billow) *
			(1.0 - smoothstep(0.92, 1.05, r - 0.06 * billow));
		dens *= smoothstep(-0.2, 0.4, billow + 0.2);
		dens *= smoothstep(0.1, 0.3, length(p - view));
		/* And about each cluster, whose winds have blown their surroundings clear: a star
		 * buried in the mass lights nothing we can see. */
		for (k = 0; k < NSKY_CLUSTERS; k++) {
			vec4 cl = u_Cluster[b * NSKY_CLUSTERS + k];

			if (cl.w > 0.0)
				dens *= smoothstep(0.12, 0.35, length(p - cl.xyz));
		}
		g.shell = dens * u_MassDensity;
		/* The dust keeps to the body of the mass, which is all of it now. */
		s = 0.0;
	}
	/* Contrast ACROSS the shell's face.  A shell of even density is about one optical depth
	 * through wherever it is looked at, so the whole sky comes out half veiled -- fog, with
	 * nothing either clear or solid.  Real interstellar gas has a lognormal column density:
	 * most sight lines thin, a few very thick.  So the shell's density is multiplied by the
	 * exponential of a smooth noise, at the scale of the holes; the detail inside is left
	 * alone.  Thresholding the fine erosion instead cuts the gas into flat-bottomed blots,
	 * which is the decal look.  The divisor keeps the mean where it was.
	 *
	 * The thick end is capped.  Uncapped it reaches over ten times the shell's density,
	 * where the lit ionisation front is far thinner than any step: a texel whose step
	 * lands right at the surface gets the front, one whose step lands a little deeper
	 * gets gas the light never reached, and the thickest patches come out peppered
	 * black.  The clear gaps come from the thin end, which is left alone. */
	clump = min(exp(u_Contrast * 2.5 * n.z) / exp(0.03 * u_Contrast * u_Contrast * 6.25),
			NSKY_CONTRAST_CAP);
	g.shell *= clump;
	contrast_noise = n.w;

	/* Holes.  keep is the fraction that survives, mapped onto the noise's range; a hole
	 * is where the distant sky shows through, and the layering that gives is half the
	 * depth in the picture. */
	keep = smoothstep(0.95 - 1.2 * shape.z, 1.07 - 1.2 * shape.z, n.x + 0.5);
	open = nsky_blister(b, normalize(g.q), n.x);
	keep *= open;

	/* Dark lanes: the ridges of one noise, where it crosses zero, broken into lengths by a
	 * second.  Lanes and not blobs, because a real HII region's dark clouds are threaded
	 * across the glow -- a round black blot reads as a hole in the sky, not as something
	 * in front of it.  Only within the shell's body, never out in its wisps. */
	n = nsky_noise(g.q * u_DustScale + o * 0.71);
	lane = 1.0 - abs(n.z) * 4.0;
	g.dust = u_DustAmount * smoothstep(0.2, 0.8, lane) * smoothstep(-0.15, 0.15, n.w) *
		(1.0 - smoothstep(0.5, 2.0, s * s));
	/* Or soft veins: as dark along the centre line, but falling off either side of it as a
	 * bell, and its lengths fading in and out rather than cut, so a lane is a vein through
	 * the glow and not a flat black band laid over it -- see nsky_dust_glow() too. */
	if (u_DustVein == 1)
		g.dust = u_DustAmount * exp(-(n.z / 0.1) * (n.z / 0.1)) *
			smoothstep(-0.35, 0.35, n.w) * (1.0 - smoothstep(0.5, 2.0, s * s));
	/* Or physical dust: where the gas is thickest, its clumps, as molecular cloud is -- not
	 * drawn on in lanes of its own.  It is gas like the rest, but more opaque: the light volume
	 * counts it, so it shadows what lies beyond it, and it glows and scatters by whatever
	 * light reaches it, so it is dark only where the light is blocked.  See the march. */
	if (u_DustVein == 2)
		g.dust = u_DustAmount * smoothstep(1.2, 2.8, clump) *
			(1.0 - smoothstep(0.5, 2.0, s * s));
	g.shell *= keep * (1.0 + (u_DustVein == 2 ? 1.0 : 3.0) * g.dust);

	/* Pillars, only where they can be: near the wall.  Dense and dusty, since they are the
	 * molecular gas the front has not yet eaten; the light volume sees them, so they shadow
	 * the wall behind, and the fine self shadowing lights their tips.  keep applies, so no
	 * pillar stands in a hole with nothing to be rooted in. */
	if (u_PillarRange[b].y > 0.0 && !(u_Form == 1 && b == 0) &&
			(abs(r - 1.0) < 0.5 || length(p) < 0.85)) {
		float core, width, drift;
		float pillar = nsky_pillars(b, g.q, p, core, width, g.near_pillar, drift,
						g.near_width) * keep;

		/* The holes in the wall cut pillars, which stand on it, and not the clouds. */
		core *= max(keep, step(0.001, drift));
		g.shell = max(g.shell, max(pillar * u_PillarDensity, drift * u_CloudDensity));
		pillar = max(pillar, drift);
		/* All dust in the core, so only the skin glows.  The light volume is coarser
		 * than a pillar is wide and would light the whole body; a real pillar is a dark
		 * column with a bright edge. */
		g.dust = max(g.dust, core);
		g.core = core;
		/* Detail no finer than a quarter of the pillar's width, inside one: the shell's
		 * erosion is scaled to the shell, and at a pillar's size its finest octaves only
		 * crinkle the surface. */
		g.min_feature = mix(g.min_feature, (drift > 0.0 ? 0.1 : 0.25) * width, pillar);
	}

	/* The ionised gas inside the shell sits in a layer against it, not in a ball on the
	 * stars: the clusters' winds have blown their surroundings clear, which is the cavity
	 * the Rosette's centre is.  Filling the middle instead puts the densest gas where the
	 * light is fiercest, and it burns out to white. */
	g.cavity = u_CavityDensity * smoothstep(0.35, 0.95, r) * (1.0 - smoothstep(0.95, 1.1, r));
	/* A mass's cavity is inside its inner surface, lobes and all. */
	if (u_Form == 1 && b == 0)
		g.cavity = u_CavityDensity * smoothstep(0.1, 0.6, r / rin) *
				(1.0 - smoothstep(0.9, 1.05, r / rin));
	/* Clumped the same way as the shell, from a noise of its own: the ionised gas glows in
	 * patches, so it does not lay a veil over every gap in the shell. */
	g.cavity *= exp(u_Contrast * 2.5 * contrast_noise) / exp(0.03 * u_Contrast * u_Contrast * 6.25);
	/* The ionised layer lines the wall, so it goes where the wall goes. */
	g.cavity *= open;
	return g;
}

/* How much of its glow gas with this much dust keeps, 0..1.  Molecular cloud is not ionised
 * through, so a lane glows only at its partly dusty edges.  Flat lanes stop glowing as soon
 * as they are substantially dusty, which is the flat dark; soft veins keep glow in proportion
 * to how far below the centre line's dust they are, so they are dark only along it. */
float nsky_dust_glow(float dust)
{
	if (u_DustVein == 2)
		return 1.0;
	if (u_DustVein == 1)
		return 1.0 - clamp(dust / max(u_DustAmount, 1e-3), 0.0, 1.0);
	return 1.0 - smoothstep(0.1, 0.45, dust);
}

/* Fractal detail, signed, about -0.5..0.5, with every octave finer than twice the footprint
 * faded out rather than fetched.
 *
 * An octave near the pixel's size is not detail but aliasing; above it, it is averaged away
 * to its mean, which is zero, so dropping it IS the correctly filtered answer and not an
 * approximation of one.  The normalisation is over every octave regardless, so the contrast
 * does not change as octaves drop out with distance. */
float nsky_detail(int b, vec3 q, float footprint)
{
	const mat3 turn = mat3(0.00, 0.80, 0.60,
				-0.80, 0.36, -0.48,
				-0.60, -0.48, 0.64);
	vec3 p = q * u_DetailScale + vec3(u_BubbleShape[b].w * 3.1);
	float feature = 1.0 / u_DetailScale;
	float a = 1.0, sum = 0.0, norm = 0.0;
	int i;

	for (i = 0; i < NSKY_MAX_OCTAVES; i++) {
		float w = clamp(feature / (2.0 * footprint) - 1.0, 0.0, 1.0);

		norm += a;
		if (w > 0.0) {
			vec4 n = nsky_noise(p);
			/* A ridge is where the noise crosses zero; folding it there piles the
			 * detail into creases.  0.2 is roughly the mean of |n|, so the ridged
			 * form is centred like the billowed one and the dial between them
			 * changes character rather than brightness. */
			float ridged = 0.2 - abs(n.y);

			sum += a * w * mix(n.x, ridged * 1.6, u_Filament);
		}
		a *= u_DetailGain;
		feature *= 0.5;
		p = turn * p * 2.03;
	}
	return sum / norm;
}

/* The gas density at p in bubble b, 0 and up, and the coarse field it was eroded from. */
float nsky_density(int b, vec3 p, float footprint, out nsky_gas g)
{
	float e, cut, total, dense;

	nsky_footprint = footprint;
	g = nsky_coarse(b, p);
	total = g.shell + g.cavity;
	if (total < 0.003)
		return 0.0;
	/* The gain on the detail is the edge hardness: steeper, and the erosion goes from
	 * nothing to everything over a narrower band of the noise, so what survives has a
	 * crisp edge -- the hard lit rim of a pillar rather than the soft edge of smoke. */
	e = clamp(0.5 + mix(2.2, 9.0, u_Hardness) *
			nsky_detail(b, g.q, max(footprint, 0.5 * g.min_feature)), 0.0, 1.0);
	/* Erode from the edges in.  Subtracting the detail's complement and renormalising
	 * leaves the dense heart of the shell nearly whole and eats hardest where it is
	 * already thin, which is what makes the edges wispy rather than the whole shell
	 * uniformly speckled.  The (0.4 + e) then puts knots and voids inside what
	 * survives, so the dense parts are not flat. */
	cut = u_Erosion * (1.0 - e);
	/* Eroded as if no denser than the shell, then scaled back up, so that dense gas -- a
	 * pillar, a clump the contrast has thickened -- gets the same ragged edge as the rest
	 * instead of a cut too small to show on it. */
	dense = max(g.shell, 1.0);
	g.shell /= dense;
	/* The cavity is left out of the erosion.  It is a hundredth of the shell's density, so
	 * any cut at all removes it entirely -- and it is the hot, fully ionised gas where the
	 * [O III] lives, the soft glow behind the filaments.  And it is left smooth: patchy
	 * only at the large scale, from the contrast.  Given the fine detail too, it varies
	 * faster than any stride through the cavity can follow, and every change of stride --
	 * near a pillar, near a cloud -- shows in it as a seam or a ripple. */
	return (max(g.shell - cut, 0.0) / max(1.0 - cut, 0.05) * (0.4 + e) * dense +
		g.cavity) * u_BubbleDensity[b];
}
