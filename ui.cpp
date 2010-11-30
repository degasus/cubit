#include "ui.h"

UInterface::UInterface()
{
	done = 0;
	catchMouse = 1;
}

void UInterface::init()
{
	initGL();
}

void UInterface::config(const boost::program_options::variables_map &c)
{
	noFullX 			= c["noFullX"].as<int>();
	noFullY 			= c["noFullY"].as<int>();
	isFullscreen 	= c["fullscreen"].as<bool>();
}


void UInterface::initGL() {
	// Slightly different SDL initialization
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 ) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}

	if (isFullscreen) {
		const SDL_VideoInfo *vi = SDL_GetVideoInfo();
		screenX = vi->current_w;
		screenY = vi->current_h;
		screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_FULLSCREEN);
	} else {
		screenX = noFullX;
		screenY = noFullY;
		screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_RESIZABLE);
	}

	if ( !screen ) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
	}
	
	glViewport(0, 0, screenX, screenY);	// Reset The Current Viewport
	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	
	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat) screenX / (GLfloat) screenY, 0.01f, 1000.0f);
	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	SDL_ShowCursor(SDL_DISABLE);

 /* //generate Textures
  glGenTextures( numberOfTex, texture );

  for(int i = 1; i < numberOfTex; i++){
    loadTexture(texFiles[i], i);
  }

*/

//	SDL_TimerID timer = SDL_AddTimer(40,GameLoopTimer,0);

}