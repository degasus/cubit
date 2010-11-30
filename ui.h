#ifndef _UI_H_
#define _UI_H_

class UInterface;


#include <boost/program_options.hpp>
#include "renderer.h"

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
	void init();

	void config(const boost::program_options::variables_map &c);

private:
	void initGL();

	//Frame conditions
	//current frame size
	int screenX;
	int screenY;
	//default frame size on no fullscreen
	int noFullX;
	int noFullY;
	//fullscreen on/off
	bool isFullscreen;

	
	bool catchMouse;

	//SDL vars
	SDL_Surface *screen;
	bool done;
};


#endif
