#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <SDL.h>
#include <SDL_opengl.h>
#include <string>

#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletDynamicsCommon.h>

#ifndef _RENDERER_H_
#define _RENDERER_H_

class Renderer;

#include "controller.h"

/**
 *
 */
class Renderer {
public:
	/**
	 *
	 */
	Renderer(Controller *controller);
	
	void init();

	void render(PlayerPosition pos);
	void deleteArea(Area* area);

	void highlightBlockDirection(BlockPosition, DIRECTION);
	
	void config(const boost::program_options::variables_map &c);
	GLuint texture[NUMBER_OF_MATERIALS];

	PlayerPosition itemPos;
	GLuint texture_item;
	GLuint gllist_item;
	btTriangleMesh triangles_item;

private:
	
	void renderArea(Area* area, bool show);
	bool areaInViewport(BlockPosition apos, PlayerPosition ppos);
	void generateViewPort(PlayerPosition pos);

	Matrix<double,3,3> viewPort;
	
	
	int areasPerFrame;
	int areasRendered;
	int maxareas;
	bool enableFog;
	
	boost::filesystem::path workingDirectory;
	boost::filesystem::path dataDirectory;
	boost::filesystem::path localDirectory;
	
	
	//fog
	GLfloat bgColor[4];		// Fog Color
	float fogDense;
	float fogStartFactor;
	float visualRange;
	
	Controller *c;

	bool highlightWholePlane;
	int textureFilterMethod;
	 
};


struct polygon {
	BlockPosition pos;
	DIRECTION d;
};

#endif
