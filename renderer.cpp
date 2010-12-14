#include <iostream>
#include <cmath>

#include "controller.h"
#include "map.h"
using namespace std;


Renderer::Renderer(Controller* controller)
{
	c = controller;
}

void Renderer::config(const boost::program_options::variables_map& c)
{
	bgColor[0] 			= c["bgColorR"].as<float>();
	bgColor[1] 			= c["bgColorG"].as<float>();
	bgColor[2] 			= c["bgColorB"].as<float>();
	bgColor[3] 			= c["bgColorA"].as<float>();
	fogDense			= c["fogDense"].as<float>();
	fogStartFactor		= c["fogStartFactor"].as<float>();
	visualRange			= c["visualRange"].as<float>();

	string textureDirectory = c["textureDirectory"].as<string>();
	Texture_Files[1]	= textureDirectory + "/" + c["texture01"].as<string>();
	Texture_Files[2]	= textureDirectory + "/" + c["texture02"].as<string>();
	Texture_Files[3]	= textureDirectory + "/" + c["texture03"].as<string>();
	Texture_Files[4]	= textureDirectory + "/" + c["texture04"].as<string>();
	Texture_Files[5]	= textureDirectory + "/" + c["texture05"].as<string>();

	areasPerFrame		= c["areasPerFrame"].as<int>();
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

	if((!area->gllist_generated || area->needupdate) && (areasRendered <= areasPerFrame)) {
		areasRendered++;
	
/*		std::cout << "render Area" << std::endl;
		if(area->needupdate)
			std::cout << "Area needs update" << std::endl;
		if(!area->gllist_generated )
			std::cout << "Area not generated" << std::endl;
*/
	
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
	} else if(area->gllist_generated) {
		glCallList(area->gllist);
	} else {
		// do nothing
	}
}


bool Renderer::areaInViewport(BlockPosition apos, PlayerPosition ppos) {
	bool left = 0;
	bool right = 0;
	
	for(int i=0; i<8; i++) {
		double diffx = apos.x - ppos.x + AREASIZE_X*(i&1);
		double diffy = apos.y - ppos.y + AREASIZE_Y*(i&2);
		double diffz = apos.z - ppos.z + AREASIZE_Z*(i&4);
		
		if(diffx*diffx + diffy*diffy + diffz*diffz > visualRange*visualRange*16*16)
			continue;
		
		double horizontal = (std::atan2(diffy,diffx)*180/M_PI+360) - ppos.orientationHorizontal;
		
		if(horizontal>=360) horizontal-=360;
		if(horizontal<0) horizontal+=360;
		if(horizontal>=360) horizontal-=360;
		if(horizontal<0) horizontal+=360;
		
		if(horizontal <= 45 || horizontal >= 360-45 )
			return 1;
		
		if(horizontal <= 90)
			right = 1;
		if(horizontal >= 360-90)
			left = 1;
	}
	if(left && right)
		return 1;

	return 0;
}

void Renderer::render(PlayerPosition pos)
{
	// Clear the screen before drawing
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And The Depth Buffer
	glLoadIdentity();							// Reset The View
	areasRendered = 0;
	
	//Mausbewegung
	glRotatef(pos.orientationVertical,0.0f,1.0f,0.0f);
	glRotatef(-pos.orientationHorizontal,0.0f,0.0f,1.0f);


	//Eigene Position
	glTranslatef(-(pos.x), -(pos.y), -(pos.z));

	glPushMatrix();
	BlockPosition areapos = pos.block().area();
	Area *area = c->map.getArea(areapos);
	areapos.z = 0;
	area = c->map.getArea(areapos);
	
	glTranslatef(area->pos.x,area->pos.y,area->pos.z);
	renderArea(area);
	glPopMatrix();

	int i=1;
	for(int r=1; r<visualRange; r+=1)
	for(int side=0; side<4; side++)
	for(int position=0; position<r*2; position+=1) {
		int x,y,z;
		
		switch(side) {
			case 0:	x = areapos.x + AREASIZE_X*(position-r);
						y = areapos.y + AREASIZE_Y*(-r);
						break;
			case 1:	x = areapos.x + AREASIZE_X*(r);
						y = areapos.y + AREASIZE_Y*(position-r);
						break;
			case 2:	x = areapos.x + AREASIZE_X*(r-position);
						y = areapos.y + AREASIZE_Y*(r); 
						break;
			case 3:	x = areapos.x + AREASIZE_X*(-r);
						y = areapos.y + AREASIZE_Y*(r-position);
						break;
		}
		z = areapos.z;
		
		if(!areaInViewport(BlockPosition::create(x,y,z), pos)) continue;
		
		glPushMatrix();
		
		Area *area = c->map.getArea(BlockPosition::create(x,y,z));

		glTranslatef(area->pos.x,area->pos.y,area->pos.z);
		renderArea(area);

		glPopMatrix();
		i++;
	}
	
	std::cout << "gerenderte areas " << i << std::endl; 
	
	/*
	for(int x=-(visualRange)+pos.x; x<(visualRange)+pos.x; x+=AREASIZE_X)
	for(int y=-(visualRange)+pos.y; y<(visualRange)+pos.y; y+=AREASIZE_Y)
	for(int z=-visualRange+pos.z; z<visualRange+pos.z; z+=AREASIZE_Z) {
		glPushMatrix();


		Area *area = c->map.getArea(BlockPosition::create(x,y,z));

		glTranslatef(area->pos.x,area->pos.y,area->pos.z);
		renderArea(area);

		glPopMatrix();
	}
*/
	
}

