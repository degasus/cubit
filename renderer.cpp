#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <vector>

#include "controller.h"
#include "map.h"

#include "matrix.h"

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
	visualRange			= c["visualRange"].as<int>();
	maxareas			= c["visualRange"].as<int>()*c["visualRange"].as<int>();
	enableFog			= c["enableFog"].as<bool>();

	string textureDirectory = c["workingDirectory"].as<string>() + "/tex";
	Texture_Files[1]	= textureDirectory + "/" + c["texture01"].as<string>();
	Texture_Files[2]	= textureDirectory + "/" + c["texture02"].as<string>();
	Texture_Files[3]	= textureDirectory + "/" + c["texture03"].as<string>();
	Texture_Files[4]	= textureDirectory + "/" + c["texture04"].as<string>();
	Texture_Files[5]	= textureDirectory + "/" + c["texture05"].as<string>();
	Texture_Files[6]	= textureDirectory + "/" + c["texture06"].as<string>();
	Texture_Files[7]	= textureDirectory + "/" + c["texture07"].as<string>();
	for(int i = 1; i <= 73; i++)
		Texture_Files[i+7]	= textureDirectory + "/tex-" + boost::lexical_cast<std::string>(i) + ".bmp";

	areasPerFrame		= c["areasPerFrameRendering"].as<int>();
	highlightWholePlane	= c["highlightWholePlane"].as<bool>();
	textureFilterMethod = c["textureFilterMethod"].as<int>();
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

	
	//LIGHT

	glEnable(GL_LIGHTING);
	
	/*GLfloat LightAmbient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat LightPosition[] = { 0.5f, 1.0f, 5.0f, 1.0f };

	//GLfloat LightPosition[] = { 0.5f, 0.1f, 1.0f, 1.0f };
	
	glLightfv(GL_LIGHT1, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);*/

	GLfloat LightAmbient[]  = { 0.8f, 0.8f, 0.8f, 1.0f };
	GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };	
	
	glLightfv(GL_LIGHT1, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  LightDiffuse);
	glEnable(GL_LIGHT1);

	glLightfv(GL_LIGHT2, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT2, GL_DIFFUSE,  LightDiffuse);
	glEnable(GL_LIGHT2);

	//Global Ambient
	/*GLfloat global_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, global_ambient);*/
	

	
	//FOG
	
	glFogi(GL_FOG_MODE, GL_LINEAR);		// Fog Mode
	glFogfv(GL_FOG_COLOR, bgColor);	// Set Fog Color
	glFogf(GL_FOG_DENSITY, fogDense);	// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_DONT_CARE);	// Fog Hint Value
	glFogf(GL_FOG_START, (visualRange-1)*fogStartFactor*AREASIZE_X);
	glFogf(GL_FOG_END, (visualRange-1)*AREASIZE_X);
	if(enableFog)
		glEnable(GL_FOG);					// Enables GL_FOG




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
			//glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			if(textureFilterMethod == 3){
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			}
			else if(textureFilterMethod == 2){
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			}
			else{
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			}


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


