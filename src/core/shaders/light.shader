/* The light volume: for every voxel of a bubble's box, the optical depth of
 * shell between it and each of the bubble's clusters.
 *
 * Marched once per voxel here instead of once per sample in the bake, which is the saving
 * that makes lighting affordable at all: a few hundred thousand voxels times one march,
 * against tens of millions of view samples times one march each.  It sees only the coarse
 * shell -- at this resolution that is all there is to see -- and the result is what puts
 * the ionisation front on the side of every fold facing the cluster.
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

/* Only the SHELL shadows the ultraviolet.  The cavity's fill is hot gas the cluster has
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

		tau += g.shell;
	}
	return tau * dt * u_Sigma * u_BubbleDensity[u_Bubble];
}


void main()
{
	vec3 uvw = vec3(gl_FragCoord.xy / u_Res, u_Slice);
	vec3 p = (uvw * 2.0 - 1.0) * u_Bound;
	int first = u_Bubble * NSKY_CLUSTERS;

	f_FragColor = vec4(depth_to(p, u_Cluster[first]),
				depth_to(p, u_Cluster[first + 1]),
				depth_to(p, u_Cluster[first + 2]), 0.0);
}

#endif
