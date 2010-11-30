#ifndef _RENDERER_H_
#define _RENDERER_H_

class Renderer;

#include "movement.h"
#include "map.h"
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

//CONSTANTS
const char* TEXTURE_FILES[NUMBER_OF_MATERIALS] = {
	"",
	"tex/wood.bmp",
	"tex/bricks.bmp",
	"tex/grass.bmp",
	"tex/grass2.bmp",
	"tex/BlackMarble.bmp",
	"tex/BlueCrackMarble.bmp",
	"tex/BrightPurpleMarble.bmp",
	"tex/BrownSwirlMarble.bmp",
	"tex/SwirlyGrayMarble.bmp"
};

/**
 *
 */
class Renderer {
public:
	/**
	 *
	 */
	Renderer();

	void render(PlayerPosition pos);
	void deleteArea(Area* area);

private:
	GLuint texture[NUMBER_OF_MATERIALS];
	
	//fog
	GLfloat bgColor[4];		// Fog Color
	float fogDense;
	float fogStartFactor;
};

#endif