void Renderer::renderArea(Area* area, bool show)
{
	if(area->empty) {
		area->needupdate = 0;
		if(area->gllist_generated)
			glDeleteLists(area->gllist,1);
		area->gllist_generated = 0;
		return;
	}
	if(show) {
		glPushMatrix();
		glTranslatef(area->pos.x,area->pos.y,area->pos.z);
	}

	if(area->needupdate && (areasRendered <= areasPerFrame)) {
		areasRendered++;
		
		area->needupdate = 0;
		
		std::vector<polygon> polys[NUMBER_OF_MATERIALS];

		Area* areas[DIRECTION_COUNT];
		for(int i=0; i<DIRECTION_COUNT; i++)
			areas[i] = 0;

		bool empty = 1;
		
		for(int x=area->pos.x; x<AREASIZE_X+area->pos.x; x++)
		for(int y=area->pos.y; y<AREASIZE_Y+area->pos.y; y++)
		for(int z=area->pos.z; z<AREASIZE_Z+area->pos.z; z++)  {

			BlockPosition pos = BlockPosition::create(x,y,z);

			Material now = area->get(pos);;
			if(now) {
				empty = 0;
				for(int dir=0; dir < DIRECTION_COUNT; dir++) {
					BlockPosition next = pos+(DIRECTION)dir;

					Material next_m;
					if((*area) << next)
						next_m = area->get(next);
					else if(areas[dir] && (*areas[dir]) << next)
						next_m = areas[dir]->get(next);
					else {
						try {
							areas[dir] = c->map->getArea(next);
							next_m = areas[dir]->get(next);
						} catch(NotLoadedException e) {
							next_m = 1;
						} catch(AreaEmptyException e) {
							next_m = 0;
						}
					}

					if(!next_m) {
						polygon p;
						p.pos = pos;
						p.d = (DIRECTION)dir;
						polys[now].push_back(p);
					}
				}
			}
		}
		if(empty) {
			area->needstore = 1;
			area->empty = 1;
		}
		
		bool found_polygon = 0;
		for(int i=0; i<NUMBER_OF_MATERIALS; i++) {
			bool texture_used = 0;
			for(std::vector<polygon>::iterator it = polys[i].begin(); it != polys[i].end(); it++) {
				
				// create new list
				if(!found_polygon) {
					found_polygon = 1;
					if(!area->gllist_generated) {
						area->gllist_generated = 1;
						area->gllist = glGenLists(1);
					}
					glNewList(area->gllist,GL_COMPILE);
				}
				
				// switch texture
				if(!texture_used) {
					texture_used = 1;
					glBindTexture( GL_TEXTURE_2D, texture[i] );
					glBegin( GL_QUADS );
				}
				
				
				glNormal3f( NORMAL_OF_DIRECTION[it->d][0], NORMAL_OF_DIRECTION[it->d][1], NORMAL_OF_DIRECTION[it->d][2]);                                     // Normal Pointing Towards Viewer
				for(int point=0; point < POINTS_PER_POLYGON; point++) {
					glTexCoord2f(
						TEXTUR_POSITION_OF_DIRECTION[it->d][point][0],
						TEXTUR_POSITION_OF_DIRECTION[it->d][point][1]
					);
					glVertex3f(
						POINTS_OF_DIRECTION[it->d][point][0]+(it->pos.x-area->pos.x),
						POINTS_OF_DIRECTION[it->d][point][1]+(it->pos.y-area->pos.y),
						POINTS_OF_DIRECTION[it->d][point][2]+(it->pos.z-area->pos.z)
					);
				}
			}
			if(texture_used)
				glEnd();
		}
		if(found_polygon)
			glEndList();
		else {
			if(area->gllist_generated)
				glDeleteLists(area->gllist,1);
			area->gllist_generated = 0;	
		}
		
	}
	if(area->gllist_generated) {
		if(show)
			glCallList(area->gllist);
	} else {
		// do nothing
	}
	if(show)
		glPopMatrix();
}

void Renderer::generateViewPort(PlayerPosition pos) {
	double a = pos.orientationHorizontal/180*M_PI;
	
	Matrix<double,3,3> drehz;
	drehz.data[0][0] = cos(a);
	drehz.data[0][1] = -sin(a);
	drehz.data[0][2] = 0;
	drehz.data[1][0] = sin(a);
	drehz.data[1][1] = cos(a);
	drehz.data[1][2] = 0;
	drehz.data[2][0] = 0;
	drehz.data[2][1] = 0;
	drehz.data[2][2] = 1;
	
	//drehz.to_str();
	
	
	a = (pos.orientationVertical)/180*M_PI;
	
	Matrix<double,3,3> drehy;
	drehy.data[0][0] = cos(a);
	drehy.data[0][1] = 0;
	drehy.data[0][2] = -sin(a);
	drehy.data[1][0] = 0;
	drehy.data[1][1] = 1;
	drehy.data[1][2] = 0;
	drehy.data[2][0] = sin(a);
	drehy.data[2][1] = 0;
	drehy.data[2][2] = cos(a);
	
	viewPort = drehy*drehz;
}

