/* The light volume: for every voxel of a bubble's box, the optical depth of
 * gas between it and each of the bubble's clusters.
 *
 * Marched once per voxel here instead of once per sample in the bake, which is the saving
 * that makes lighting affordable at all: a few hundred thousand voxels times one march,
 * against tens of millions of view samples times one march each.  It sees only the coarse
 * mass -- at this resolution that is all there is to see -- and the result is what puts
 * the ionisation front on the side of every lump facing the cluster.
 *
 * One channel per cluster, up to three.  Drawn one slice of the 3D texture at a time, as a
 * full screen triangle into that layer.
 */

#if defined(INCLUDE_VS)

void main()
{
	vec2 corner[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

	gl_Position = vec4(corner[gl_VertexID], 0.0, 1.0);
}

#endif

#if defined(INCLUDE_FS)

uniform int u_Bubble;
uniform float u_Res;		/* voxels per side */
uniform float u_Slice;		/* this layer's z, 0..1, at the voxel centre */
uniform float u_Bound;		/* the box is -bound..bound in the bubble frame */
uniform int u_LightSteps;
uniform float u_Sigma;		/* ionising optical depth per bubble radius at density 1 */

out vec4 f_FragColor;

/* Only the MASS shadows the ultraviolet.  The cavity's fill is hot gas the cluster has
 * already ionised, which is to say transparent to it; counting it would dim every wall by
 * the width of the cavity, and a bubble lit from inside would be dark all round. */
float depth_to(vec3 p, vec4 cluster)
{
	vec3 to_cluster = cluster.xyz - p;
	float dt = length(to_cluster) / float(u_LightSteps);
	float tau = 0.0;
	int i;

	if (cluster.w <= 0.0)
		return 0.0;
	for (i = 0; i < u_LightSteps; i++) {
		nsky_gas g = nsky_coarse(u_Bubble, p + to_cluster *
				((float(i) + 0.5) / float(u_LightSteps)));

		tau += g.mass;
	}
	return tau * dt * u_Sigma * u_BubbleDensity[u_Bubble];
}

/* Depth to a cluster of stars, not a point.  From a point every lump casts a long, straight,
 * hard shadow, which in a thick mass seen side on is a streak across the sky; a young cluster
 * is dozens of stars spread over a region, and its shadows have penumbrae that widen with
 * distance.  So the light arriving from five points over it -- its centre and a tetrahedron
 * about that -- is averaged, the light and not the depth, and the depth of that average kept. */
float depth_to_cluster(vec3 p, vec4 cluster)
{
	const vec3 corner[4] = vec3[4](vec3(1.0, 1.0, 1.0), vec3(1.0, -1.0, -1.0),
					vec3(-1.0, 1.0, -1.0), vec3(-1.0, -1.0, 1.0));
	vec3 h, axis;
	float light, a;
	int k;

	if (u_ClusterSize <= 0.0 || cluster.w <= 0.0)
		return depth_to(p, cluster);
	/* The tetrahedron turned a different way in every voxel: five fixed points cast five
	 * hard shadows, offset copies that stack into stripes; turned at random they fall in a
	 * different place in each voxel, which the bake's lookup then averages into one soft
	 * penumbra.  See light_depth() there. */
	h = fract(sin(vec3(dot(gl_FragCoord.xy, vec2(12.9898, 78.233)) + u_Slice * 311.7,
				dot(gl_FragCoord.yx, vec2(39.346, 11.135)) + u_Slice * 173.3,
				dot(gl_FragCoord.xy, vec2(73.156, 52.235)) + u_Slice * 97.1)) * 43758.5453);
	axis = normalize(h * 2.0 - 1.0 + vec3(1e-3));
	a = h.x * 6.2831853;
	light = exp(-depth_to(p, cluster));
	for (k = 0; k < 4; k++) {
		vec3 c = corner[k] * 0.577;

		/* Rodrigues: c turned by a about axis. */
		c = c * cos(a) + cross(axis, c) * sin(a) + axis * dot(axis, c) * (1.0 - cos(a));
		light += exp(-depth_to(p, vec4(cluster.xyz + c * u_ClusterSize, cluster.w)));
	}
	return -log(max(light / 5.0, 1e-12));
}

void main()
{
	vec3 uvw = vec3(gl_FragCoord.xy / u_Res, u_Slice);
	vec3 p = (uvw * 2.0 - 1.0) * u_Bound;
	int first = u_Bubble * NSKY_CLUSTERS;

	f_FragColor = vec4(depth_to_cluster(p, u_Cluster[first]),
				depth_to_cluster(p, u_Cluster[first + 1]),
				depth_to_cluster(p, u_Cluster[first + 2]), 0.0);
}

#endif
