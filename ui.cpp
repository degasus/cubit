#include "ui.h"

UInterface::UInterface()
{
	done = 0;
	catchMouse = 1;
}

void UInterface::init()
{
	// Slightly different SDL initialization
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 ) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}
	
	initWindow();
	
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	SDL_ShowCursor(SDL_DISABLE);
}

void UInterface::config(const boost::program_options::variables_map &c)
{
	noFullX 			= c["noFullX"].as<int>();
	noFullY 			= c["noFullY"].as<int>();
	isFullscreen 		= c["fullscreen"].as<bool>();
}


void UInterface::initWindow()
{
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

}


void UInterface::run()
{
	SDL_Event event;

	while((!done) && (SDL_WaitEvent(&event))) {
		switch(event.type) {
			case SDL_USEREVENT:			handleUserEvents(event.user);			break;
			case SDL_KEYDOWN:			handleKeyDownEvents(event.key); 		break;
			case SDL_KEYUP:				handleKeyUpEvents(event.key); 			break;
			case SDL_MOUSEBUTTONDOWN: 	handleMouseDownEvents(event.button); 	break;
			case SDL_MOUSEBUTTONUP: 	handleMouseUPEvents(event.button); 		break;
			case SDL_MOUSEMOTION: 		handleMouseEvents(event.motion); 		break;
			case SDL_QUIT: 				done = true;  							break;

			case SDL_VIDEORESIZE:
				screenX = event.resize.w;
				screenY = event.resize.h;
				initWindow();
				break;
		}
	}
}

void UInterface::handleKeyDownEvents(SDL_KeyboardEvent e)
{
	switch(e.keysym.sym){
		
	}
}

void UInterface::handleKeyUpEvents(SDL_KeyboardEvent e)
{
	switch(e.keysym.sym){
		
	}
}

void UInterface::handleUserEvents(SDL_UserEvent e)
{
	
//	debug();

}

void UInterface::handleMouseDownEvents(SDL_MouseButtonEvent e)
{

}

void UInterface::handleMouseUPEvents(SDL_MouseButtonEvent e)
{

}

void UInterface::handleMouseEvents(SDL_MouseMotionEvent e)
{
	if(catchMouse) {
		int x = e.x-screenX/2;
		int y = e.y-screenY/2;
		
		SDL_WarpMouse(screenX/2, screenY/2);
	}
}



