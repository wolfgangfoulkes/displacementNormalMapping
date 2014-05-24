//this shader will render a displacement map to a floating point texture, updated every frame

#version 120

void main()
{
	// simply pass position and texture coordinate on to the fragment shader
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_MultiTexCoord0;
}