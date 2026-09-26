/* The galaxy's density at given points, for test_galaxy: galaxy.glsl's gal_density() and
 * galaxy.cpp's galaxyDensity() are one specification, and the test holds them to it
 * (PRINCIPLES section 13).  Part of no bake.  Needs field.glsl and galaxy.glsl in front of it.
 */

#if defined(INCLUDE_VS)

void main()
{
	vec2 corner[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

	gl_Position = vec4(corner[gl_VertexID], 0.0, 1.0);
}

#endif

#if defined(INCLUDE_FS)

#define PROBE_POINTS 64

uniform vec3 u_Points[PROBE_POINTS];

out vec4 f_FragColor;

void main()
{
	gal_sample s = gal_density(u_Points[int(gl_FragCoord.x)]);

	f_FragColor = vec4(s.old, s.young, s.dust, 1.0);
}

#endif
