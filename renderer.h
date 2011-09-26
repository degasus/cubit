#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>

#include <SDL.h>
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
	~Renderer();
	
	void init();

	void render(PlayerPosition pos, double eye = 0);
	void deleteArea(Area* area);

	void highlightBlockDirection(BlockPosition, DIRECTION);
	
	void config(const boost::program_options::variables_map &c);
	GLuint texture[NUMBER_OF_MATERIALS];
	
	std::string debug_output[2];
	int time;

private:
	
	void renderArea(Area* area, int l);
	void generateArea(Area* area);
	void renderObjects();
	bool areaInViewport(BlockPosition apos, PlayerPosition ppos);
	void generateViewPort(PlayerPosition pos);

	Matrix<double,3,3> viewPort;
	
	int areasPerFrame;
	int areasRendered;
	int maxareas;
	bool enableFog;
	double angleOfVision;
	
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
	
	
	float stats[4];
	int texture_size;
	
	// shader
	struct Shader {
		GLhandleARB solid_po;
		GLhandleARB solid_vs;
		GLhandleARB solid_fs;
		
		//uniform
		GLint position;
		GLint bgColor;
		GLint tex;
		GLint time;
		GLint visualRange;
		GLint fogStart;
		
		GLint LightAmbient;
		GLint LightDiffuseDirectionA;
		GLint LightDiffuseDirectionB;
	
		//attribute
		GLint normal;
	} shader;
	
	 
};


#endif
