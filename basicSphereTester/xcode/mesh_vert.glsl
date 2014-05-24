// this shader will use the displacement and normal maps to displace vertices of a mesh
#version 120

uniform sampler2D displacement_map;

varying vec3 Nsurface;

void main()
{
	// lookup displacement in map
	float displacement = texture2D( displacement_map, gl_MultiTexCoord0.xy ).r;

	// now take the vertex and displace it along its normal
	vec4 V = gl_Vertex; //use to displace gl_position
	V.x += gl_Normal.x * displacement;
	V.y += gl_Normal.y * displacement;
	V.z += gl_Normal.z * displacement;

	// pass the surface normal on to the fragment shader
	Nsurface = gl_Normal;

	// pass vertex and texture coordinate on to the fragment shader
	gl_Position = gl_ModelViewProjectionMatrix * V; //displace gl_position by V put into eyespace.
	gl_TexCoord[0] = gl_MultiTexCoord0;
}