bool Renderer::areaInViewport(BlockPosition apos, PlayerPosition ppos) {
		
	Matrix<double,1,3> pos;
	pos.data[0][0] = apos.x - ppos.x + AREASIZE_X/2;
	pos.data[0][1] = apos.y - ppos.y + AREASIZE_Y/2;
	pos.data[0][2] = apos.z - ppos.z + AREASIZE_Z/2;
	
	//pos.to_str();
	
	//drehy.to_str();
	
	Matrix<double,1,3> erg = viewPort*pos;
/*	erg.to_str();
	
	std::cout << apos.z << " " << ppos.z << std::endl;
	std::cout << std::endl;
	*/
	return (erg.data[0][0] > - AREASIZE_X/1.7321) 				// nicht hinter einem
	    && (erg.data[0][0] < AREASIZE_X*visualRange)	// sichtweite
		 && (abs(erg.data[0][1])/(abs(erg.data[0][0])+AREASIZE_X*1.7321) < (double(c->ui->screenX) / c->ui->screenY)/2 )	// seitlich ausm sichtbereich
		 && (abs(erg.data[0][2])/(abs(erg.data[0][0])+AREASIZE_X*1.7321) < 0.5 )	// seitlich ausm sichtbereich
		 ;
}
void Renderer::render(PlayerPosition pos)
{
	// Clear the screen before drawing
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And The Depth Buffer
	glLoadIdentity();							// Reset The View
	if(enableFog)
		glEnable(GL_FOG);
	else
		glDisable(GL_FOG);
	glScalef(-1,1,1);
	glRotatef(90.0,0.0f,0.0f,1.0f);
	glRotatef(90.0,0.0f,1.0f,0.0f);
	
	if(areasRendered<0) areasRendered = 0;
	areasRendered -= areasPerFrame;
	
	//Mausbewegung
	glRotatef(pos.orientationVertical,0.0f,1.0f,0.0f);
	glRotatef(-pos.orientationHorizontal,0.0f,0.0f,1.0f);


	//Eigene Position
	glTranslatef(-(pos.x), -(pos.y), -(pos.z));
	
	GLfloat LightPosition[] = { 10000000.0f, 6600000.0f, 25000000.0f, 1.0f };
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);

	GLfloat LightPosition2[] = { -10000000.0f, -2000000.0f, 25000000.0f, 1.0f };
	glLightfv(GL_LIGHT2, GL_POSITION, LightPosition2);

	// eigenes gebiet
	BlockPosition areapos = pos.block().area();
	generateViewPort(pos);
	

	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		bool inview = areaInViewport(a->pos, pos);
		if(a->state == Area::STATE_READY && (inview || areasRendered < 0)) {
			renderArea(a, inview);
			if(!a->needupdate && !a->gllist_generated) {
				c->map->areas_with_gllist.erase(it);
			} 
		}
	}
	//std::cout << "anzahl areas: " << c->map->areas_with_gllist.size() << std::endl;
}

void Renderer::highlightBlockDirection(BlockPosition block, DIRECTION direct){
		glDisable(GL_LIGHT1);
		glDisable(GL_LIGHTING);

		if(highlightWholePlane)
			glDisable(GL_DEPTH_TEST);
		else
			glEnable(GL_DEPTH_TEST);
		glColor4f(0.0f, 1.0f, 1.0f, 0.5f);
		glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);
		glEnable(GL_BLEND);
		glBegin(GL_QUADS);		

		for(int i = 0; i < POINTS_PER_POLYGON; i++){
			glVertex3f(block.x + POINTS_OF_DIRECTION[direct][i][0],
					   block.y + POINTS_OF_DIRECTION[direct][i][1],
					   block.z + POINTS_OF_DIRECTION[direct][i][2]
  					);
		}
		
		glEnd();
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		
		glColor4f(1.0f, 1.0f, 1.0f, 0.0f);

		glEnable(GL_LIGHT1);
		glEnable(GL_LIGHTING);
}

