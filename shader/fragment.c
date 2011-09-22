//#version 120
//#extension GL_EXT_gpu_shader4 : enable
//#extension GL_EXT_texture_array : enable
uniform sampler3D tex;
uniform vec4 background;
uniform float time;
varying vec4 pos;

float abstand;
float sichtbarkeit;
vec4 texture;

void main (void)
{
	abstand = length(pos); 
	if(abstand < 100.0) {
		sichtbarkeit = 1.0;
	} else if(abstand < 125.0){
		sichtbarkeit = (125.0-abstand)/25.0;
	} else {
		sichtbarkeit = 0.0;
	} 
	
	texture = texture3D(tex,gl_TexCoord[0].xyz);
	gl_FragColor = texture * sichtbarkeit + background * (1.0 - sichtbarkeit);
}