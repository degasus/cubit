varying vec4 pos;
uniform vec4 background;
uniform float time;

void main(void)
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	pos = gl_ModelViewMatrix * gl_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}