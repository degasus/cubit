uniform float time;

attribute float normal;

varying vec4 pos;
varying vec3 normals;

void main(void)
{
	if(normal == 0.0)
		normals = vec3(1.0, 0.0, 0.0);
	else if(normal == 1.0)
		normals = vec3(-1.0, 0.0, 0.0);
	else if(normal == 2.0)
		normals = vec3(0.0, 1.0, 0.0);
	else if(normal == 3.0)
		normals = vec3(0.0, -1.0, 0.0);
	else if(normal == 4.0)
		normals = vec3(0.0, 0.0, 1.0);
	else
		normals = vec3(0.0, 0.0, -1.0);
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
	pos = gl_ModelViewMatrix * gl_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}