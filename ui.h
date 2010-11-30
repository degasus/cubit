#ifndef _UI_H_
#define _UI_H_

class UInterface;

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

/**
 *
 *
 */
class UInterface {
public:
	/**
	 *
	 *
	 */
	UInterface();

	/**
	 *
	 */



private:
	void initGL();

	//Frame conditions
	//current frame size
	int screenX;
	int screenY;
	//fullscreen on/off
	bool isFullscreen;

	//SDL vars
	SDL_Surface *screen;
	bool done;
};


#endif
