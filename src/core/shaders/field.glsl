/* The sky's gas: where it is and how dense, for every pass that needs to know.
 *
 * Concatenated in front of both the light pass and the bake, so the light volume and the
 * view march cannot disagree about where the gas is.  See scene.h for the bubble and its
 * units: everything here is in a bubble's own frame, where it is a unit sphere.
 *
 * Every bubble is a mass: a thick layer of billowing gas round a glowing cavity -- the main
 * one seen from within, the distant ones from outside.  (Look 1 had a thin shell as well,
 * retired in look 2; see ROADMAP.md.)  Two levels, and the split between them is the whole
 * design:
 *
 *	nsky_coarse()	the mass itself -- its lobes, its billows, its holes, and the
 *			faint gas in the cavity.  Smooth, cheap, and all the light volume
 *			sees, since shadowing at the scale of the mass is what makes the
 *			ionisation front.
 *
 *	nsky_density()	the coarse mass eroded by detail, octave by octave, down to the
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
/* The most the contrast may thicken the gas; see nsky_coarse(). */
#define NSKY_CONTRAST_CAP 3.0

uniform sampler3D u_Noise;

uniform vec4 u_BubbleSphere[NSKY_MAX_BUBBLES];	/* centre in sky units, radius */
uniform mat3 u_BubbleRot[NSKY_MAX_BUBBLES];	/* sky offset -> bubble frame */
/* stride scale (the march's; see bake.shader), fold, keep, noise offset */
uniform vec4 u_BubbleShape[NSKY_MAX_BUBBLES];
/* Each bubble's clusters, NSKY_CLUSTERS apiece: position in the bubble frame, luminosity.
 * A luminosity of zero is an unused slot. */
uniform vec4 u_Cluster[NSKY_MAX_BUBBLES * NSKY_CLUSTERS];


uniform float u_FoldScale;	/* fold warp frequency, cycles per bubble radius */
uniform float u_HoleScale;	/* frequency of the holes and thickness variation */
uniform float u_CavityDensity;	/* the main bubble's cavity glow */
/* Each bubble's dense side, unnormalised; zero for none.  See nsky_blister(). */
uniform vec3 u_BlisterAxis[NSKY_MAX_BUBBLES];
uniform float u_Blister;	/* how far the far side is blown out, 0..1 */
uniform float u_DistantDensity;	/* a distant mass's density, relative to the main one's */
uniform float u_DistantCavity;	/* a distant mass's cavity glow, as u_CavityDensity */
uniform float u_DistantBlister;	/* how far a distant mass is blown open, as u_Blister */
uniform float u_MassInner;	/* the mass's inner surface, in bubble radii */
uniform float u_MassLobes;	/* how far its lobes reach in from that, in bubble radii */
uniform float u_MassScale;	/* its billows, cycles per bubble radius */
uniform float u_MassDensity;	/* its density */
uniform float u_MassWarp;	/* how far the fold warp bends it, 0..1 */
uniform float u_MassFine;	/* fine lumps on its surface, 0 none */
uniform float u_MassBillow;	/* how deep its lumps are: 1 as judged, 0 a smooth sphere */
uniform float u_MassEdge;	/* how hard its surface is, 0..1 */
uniform float u_MassEdgePatch;	/* 0 hard all over, 1 only in patches */
/* Each bubble's squeeze along its own axes, 1 or more.  See struct Bubble. */
uniform vec4 u_BubbleForm[NSKY_MAX_BUBBLES];
uniform float u_BubbleDensity[NSKY_MAX_BUBBLES];	/* multiplies each bubble's gas */
uniform vec3 u_BubbleVeil[NSKY_MAX_BUBBLES];	/* what of its light passes the dust in front */
uniform float u_DetailScale;	/* the coarsest detail octave, cycles per bubble radius */
uniform float u_DetailGain;	/* amplitude kept per octave */
uniform float u_Erosion;	/* how far detail eats into the gas, 0..1 */
uniform float u_Filament;	/* 0 billowed detail, 1 ridged filaments */
uniform float u_Hardness;	/* 0 soft eroded edges, 1 abrupt ones */
uniform float u_Contrast;	/* spread of the gas's column density; 0 is uniform */
uniform float u_ClusterSize;	/* a cluster's radius, bubble radii; 0 a point */

