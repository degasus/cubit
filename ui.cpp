#include "ui.h"

UInterface::UInterface()
{
	done = 0;
	initGL();
}


void UInterface::initGL() {
	// Slightly different SDL initialization
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 ) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}

	const SDL_VideoInfo *vi = SDL_GetVideoInfo();
	screenX = vi->current_w;
	screenY = vi->current_h;

	//screenX = 1366;
	//screenY = 786;

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_RESIZABLE | SDL_FULLSCREEN);
	if ( !screen ) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
	}

	// Set the OpenGL state after creating the context with SDL_SetVideoMode

	glEnable(GL_TEXTURE_2D);											// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);											// Enable Smooth Shading
	glClearColor(fogColor[0],fogColor[1], fogColor[2], fogColor[3]);	// Black Background
	glClearDepth(1.0f);													// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);											// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);												// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);					// Really Nice Perspective Calculations
	glHint(GL_LINE_SMOOTH, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);

	glViewport(0, 0, screenX, screenY);	// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat) screenX / (GLfloat) screenY, 0.01f, 1000.0f);

	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	glLoadIdentity();			// Reset The Modelview Matrix

	GLfloat LightAmbient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat LightPosition[] = { 0.5f, 1.0f, 5.0f, 1.0f };

	glLightfv(GL_LIGHT1, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);

	glFogi(GL_FOG_MODE, GL_LINEAR);		// Fog Mode
	glFogfv(GL_FOG_COLOR, fogColor);	// Set Fog Color
	glFogf(GL_FOG_DENSITY, fogDense);	// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_DONT_CARE);	// Fog Hint Value
	glFogf(GL_FOG_START, xsize*fogStartFactor);
	glFogf(GL_FOG_END, xsize);
  	glEnable(GL_FOG);					// Enables GL_FOG

	enable_rotate = 1;
	SDL_ShowCursor(SDL_DISABLE);
	x_orig = screenX/2;
	y_orig = screenY/2;

  //generate Textures
  glGenTextures( numberOfTex, texture );

  for(int i = 1; i < numberOfTex; i++){
    loadTexture(texFiles[i], i);
  }

  gen_land();
  
  // reserviert Speicher für eine OpenGL Liste
	box=glGenLists(1);
  gen_gllist();


	SDL_TimerID timer = SDL_AddTimer(40,GameLoopTimer,0);

}