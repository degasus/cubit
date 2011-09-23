uniform sampler3D tex;
uniform vec4 bgColor;
uniform float time;
uniform float visualRange;
uniform float fogStart;
uniform vec3 position;

// lightning
uniform vec4 LightAmbient;
uniform vec3 LightDiffuseDirectionA;
uniform vec3 LightDiffuseDirectionB;

varying vec3 pos;
varying vec3 normals;

float abstand;
float sichtbarkeit;
vec4 helligkeit;
vec4 texture;

void main (void)
{
	abstand = length(pos-position); 
	if(abstand < fogStart) {
		sichtbarkeit = 1.0;
	} else if(abstand < visualRange){
		sichtbarkeit = (visualRange-abstand)/(visualRange-fogStart);
	} else {
		sichtbarkeit = 0.0;
	} 
	
	helligkeit = LightAmbient;
	helligkeit = helligkeit + clamp(dot(LightDiffuseDirectionA,normals),0.0,0.5);
	helligkeit = helligkeit + clamp(dot(LightDiffuseDirectionB,normals),0.0,0.5);
	helligkeit = clamp(helligkeit, 0.0, 1.0);
	
	texture = texture3D(tex,gl_TexCoord[0].xyz) * helligkeit;
	gl_FragColor = mix(bgColor, texture, sichtbarkeit);
}