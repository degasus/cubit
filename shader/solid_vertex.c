uniform float time;

attribute float normal;
attribute vec3 bPos;
attribute vec3 tPos;

varying vec3 pos;
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
	
	normals = (gl_ModelViewMatrix * vec4(normals,0.0)).xyz;
	
	gl_TexCoord[0] = vec4(tPos,0.0);
	pos = (gl_ModelViewMatrix * vec4(bPos,1.0)).xyz;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(bPos,1.0);
}