/* How big a sample is, in bubble radii: the finest detail it can hold without aliasing is
 * about twice this.  Set by nsky_density() for the view march; the light pass leaves it at
 * its default, which is coarse, since the light volume holds nothing finer anyway. */
float nsky_footprint = 0.02;


/* The weight an octave of this feature size gets at the current footprint: 1 well above it,
 * fading to 0 at twice it.  The same rule nsky_detail() uses. */
float nsky_octave(float feature)
{
	return clamp(feature / max(2.0 * nsky_footprint, 1e-6) - 1.0, 0.0, 1.0);
}

/* What the coarse field says about a point. */
struct nsky_gas {
	vec3 q;		/* the warped point, which the detail is read at too */
	float mass;	/* the mass's own gas */
	float cavity;	/* the glow of the cavity */
	float near_edge;	/* 1 at a hard surface, where strides must be short */
	float near_width;	/* the surface's thickness, which sets how short */
};

/* One fetch: four independent single octave noises, each about -0.7..0.7. */
vec4 nsky_noise(vec3 p)
{
	return texture(u_Noise, p * (1.0 / NSKY_NOISE_PERIOD)) * 2.0 - 1.0;
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

	if ((b > 0 ? u_DistantBlister : u_Blister) <= 0.0 || len < 1e-4)
		return 1.0;
	side = dot(dir, axis / len) + 0.6 * rag;
	/* 0 opens nothing, 1 leaves a cap about a third of the sky across. */
	edge = mix(-1.9, 0.45, b > 0 ? u_DistantBlister : u_Blister);
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
	float r, keep, contrast_noise, open, rin, clump;

	/* Displace the point by the fold's swirl, as warping where the gas is measured buckles
	 * it -- but only a little: a mass is modelled, not folded, and the swirl at full strength
	 * covers it in deep waves, where the references' masses are blocked out like clay. */
	g.q = p + w * shape.y * u_MassWarp;
	g.near_edge = 0.0;
	g.near_width = 1.0;
	r = length(g.q * u_BubbleForm[b].xyz);
	/* A distant mass's outline pushed in and out by a low noise of direction, well over a
	 * third either way: a lobed cloud, not a disc.  Of direction, since it is only seen from
	 * outside, where the prisms that makes are never seen along. */
	if (b > 0) {
		vec3 lo = normalize(g.q) * 1.4 + o * 0.61;

		r /= 1.0 + 0.6 * (nsky_noise(lo).x + 0.5 * nsky_noise(lo * 2.3).y);
	}

	n = nsky_noise(g.q * u_HoleScale + o * 0.37);
	/* A thick mass of rounded lumps round the glowing cavity.
	 *
	 * The reference skyboxes surround the eye with cloud: stretched, their darkest sky is the
	 * faintly lit faces of billows almost everywhere, with the light breaking through in a
	 * few places.  A thin shell cannot be that -- its holes show the smooth haze behind, not
	 * more cloud, and every part of it is seen lit from the eye's side.  So the gas fills a
	 * thick layer from an inner surface out to the bubble's edge.  The inner surface is
	 * pushed in by a low noise into lobes that reach over the cavity, in front of its glow,
	 * as part of the one mass and not as clouds of their own.  Its lumps are a smooth noise
	 * of a few octaves, in the density and on the surface.  Cleared about the viewer: gas at
	 * the eye is fog over the whole sky. */
	{
		vec3 view = u_BubbleRot[b] * (-u_BubbleSphere[b].xyz / u_BubbleSphere[b].w);
		/* The lobes from a noise of the point, not of its direction alone: of the direction,
		 * the inner surface moves in and out the same all along each ray from the centre,
		 * and every lobe is a prism standing on it, straight-sided, that from off the
		 * centre shows as streaks and sawteeth.  Of the point, a lobe is a rounded mass. */
		vec3 lq = g.q * 1.6 + o * 0.23, bq = g.q * u_MassScale + o * 0.41;
		float lobe = nsky_noise(lq).x + 0.5 * nsky_noise(lq * 2.1 + 3.7).y;
		float billow = 0.0, amp = 1.0, dens, h = u_MassEdge;
		int k;

		/* Smooth lumps, two octaves: the absolute value of a noise, which is round lumps
		 * with creases between, reads as cauliflower, and more octaves as crumple. */
		for (k = 0; k < 2; k++) {
			billow += amp * 1.6 * nsky_noise(bq).x;
			amp *= 0.4;
			bq = bq * 2.07 + 7.3;
		}
		/* And finer lumps on those, each octave a smaller displacement of the surface the
		 * larger ones made -- carved into the form, not sprinkled through the gas -- so a big
		 * smooth bulge still has a skin of small ones for a grazing light to pick out.  As
		 * many octaves as the sample can hold; see nsky_octave().  Not on a distant one: at
		 * a few degrees across they are specks. */
		if (u_MassFine > 0.0 && b == 0) {
			float fine = 0.0, fa = 1.0, feature = 1.0 / (u_MassScale * 4.28);

			for (k = 0; k < 4; k++) {
				fine += fa * nsky_octave(feature) * nsky_noise(bq).y;
				fa *= 0.5;
				feature *= 0.483;
				bq = bq * 2.07 + 3.1;
			}
			billow += u_MassFine * 0.35 * fine;
		}
		billow *= u_MassBillow;
		rin = clamp(u_MassInner - u_MassLobes * max(lobe + 0.15, 0.0), 0.1, 0.95);
		/* How hard the surface is.  Soft, the gas thins over a good part of a lump's width
		 * and every edge is a fade -- seen whole, the sky reads well, but at a game's field
		 * of view, a few tens of degrees, the form is vague.  Hard, the same two ramps close
		 * to a narrow band about the same place, and the lumps end in crisp silhouettes and
		 * lit rims.  In patches, by a noise a few lumps across, some of the mass is crisp and
		 * the rest soft beside it, as in the references. */
		if (h > 0.0 && u_MassEdgePatch > 0.0)
			h *= mix(1.0, smoothstep(-0.12, 0.12, nsky_noise(g.q * 0.9 + o * 0.53).z),
				u_MassEdgePatch);
		dens = smoothstep(mix(-0.02, 0.03, h), mix(0.1, 0.05, h), r - rin + 0.12 * billow) *
			(1.0 - smoothstep(0.92, 1.05, r - 0.06 * billow));
		dens *= smoothstep(mix(-0.2, 0.06, h), mix(0.4, 0.14, h), billow + 0.2);
		/* A hard surface is thinner than the march's stride, and whether a texel's step
		 * lands on it or over it is its jitter's say -- grain along every crisp edge.  So
		 * near it the strides shorten. */
		if (h > 0.0) {
			g.near_edge = h * max(1.0 - smoothstep(0.15, 0.35, abs(billow + 0.1)),
					1.0 - smoothstep(0.05, 0.1, abs(r - rin + 0.12 * billow - 0.04)));
			g.near_width = 0.04;
		}
		dens *= smoothstep(0.1, 0.3, length(p - view));
		/* And about each cluster, whose winds have blown their surroundings clear: a star
		 * buried in the mass lights nothing we can see. */
		for (k = 0; k < NSKY_CLUSTERS; k++) {
			vec4 cl = u_Cluster[b * NSKY_CLUSTERS + k];

			if (cl.w > 0.0)
				dens *= smoothstep(0.12, 0.35, length(p - cl.xyz));
		}
		g.mass = dens * u_MassDensity * (b == 0 ? 1.0 : u_DistantDensity);
	}
	/* Contrast ACROSS the mass.  Gas of even density is about one optical depth through
	 * wherever it is looked at, so the whole sky comes out half veiled -- fog, with nothing
	 * either clear or solid.  Real interstellar gas has a lognormal column density: most
	 * sight lines thin, a few very thick.  So the density is multiplied by the exponential
	 * of a smooth noise, at the scale of the holes; the detail inside is left alone.
	 * Thresholding the fine erosion instead cuts the gas into flat-bottomed blots, which is
	 * the decal look.  The divisor keeps the mean where it was.
	 *
	 * The thick end is capped.  Uncapped it reaches over ten times the mean density, where
	 * the lit ionisation front is far thinner than any step: a texel whose step lands right
	 * at the surface gets the front, one whose step lands a little deeper gets gas the light
	 * never reached, and the thickest patches come out peppered black.  The clear gaps come
	 * from the thin end, which is left alone. */
	clump = min(exp(u_Contrast * 2.5 * n.z) / exp(0.03 * u_Contrast * u_Contrast * 6.25),
			NSKY_CONTRAST_CAP);
	g.mass *= clump;
	contrast_noise = n.w;

	/* Holes.  keep is the fraction that survives, mapped onto the noise's range; a hole
	 * is where the distant sky shows through, and the layering that gives is half the
	 * depth in the picture. */
	keep = smoothstep(0.95 - 1.2 * shape.z, 1.07 - 1.2 * shape.z, n.x + 0.5);
	open = nsky_blister(b, normalize(g.q), n.x);
	keep *= open;
	g.mass *= keep;

	/* The ionised gas glows in the cavity, inside the inner surface, lobes and all: the
	 * clusters' winds have blown their surroundings clear, which is the cavity the Rosette's
	 * centre is. */
	g.cavity = u_CavityDensity * smoothstep(0.1, 0.6, r / rin) *
			(1.0 - smoothstep(0.9, 1.05, r / rin));
	/* A distant nebula's glow ends at its inner surface, in an edge, not a fade -- or it
	 * reads as a piece cut from a larger nebula -- with a layer bright just inside it, the
	 * ionisation front, which seen edge on rims it; and inside it is gathered into
	 * filaments and knots, the ridges of two noises (distant, blind, rounds 1-2). */
	if (b > 0) {
		vec3 fq = g.q * 3.2 + o * 0.29;
		float x = r / rin;
		float f1 = 1.0 - abs(nsky_noise(fq).y), f2 = 1.0 - abs(nsky_noise(fq * 2.4 + 5.3).z);

		g.cavity = u_DistantCavity * smoothstep(0.05, 0.5, x) * (1.0 - smoothstep(0.97, 1.0, x)) *
				(0.3 + 2.0 * pow(f1, 6.0) + 1.0 * pow(f2, 6.0) +
				 5.0 * exp(-(x - 0.95) * (x - 0.95) / 0.0006));
	}
	/* Clumped the same way as the mass, from a noise of its own: the ionised gas glows in
	 * patches, so it does not lay a veil over every gap. */
	g.cavity *= exp(u_Contrast * 2.5 * contrast_noise) / exp(0.03 * u_Contrast * u_Contrast * 6.25);
	/* The ionised gas lines the wall, so it goes where the wall goes. */
	g.cavity *= open;
	return g;
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
		float w = clamp(feature / max(2.0 * footprint, 1e-6) - 1.0, 0.0, 1.0);

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
	total = g.mass + g.cavity;
	if (total < 0.003)
		return 0.0;
	/* The gain on the detail is the edge hardness: steeper, and the erosion goes from
	 * nothing to everything over a narrower band of the noise, so what survives has a
	 * crisp edge -- a hard lit rim rather than the soft edge of smoke. */
	e = clamp(0.5 + mix(2.2, 9.0, u_Hardness) * nsky_detail(b, g.q, footprint), 0.0, 1.0);
	/* Erode from the edges in.  Subtracting the detail's complement and renormalising
	 * leaves the dense heart of the mass nearly whole and eats hardest where it is already
	 * thin, which is what makes the edges wispy rather than the whole mass uniformly
	 * speckled.  The (0.4 + e) then puts knots and voids inside what survives, so the dense
	 * parts are not flat. */
	cut = u_Erosion * (1.0 - e);
	/* Eroded as if no denser than 1, then scaled back up, so that dense gas -- a clump the
	 * contrast has thickened -- gets the same ragged edge as the rest instead of a cut too
	 * small to show on it. */
	dense = max(g.mass, 1.0);
	g.mass /= dense;
	/* The cavity is left out of the erosion.  It is a hundredth of the mass's density, so
	 * any cut at all removes it entirely -- and it is the hot, fully ionised gas where the
	 * [O III] lives, the soft glow behind the lumps.  And it is left smooth: patchy only at
	 * the large scale, from the contrast.  Given the fine detail too, it varies faster than
	 * any stride through the cavity can follow, and every change of stride shows in it as a
	 * seam or a ripple. */
	return (max(g.mass - cut, 0.0) / max(1.0 - cut, 0.05) * (0.4 + e) * dense +
		g.cavity) * u_BubbleDensity[b];
}
