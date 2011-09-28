uniform float time;

attribute vec4 bPos;
attribute vec4 tPos;

varying vec3 pos;
varying vec3 normals;
varying vec3 texPos;

void main(void)
{
	if(tPos[3] == 0.0)
		normals = vec3(1.0, 0.0, 0.0);
	else if(tPos[3] == 1.0)
		normals = vec3(-1.0, 0.0, 0.0);
	else if(tPos[3] == 2.0)
		normals = vec3(0.0, 1.0, 0.0);
	else if(tPos[3] == 3.0)
		normals = vec3(0.0, -1.0, 0.0);
	else if(tPos[3] == 4.0)
		normals = vec3(0.0, 0.0, 1.0);
	else
		normals = vec3(0.0, 0.0, -1.0);
	
	normals = (gl_ModelViewMatrix * vec4(normals,0.0)).xyz;
	
	texPos = tPos.xyz;
	
	pos = (gl_ModelViewMatrix * bPos).xyz;
	gl_Position = gl_ModelViewProjectionMatrix * bPos;
}
