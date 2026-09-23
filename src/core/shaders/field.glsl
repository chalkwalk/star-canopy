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


uniform float u_FoldScale;	/* fold warp frequency, cycles per bubble radius */
uniform float u_OuterSharpness;	/* how much harder the shell's outer edge is than its inner */
uniform float u_HoleScale;	/* frequency of the holes and thickness variation */
uniform float u_CavityDensity;	/* faint gas filling the cavity */
uniform float u_DetailScale;	/* the coarsest detail octave, cycles per bubble radius */
uniform float u_DetailGain;	/* amplitude kept per octave */
uniform float u_Erosion;	/* how far detail eats into the shell, 0..1 */
uniform float u_Filament;	/* 0 billowed detail, 1 ridged filaments */
uniform float u_Hardness;	/* 0 soft eroded edges, 1 abrupt ones */
uniform float u_Contrast;	/* spread of the shell's column density; 0 is uniform */
uniform float u_DustAmount;	/* how much of the shell is dark molecular cloud, 0..1 */
uniform float u_DustScale;	/* size of the dark lanes, cycles per bubble radius */
uniform float u_DustOpacity;	/* extra extinction of the dust */

/* How big a sample is, in bubble radii: the finest detail it can hold without aliasing is
 * about twice this.  Set by nsky_density() for the view march; the light pass leaves it at
 * its default, which is coarse, since the light volume holds nothing finer anyway. */
float nsky_footprint = 0.02;



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
	g.min_feature = 0.0;
	g.near_pillar = 0.0;
	g.near_width = 1.0;
	g.core = 0.0;
	r = length(g.q);

	n = nsky_noise(g.q * u_HoleScale + o * 0.37);
	thick = shape.x * (0.6 + 0.8 * clamp(n.y + 0.5, 0.0, 1.0));
	s = (r - 1.0) / thick;
	/* Harder on the outside than the in.  Vela's filaments have a sharp edge away from the
	 * remnant's centre and fade over several times that toward it, which is the shock
	 * running outward into cold gas with the hot interior behind it. */
	g.shell = exp(-s * s * (s > 0.0 ? u_OuterSharpness : 1.0));
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

	/* Dark lanes: the ridges of one noise, where it crosses zero, broken into lengths by a
	 * second.  Lanes and not blobs, because a real HII region's dark clouds are threaded
	 * across the glow -- a round black blot reads as a hole in the sky, not as something
	 * in front of it.  Only within the shell's body, never out in its wisps. */
	n = nsky_noise(g.q * u_DustScale + o * 0.71);
	lane = 1.0 - abs(n.z) * 4.0;
	g.dust = u_DustAmount * smoothstep(0.2, 0.8, lane) * smoothstep(-0.15, 0.15, n.w) *
		(1.0 - smoothstep(0.5, 2.0, s * s));
	g.shell *= keep * (1.0 + 3.0 * g.dust);


	/* The ionised gas inside the shell sits in a layer against it, not in a ball on the
	 * stars: the clusters' winds have blown their surroundings clear, which is the cavity
	 * the Rosette's centre is.  Filling the middle instead puts the densest gas where the
	 * light is fiercest, and it burns out to white. */
	g.cavity = u_CavityDensity * smoothstep(0.35, 0.95, r) * (1.0 - smoothstep(0.95, 1.1, r));
	/* Clumped the same way as the shell, from a noise of its own: the ionised gas glows in
	 * patches, so it does not lay a veil over every gap in the shell. */
	g.cavity *= exp(u_Contrast * 2.5 * contrast_noise) / exp(0.03 * u_Contrast * u_Contrast * 6.25);
	return g;
}

/* How much of its glow gas with this much dust keeps, 0..1.  Molecular cloud is not ionised
 * through, so a lane glows only at its partly dusty edges.  Flat lanes stop glowing as soon
 * as they are substantially dusty, which is the flat dark; soft veins keep glow in proportion
 * to how far below the centre line's dust they are, so they are dark only along it. */
float nsky_dust_glow(float dust)
{
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
		g.cavity);
}
