#ifndef _RENDERER_H_
#define _RENDERER_H_

class Renderer;

#include <boost/program_options.hpp>
#include "movement.h"
#include "map.h"
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <string>

/**
 *
 */
class Renderer {
public:
	/**
	 *
	 */
	Renderer();
	
	void init();

	void render(PlayerPosition pos);
	void deleteArea(Area* area);
	
	void config(const boost::program_options::variables_map &c);

private:
	GLuint texture[NUMBER_OF_MATERIALS];
	std::string Texture_Files[NUMBER_OF_MATERIALS];
	
	//fog
	GLfloat bgColor[4];		// Fog Color
	float fogDense;
	float fogStartFactor;
	float visualRange;
};

#endif