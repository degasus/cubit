#include "renderer.h"

using namespace std;


Renderer::Renderer()
{ }

void Renderer::config(const boost::program_options::variables_map& c)
{
	bgColor[0] 			= c["bgColorR"].as<float>();
	bgColor[1] 			= c["bgColorG"].as<float>();
	bgColor[2] 			= c["bgColorB"].as<float>();
	bgColor[3] 			= c["bgColorA"].as<float>();
	fogDense				= c["fogDense"].as<float>();
	fogStartFactor		= c["fogStartFactor"].as<float>();
	visualRange			= c["visualRange"].as<float>();
	
	string textureDirectory = c["textureDirectory"].as<string>();
	Texture_Files[1]	= textureDirectory + "/" + c["texture01"].as<string>();
	Texture_Files[2]	= textureDirectory + "/" + c["texture02"].as<string>();
	Texture_Files[3]	= textureDirectory + "/" + c["texture03"].as<string>();
	Texture_Files[4]	= textureDirectory + "/" + c["texture04"].as<string>();
	
}


void Renderer::init()
{
	// Set the OpenGL state 
	glEnable(GL_TEXTURE_2D);											// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);											// Enable Smooth Shading
	glClearColor(bgColor[0],bgColor[1], bgColor[2], bgColor[3]);	// Black Background
	glClearDepth(1.0f);													// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);											// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);												// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);					// Really Nice Perspective Calculations
	glHint(GL_LINE_SMOOTH, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	

	GLfloat LightAmbient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat LightPosition[] = { 0.5f, 1.0f, 5.0f, 1.0f };
	
	glLightfv(GL_LIGHT1, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);
	
	glFogi(GL_FOG_MODE, GL_LINEAR);		// Fog Mode
	glFogfv(GL_FOG_COLOR, bgColor);	// Set Fog Color
	glFogf(GL_FOG_DENSITY, fogDense);	// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_DONT_CARE);	// Fog Hint Value
	glFogf(GL_FOG_START, visualRange*fogStartFactor);
	glFogf(GL_FOG_END, visualRange);
	glEnable(GL_FOG);					// Enables GL_FOG 
}