#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <SDL_image.h> 


#define _USE_MATH_DEFINES
#include <math.h>

#include "controller.h"
#include "map.h"

#include "matrix.h"

using namespace std;
namespace fs = boost::filesystem;

Renderer::Renderer(Controller* controller)
{
	c = controller;
	areasRendered = 0;
}

Renderer::~Renderer() {
	glDeleteLists(polygon_gllist, DIRECTION_COUNT);
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

	workingDirectory = c["workingDirectory"].as<fs::path>();
	dataDirectory = c["dataDirectory"].as<fs::path>();
	localDirectory = c["localDirectory"].as<fs::path>();

	areasPerFrame		= c["areasPerFrameRendering"].as<int>();
	highlightWholePlane	= c["highlightWholePlane"].as<bool>();
	textureFilterMethod = c["textureFilterMethod"].as<int>();
}

void Renderer::init()
{
//	itemPos.x = 0;
//	itemPos.y = 0;
//	itemPos.z = -40;
	
	// Set the OpenGL state
	glEnable(GL_TEXTURE_2D);											// Enable Texture Mapping
	glClearColor(bgColor[0],bgColor[1], bgColor[2], bgColor[3]);	// Black Background
	glClearDepth(1.0f);													// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);											// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);												// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);					// Really Nice Perspective Calculations

	glEnable(GL_CULL_FACE);
	
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
	glHint(GL_FOG_HINT, GL_FASTEST); 
	glFogf(GL_FOG_START, visualRange*fogStartFactor*AREASIZE_X);
	glFogf(GL_FOG_END, visualRange*AREASIZE_X);
	if(enableFog)
		glEnable(GL_FOG);					// Enables GL_FOG




	glGenTextures( NUMBER_OF_MATERIALS, texture );
	for(int i=1; i<NUMBER_OF_MATERIALS; i++) {
		SDL_Surface *surface; // Gives us the information to make the texture
		
		fs::path filename = fs::path("tex") / (std::string("tex-") + boost::lexical_cast<std::string>(i) + ".jpg");
		
		if ( 	(surface = IMG_Load((dataDirectory / filename).file_string().c_str())) ||
				(surface = IMG_Load((workingDirectory / filename).file_string().c_str())) ||
				(surface = IMG_Load((localDirectory / filename).file_string().c_str())) ||
				(surface = IMG_Load(filename.file_string().c_str()))
		) {

			// Check that the image's width is a power of 2
			if ( (surface->w & (surface->w - 1)) != 0 ) {
				printf("warning: %s's width is not a power of 2\n", filename.file_string().c_str());
			}

			// Also check if the height is a power of 2
			if ( (surface->h & (surface->h - 1)) != 0 ) {
				printf("warning: %s's height is not a power of 2\n", filename.file_string().c_str());
			}

			// Bind the texture object
			glBindTexture( GL_TEXTURE_2D, texture[i] );

			// Set the texture's stretching properties
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

			if(textureFilterMethod == 3){
				gluBuild2DMipmaps(GL_TEXTURE_2D, 3, surface->w, surface->h, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, 3, surface->w, surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
			}
				
		}
		else {
			printf("SDL could not load %s: %s\n", filename.file_string().c_str(), IMG_GetError());
			SDL_Quit();
		}

		// Free the SDL_Surface only if it was successfully created
		if ( surface ) {
			SDL_FreeSurface( surface );
		}
		
		// GL Lists for the polygons
		polygon_gllist = glGenLists(DIRECTION_COUNT);
		for(int i=0; i<DIRECTION_COUNT; i++) {
			glNewList(polygon_gllist + i,GL_COMPILE);
			
			glBegin( GL_QUADS );
			glNormal3f( NORMAL_OF_DIRECTION[i][0], NORMAL_OF_DIRECTION[i][1], NORMAL_OF_DIRECTION[i][2]);
			// Normal Pointing Towards Viewer
			for(int point=0; point < POINTS_PER_POLYGON; point++) {
				glTexCoord2f(
					TEXTUR_POSITION_OF_DIRECTION[i][point][0],
					TEXTUR_POSITION_OF_DIRECTION[i][point][1]
				);
				glVertex3f(
					POINTS_OF_DIRECTION[i][point][0],
					POINTS_OF_DIRECTION[i][point][1],
					POINTS_OF_DIRECTION[i][point][2]
				);
			}
			glEnd();
			glEndList();
		}
		
	}
	/*
	 
	std::string cfile;
	int k = 1; //rand()%86;
	std::ifstream f3;
	f3.open((fs::path("structs") / "allfiles").file_string().c_str());
	assert(f3.is_open());
	
	for(int i=0; i<=k; i++)
		f3 >> cfile;
	
	glGenTextures( 1, &texture_item );
	
	gllist_item = glGenLists(1);
	glNewList(gllist_item,GL_COMPILE);
	glBindTexture( GL_TEXTURE_2D, texture_item );
	
	std::ifstream f((fs::path("structs") / cfile).file_string().c_str());
	assert(f.is_open());
	
	std::string textur;
	
	for(int i=0; i<7; i++) {
		f >> textur;
	}
	
	std::ifstream f2;
	f2.open((fs::path("texpages") / textur).file_string().c_str());
	
	assert(f2.is_open());
	
	SDL_Surface *surface2 = IMG_Load((fs::path("texpages") / textur).file_string().c_str());
	if(!surface2) printf("SDL could not load %s: %s\n", (fs::path("texpages") / textur).file_string().c_str(), IMG_GetError());
	
	SDL_Surface *surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
															  surface2->w, surface2->h, 
															32, 0x000000ff, 0x0000ff00,0x00ff0000 , 0xff000000);
	if(!surface) printf("SDL could not load %s: %s\n", (fs::path("texpages") / textur).file_string().c_str(), SDL_GetError());
	SDL_BlitSurface(surface2, 0, surface, 0);
	
	SDL_FreeSurface(surface2);
	
	glBindTexture( GL_TEXTURE_2D, texture_item );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, surface->w, surface->h, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
	if ( surface ) SDL_FreeSurface( surface );
	

	for(int i=0; i<7; i++) {
		f >> textur;
	}
	
	int points_count;
	f >> points_count;
	float *points;
	points = new float[points_count*3];
	for(int i=0; i<points_count; i++) {
		f >> points[i*3+0];
		f >> points[i*3+1];
		f >> points[i*3+2];
		points[i*3+1]-=15;
		points[i*3+0]/=3;
		points[i*3+1]/=3;
		points[i*3+2]/=3;
	}
	
	f >> textur; 
	
	int polygons;
	f >> polygons;
	for(int i=0; i<polygons; i++) {
		double farbe[4];
		int flags;
		int dots_count;
		f >> flags;
		f >> dots_count;
		
		//assert(flags == 200 || flags == 4200);
		
		
		int   *dots = new   int[dots_count];
		double *tex  = new double[dots_count*2];
		
		for(int k=0; k<dots_count; k++) {
			f >> dots[k];
			assert(dots[k] < points_count);
		}
		
		if((flags/1000) & 4) {
			f >> farbe[0];
			f >> farbe[1];
			f >> farbe[2];
			f >> farbe[3];
			
			farbe[0] /= 256;
			farbe[1] /= 256;
			farbe[2] /= 256;
			farbe[3] /= 256;
			
			glColor4f(farbe[0],farbe[1],farbe[2],farbe[3]);
		} else {
			glColor4f(1,1,1,1);
			
		}
		
		for(int k=0; k<dots_count; k++) {
			f >> tex[k*2]; f >> tex[k*2+1]; 
		}
		
		
		if(dots_count == 3) 	glBegin( GL_TRIANGLES );
		if(dots_count == 4)	glBegin( GL_QUADS );
		else						glBegin( GL_POLYGON );
		for(int k=0; k<dots_count; k++) {
			glTexCoord2f(tex[k*2+0]/256,tex[k*2+1]/256);
			glVertex3f(points[dots[k]*3+0],points[dots[k]*3+1],points[dots[k]*3+2]);
		}
		glEnd();
		
		for(int k=2; k<dots_count; k++)
			triangles_item.addTriangle (
				btVector3(points[dots[0]*3+0],points[dots[0]*3+1],points[dots[0]*3+2]), 
				btVector3(points[dots[k-1]*3+0],points[dots[k-1]*3+1],points[dots[k-1]*3+2]), 
				btVector3(points[dots[k]*3+0],points[dots[k]*3+1],points[dots[k]*3+2]));
	
		delete [] dots;
		delete [] tex;
	}
	
	delete [] points;

	f.close();
	glEndList();
	*/
}




void Renderer::renderArea(Area* area, bool show)
{
	if(area->empty) {
		area->needupdate = 0;
		if(area->gllist_generated)
			glDeleteLists(area->gllist,1);
		area->delete_collision(c->movement->dynamicsWorld);
		area->gllist = 0;
		area->gllist_generated = 0;
		return;
	}
	if(show) {
		glPushMatrix();
		glTranslatef(area->pos.x,area->pos.y,area->pos.z);
	}
	
	if(area->needupdate && (areasRendered <= areasPerFrame)) {
		areasRendered++;
		
		
		area->delete_collision(c->movement->dynamicsWorld);
		
		area->needupdate = 0;
		
		std::vector<polygon> polys[NUMBER_OF_MATERIALS];

		bool empty = 1;
		
		int polys_count = 0;
		
		for(int x=area->pos.x; x<AREASIZE_X+area->pos.x; x++)
		for(int y=area->pos.y; y<AREASIZE_Y+area->pos.y; y++)
		for(int z=area->pos.z; z<AREASIZE_Z+area->pos.z; z++)  {

			BlockPosition pos = BlockPosition::create(x,y,z);

			Material now = area->get(pos);
			if(now) {
				empty = 0;
				for(int dir=0; dir < DIRECTION_COUNT; dir++) {
					BlockPosition next = pos+(DIRECTION)dir;

					Material next_m;
					if((*area) << next)
						next_m = area->get(next);
					else if(area->next[dir] && area->next[dir]->state == Area::STATE_READY)
						next_m = area->next[dir]->get(next);
					else 
						next_m = 1;
					

					if(!next_m) {
						polygon p;
						p.pos = pos;
						p.d = (DIRECTION)dir;
						polys[now].push_back(p);
						polys_count++;
					}
				}
			}
		}
		if(empty) {
			area->needstore = 1;
			area->empty = 1;
		} 
		if(polys_count) {	
			
			area->mesh = new btTriangleMesh();
	//		mesh.preallocateIndices(polys_count*(POINTS_PER_POLYGON-2));
			
			if(!area->gllist_generated) {
				area->gllist_generated = 1;
				area->gllist = glGenLists(1);
			}
			glNewList(area->gllist,GL_COMPILE);
			
			int diff_oldx = 0;
			int diff_oldy = 0;
			int diff_oldz = 0;
      
      
			for(int i=0; i<NUMBER_OF_MATERIALS; i++) {
				bool texture_used = 0;
				for(std::vector<polygon>::iterator it = polys[i].begin(); it != polys[i].end(); it++) {
					
					// switch texture
					if(!texture_used) {
						texture_used = 1;
						glBindTexture( GL_TEXTURE_2D, texture[i] );
						glBegin( GL_QUADS );
					}
					
					int diffx = it->pos.x-area->pos.x;
					int diffy = it->pos.y-area->pos.y;
					int diffz = it->pos.z-area->pos.z;
          
          
					//glTranslatef(diffx-diff_oldx, diffy-diff_oldy, diffz-diff_oldz);
					//glCallList(polygon_gllist + it->d);
          
					diff_oldx = diffx;
					diff_oldy = diffy;
					diff_oldz = diffz;
          
					
					glNormal3f( NORMAL_OF_DIRECTION[it->d][0], NORMAL_OF_DIRECTION[it->d][1], NORMAL_OF_DIRECTION[it->d][2]);                                     // Normal Pointing Towards Viewer
					for(int point=0; point < POINTS_PER_POLYGON; point++) {
						glTexCoord2f(
							TEXTUR_POSITION_OF_DIRECTION[it->d][point][0],
							TEXTUR_POSITION_OF_DIRECTION[it->d][point][1]
						);
						glVertex3f(
							POINTS_OF_DIRECTION[it->d][point][0]+diffx,
							POINTS_OF_DIRECTION[it->d][point][1]+diffy,
							POINTS_OF_DIRECTION[it->d][point][2]+diffz
						);
					}
					for(int point=2; point < POINTS_PER_POLYGON; point++) {
						area->mesh->addTriangle(
							btVector3(
								POINTS_OF_DIRECTION[it->d][0][0]+diffx,
								POINTS_OF_DIRECTION[it->d][0][1]+diffy,
								POINTS_OF_DIRECTION[it->d][0][2]+diffz
							),
							btVector3(
								POINTS_OF_DIRECTION[it->d][point][0]+diffx,
								POINTS_OF_DIRECTION[it->d][point][1]+diffy,
								POINTS_OF_DIRECTION[it->d][point][2]+diffz
							),
							btVector3(
								POINTS_OF_DIRECTION[it->d][point-1][0]+diffx,
								POINTS_OF_DIRECTION[it->d][point-1][1]+diffy,
								POINTS_OF_DIRECTION[it->d][point-1][2]+diffz
							)							
						);
					}
				}
				if(texture_used)
					glEnd();
			}
//			glTranslatef(-diff_oldx, -diff_oldy, -diff_oldz);
			glEndList();
			
			area->shape = new btBvhTriangleMeshShape(area->mesh,1);
			//area->shape = new btConvexTriangleMeshShape(area->mesh);
			
			///create a few basic rigid bodies			
			btTransform groundTransform;
			groundTransform.setIdentity();
			groundTransform.setOrigin(btVector3(area->pos.x,area->pos.y,area->pos.z));
		
			area->motion = new btDefaultMotionState(groundTransform);
			area->rigid = new btRigidBody(0,area->motion,area->shape,btVector3(0,0,0));
			
			//add the body to the dynamics world
			c->movement->dynamicsWorld->addRigidBody(area->rigid);
			
		} else {
			if(area->gllist_generated)
				glDeleteLists(area->gllist,1);
			area->gllist_generated = 0;
			area->gllist = 0;
			area->delete_collision(c->movement->dynamicsWorld);
		}
	}
	if(area->gllist_generated) {
		if(show) {
			glCallList(area->gllist);
		}
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

//	erg.to_str();
//	std::cout << std::endl;
	
	return (erg.data[0][0] > - AREASIZE_X/2*1.7321) 				// nicht hinter einem
	    && (erg.data[0][0] < AREASIZE_X*(visualRange+0.866))	// sichtweite
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
	
	if(areasRendered<0) areasRendered = 0;
	areasRendered -= areasPerFrame;

	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
//		if(a->pos != areapos) continue;
		bool inview = areaInViewport(a->pos, pos);
		if(a->state == Area::STATE_READY && (inview || areasRendered < 0)) {
			renderArea(a, inview);
		}
	}
	
	renderObjects();
	
/*	glPushMatrix();
	glTranslatef(itemPos.x,itemPos.y,itemPos.z);
	glRotatef(itemPos.rotate.getAngle()*180/M_PI,itemPos.rotate.getAxis().getX(),itemPos.rotate.getAxis().getY(),itemPos.rotate.getAxis().getZ());
	glCallList(gllist_item);
	
	glPopMatrix();
	*/
/*	int anzahl_mit_data=0;
	int anzahl_ohne_data=0;
	
	for(Map::iterator it = c->map->areas.begin(); it != c->map->areas.end(); it++)
		if(it->second->m)
			anzahl_mit_data++;
		else
			anzahl_ohne_data++;
	std::cout << "anzahl areas: " << anzahl_mit_data << " " << anzahl_ohne_data <<std::endl;
	*/
	
}

void Renderer::highlightBlockDirection(BlockPosition block, DIRECTION direct){
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHTING);
	glBindTexture( GL_TEXTURE_2D, texture[1] );
	glColor4f(0.5f, 0.5f, 0.5f, 0.5f);

	if(highlightWholePlane)
		glDisable(GL_DEPTH_TEST);
	else
		glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);
	glEnable(GL_BLEND);
	glBegin(GL_QUADS);		

	for(int i = POINTS_PER_POLYGON-1; i >=0; i--){
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

void Renderer::renderObjects() {
	std::list<MovingObject*>::iterator it;
	for(it = c->map->objects.begin(); it != c->map->objects.end(); it++) {
		glPushMatrix();
		
		btTransform trans;
		(*it)->m->getWorldTransform(trans);		
		glTranslatef(trans.getOrigin().getX(),trans.getOrigin().getY(),trans.getOrigin().getZ());
		glRotatef( trans.getRotation().getAngle()*180/M_PI, trans.getRotation().getAxis().getX(), trans.getRotation().getAxis().getY(), trans.getRotation().getAxis().getZ());
		
		glBindTexture( GL_TEXTURE_2D, texture[(*it)->tex] );
		glBegin( GL_QUADS );

		for(int i=0; i<DIRECTION_COUNT; i++) {
			glNormal3f( NORMAL_OF_DIRECTION[i][0], NORMAL_OF_DIRECTION[i][1], NORMAL_OF_DIRECTION[i][2]);                                     // Normal Pointing Towards Viewer
					
			for(int point=0; point < POINTS_PER_POLYGON; point++) {
				glTexCoord2f(
					TEXTUR_POSITION_OF_DIRECTION[i][point][0],
					TEXTUR_POSITION_OF_DIRECTION[i][point][1]
				);
				glVertex3f(
					(POINTS_OF_DIRECTION[i][point][0]*2-1)*0.1,
					(POINTS_OF_DIRECTION[i][point][1]*2-1)*0.1,
					(POINTS_OF_DIRECTION[i][point][2]*2-1)*0.1
				);
			}
		}
		glEnd();

		glPopMatrix();
	}
}