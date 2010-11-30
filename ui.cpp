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