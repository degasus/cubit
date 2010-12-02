#include <iostream>

#include "controller.h"
#include "map.h"
using namespace std;


Renderer::Renderer(Controller* controller)
{ 
	c = controller;
	render_this_round = 1;
}

void Renderer::config(const boost::program_options::variables_map& c)
{
	bgColor[0] 			= c["bgColorR"].as<float>();
	bgColor[1] 			= c["bgColorG"].as<float>();
	bgColor[2] 			= c["bgColorB"].as<float>();
	bgColor[3] 			= c["bgColorA"].as<float>();
	fogDense				= c["fogDense"].as<float>();
	fogStartFactor		= c["fogStartFactor"].as<float>();
	visualRange			= c["visualRange"].as<float>();
	
	string textureDirectory = c["textureDirectory"].as<string>();
	Texture_Files[1]	= textureDirectory + "/" + c["texture01"].as<string>();
	Texture_Files[2]	= textureDirectory + "/" + c["texture02"].as<string>();
	Texture_Files[3]	= textureDirectory + "/" + c["texture03"].as<string>();
	Texture_Files[4]	= textureDirectory + "/" + c["texture04"].as<string>();
	
}


void Renderer::init()
{
	// Set the OpenGL state 
	glEnable(GL_TEXTURE_2D);											// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);											// Enable Smooth Shading
	glClearColor(bgColor[0],bgColor[1], bgColor[2], bgColor[3]);	// Black Background
	glClearDepth(1.0f);													// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);											// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);												// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);					// Really Nice Perspective Calculations
	glHint(GL_LINE_SMOOTH, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);
	
/*
	GLfloat LightAmbient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat LightPosition[] = { 0.5f, 1.0f, 5.0f, 1.0f };
	
	glLightfv(GL_LIGHT1, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);
	
	glFogi(GL_FOG_MODE, GL_LINEAR);		// Fog Mode
	glFogfv(GL_FOG_COLOR, bgColor);	// Set Fog Color
	glFogf(GL_FOG_DENSITY, fogDense);	// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_DONT_CARE);	// Fog Hint Value
	glFogf(GL_FOG_START, visualRange*fogStartFactor);
	glFogf(GL_FOG_END, visualRange);
	glEnable(GL_FOG);					// Enables GL_FOG 
*/


	glGenTextures( NUMBER_OF_MATERIALS, texture );
	for(int i=1; i<NUMBER_OF_MATERIALS; i++) {
		SDL_Surface *surface; // Gives us the information to make the texture

		if ( (surface = SDL_LoadBMP(Texture_Files[i].c_str())) ) {

			// Check that the image's width is a power of 2
			if ( (surface->w & (surface->w - 1)) != 0 ) {
				printf("warning: %s's width is not a power of 2\n", Texture_Files[i].c_str());
			}

			// Also check if the height is a power of 2
			if ( (surface->h & (surface->h - 1)) != 0 ) {
				printf("warning: %s's height is not a power of 2\n", Texture_Files[i].c_str());
			}

			// Bind the texture object
			glBindTexture( GL_TEXTURE_2D, texture[i] );

			// Set the texture's stretching properties
			//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );


			gluBuild2DMipmaps(GL_TEXTURE_2D, 3, surface->w, surface->h, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
		}
		else {
			printf("SDL could not load %s: %s\n", Texture_Files[i].c_str(), SDL_GetError());
			SDL_Quit();
		}

		// Free the SDL_Surface only if it was successfully created
		if ( surface ) {
			SDL_FreeSurface( surface );
		}
	}
	
}

void Renderer::renderArea(Area* area)
{
	
	if(!area->gllist_generated || area->needupdate) {
		
		if(!area->gllist_generated) {
			area->gllist_generated = 1;
			area->gllist = glGenLists(1);
		}
		area->needupdate = 0;
		
		glNewList(area->gllist,GL_COMPILE_AND_EXECUTE);
		
		Area* areas[DIRECTION_COUNT];
		for(int i=0; i<DIRECTION_COUNT; i++)
			areas[i] = 0;

		for(int i=1; i < NUMBER_OF_MATERIALS; i++){
			glBindTexture( GL_TEXTURE_2D, texture[i] );
			
			glBegin( GL_QUADS );
			for(int x=area->pos.x; x<AREASIZE_X+area->pos.x; x++) 
			for(int y=area->pos.y; y<AREASIZE_Y+area->pos.y; y++) 
			for(int z=area->pos.z; z<AREASIZE_Z+area->pos.z; z++)  {
				
				BlockPosition pos = BlockPosition::create(x,y,z);
				
				if(area->get(pos) == i){
					Material now = area->get(pos);;
					
					if(now) {
						for(int dir=0; dir < DIRECTION_COUNT; dir++) {
							BlockPosition next = pos+(DIRECTION)dir;
							
							Material next_m;
							if((*area) << next)
								next_m = area->get(next);
							else if(areas[dir] && (*areas[dir]) << next) 
								next_m = areas[dir]->get(next);
							else {
								areas[dir] = c->map.getArea(next);
								next_m = areas[dir]->get(next);
							}
							
							if(!next_m) {
								glNormal3f( NORMAL_OF_DIRECTION[dir][0], NORMAL_OF_DIRECTION[dir][1], NORMAL_OF_DIRECTION[dir][2]);					// Normal Pointing Towards Viewer
								for(int point=0; point < POINTS_PER_POLYGON; point++) {
									glTexCoord2f(
										TEXTUR_POSITION_OF_DIRECTION[dir][point][0],
										TEXTUR_POSITION_OF_DIRECTION[dir][point][1]
									); 
									glVertex3f(
										POINTS_OF_DIRECTION[dir][point][0]+(x-area->pos.x),
										POINTS_OF_DIRECTION[dir][point][1]+(y-area->pos.y),
										POINTS_OF_DIRECTION[dir][point][2]+(z-area->pos.z)
									);
								}
							}
						}
					}
				}
			}
		glEnd();
		}
		glEndList();
	} else {
		glCallList(area->gllist);
	}
}

double k=0;
void Renderer::render(PlayerPosition pos)
{
	// Clear the screen before drawing
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And The Depth Buffer
	glLoadIdentity();							// Reset The View
	
	//Mausbewegung
	glRotatef(pos.orientationHorizontal,0.0f,0.0f,1.0f);
	glRotatef(pos.orientationVertical,0.0f,1.0f,0.0f);

	
	k+=0.3;
	//Eigene Position
	glTranslatef(-(pos.x-20+k), -(pos.y), -(pos.z+AREASIZE_Z/2+2.5));

	
	for(int x=-(visualRange)+k; x<(visualRange)+k; x+=AREASIZE_X) 
	for(int y=-(visualRange); y<(visualRange); y+=AREASIZE_Y) 
	for(int z=-visualRange/2; z<visualRange/2; z+=AREASIZE_Z) {
		glPushMatrix();
		

		Area *area = c->map.getArea(BlockPosition::create(x,y,z));
		
		glTranslatef(area->pos.x,area->pos.y,area->pos.z);
		renderArea(area);
		
		glPopMatrix();
	}

	SDL_GL_SwapBuffers();
}

