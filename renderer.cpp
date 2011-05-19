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
#include <lzo/lzoconf.h>

using namespace std;
namespace fs = boost::filesystem;

#define USE_VBO

void getGlError() { 
	/*
	GLenum errCode;
	const GLubyte *errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = gluErrorString(errCode);
		fprintf (stderr, "OpenGL Error: %s\n", errString);
	}
	*/
}

Renderer::Renderer(Controller* controller)
{
	c = controller;
	areasRendered = 0;
}

Renderer::~Renderer() {
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
	angleOfVision 		= std::tan(c["angleOfVision"].as<double>()*(1/360.0*M_PI));
	

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
	if(enableFog && visualRange > 0)
		glEnable(GL_FOG);					// Enables GL_FOG



	
	unsigned char* pixels = new unsigned char[1024*1024*4];
	
	glGenTextures( 1, texture );
	for(int i=1; i<NUMBER_OF_MATERIALS; i++) {
		SDL_Surface *surface; // Gives us the information to make the texture
		
		fs::path filename = fs::path("tex") / (std::string("tex-") + boost::lexical_cast<std::string>(i) + ".jpg");
		
		if ( 	(surface = IMG_Load((dataDirectory / filename).string().c_str())) ||
				(surface = IMG_Load((workingDirectory / filename).string().c_str())) ||
				(surface = IMG_Load((localDirectory / filename).string().c_str())) ||
				(surface = IMG_Load(filename.string().c_str()))
		) {

			// Check that the image's width is a power of 2
			if ( (surface->w & (surface->w - 1)) != 0 ) {
				printf("warning: %s's width is not a power of 2\n", filename.string().c_str());
			}

			// Also check if the height is a power of 2
			if ( (surface->h & (surface->h - 1)) != 0 ) {
				printf("warning: %s's height is not a power of 2\n", filename.string().c_str());
			}

/*			// Bind the texture object
			glBindTexture( GL_TEXTURE_2D, texture[i] );

			// Set the texture's stretching properties
			if ( textureFilterMethod >= 4 ) {
				float maxAni;
				glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni );
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAni );   
				
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			} else if(textureFilterMethod >= 3){
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			}
			else if(textureFilterMethod >= 2){
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			}
			else{
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			}

			if(textureFilterMethod >= 3){
				gluBuild2DMipmaps(GL_TEXTURE_2D, 3, surface->w, surface->h, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
			} else {
				glTexImage2D(GL_TEXTURE_2D, 0, 3, surface->w, surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, surface->pixels);
			}
*/			
			unsigned char* p = (unsigned char*)surface->pixels;
			for(int x=0; x<64; x++) for(int y=0; y<64; y++) {
				
				for(int c=0; c<3; c++) 
					pixels[c + x*4 + y*4*1024 + (i%16)*64*4 + (i/16) * (1024*64*4)] = p[c + x*3 + y*3*64];
				
				pixels[3 + x*4 + y*4*1024 + (i%16)*64*4 + (i/16) * (1024*64*4)] = ((x/2)%2)&&((y/2)%2)?0:255;
				
			}
				
		}
		else {
			printf("SDL could not load %s: %s\n", filename.string().c_str(), IMG_GetError());
			SDL_Quit();
		}

		// Free the SDL_Surface only if it was successfully created
		if ( surface ) {
			SDL_FreeSurface( surface );
		}
		
	}
	// Bind the texture object
	glBindTexture( GL_TEXTURE_2D, texture[0] );

	// Set the texture's stretching properties
	if ( textureFilterMethod >= 4 /*&& glewIsSupported( "GL_EXT_texture_filter_anisotropic" )*/ ) {
		float maxAni;
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAni );   
		
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	} 
	else if(textureFilterMethod >= 3){
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	else if(textureFilterMethod >= 2){
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	else{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	if(textureFilterMethod >= 3){
		//gluBuild2DMipmaps(GL_TEXTURE_2D, 3, 1024, 1024, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, true);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
	} 
	glTexImage2D(GL_TEXTURE_2D, 0, 4, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	
	delete [] pixels;
}


void Renderer::renderArea(Area* area, bool show)
{
	
	if(area->empty) {
		area->needupdate = 0;
		area->delete_opengl();
		area->delete_collision(c->movement->dynamicsWorld);
		return;
	}
	
	bool generate_bullet = (c->movement->getPosition() - area->pos) < 50*50;
	bool delete_bullet = (c->movement->getPosition() - area->pos) > 60*60;
	
	if(area->bullet_generated && delete_bullet) {
		area->delete_collision(c->movement->dynamicsWorld);
	}
	
	if((area->needupdate || (!area->bullet_generated && generate_bullet)) && (areasRendered <= areasPerFrame)) {

		// only update, if the other blocks are loaded
		for(int d=0; d<DIRECTION_COUNT && !area->full; d++) {
			if(area->next[d]) {
				switch(area->next[d]->state) {				
					case Area::STATE_READY:			
					case Area::STATE_LOADED_BUT_NOT_FOUND:
						break;
					default:
						return;
					
				}
			} else return;
		}

		areasRendered++;
		
		area->delete_opengl();
		area->delete_collision(c->movement->dynamicsWorld);
		
		area->bullet_generated = generate_bullet;
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
						next_m = now;
					
					// Material 9 = water
					if(!next_m || (next_m == 9 && now != next_m )) {
						polygon p;
						p.pos = pos;
						p.d = (DIRECTION)dir;
						p.m = now;
						if(now == 9) {
							polys[now].push_back(p);
						} else {
							polys[0].push_back(p);
						}
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
			bool has_mesh = 0;
			
			if(generate_bullet)
				area->mesh = new btTriangleMesh();
			
			area->vbo_generated = 1;
			glGenBuffers(NUMBER_OF_MATERIALS, area->vbo);
			
			int max_counts = 0;
      
			for(int i=0; i<NUMBER_OF_MATERIALS; i++) {
				if(polys[i].empty()) continue;
				
				int size = polys[i].size()*8*POINTS_PER_POLYGON;
				
				if(max_counts < size)
					max_counts = size;
				
				area->vbo_created[i] = size;
#ifdef USE_VBO 
				glBindBuffer(GL_ARRAY_BUFFER, area->vbo[i]);
				getGlError();
#endif
				area->vbopointer[i] = new GLfloat[size];
				int vbocounter = 0;
				
				for(std::vector<polygon>::iterator it = polys[i].begin(); it != polys[i].end(); it++) {
					
					int diffx = it->pos.x-area->pos.x;
					int diffy = it->pos.y-area->pos.y;
					int diffz = it->pos.z-area->pos.z;
					for(int point=0; point < POINTS_PER_POLYGON; point++) {
						//assert(vbocounter < size);
						
						// texture
						area->vbopointer[i][vbocounter+0] = (TEXTUR_POSITION_OF_DIRECTION[it->d][point][0]+it->m%16)/16.0;
						area->vbopointer[i][vbocounter+1] = (TEXTUR_POSITION_OF_DIRECTION[it->d][point][1]+it->m/16)/16.0;
						
						// normale
						area->vbopointer[i][vbocounter+2] = NORMAL_OF_DIRECTION[it->d][0];
						area->vbopointer[i][vbocounter+3] = NORMAL_OF_DIRECTION[it->d][1];
						area->vbopointer[i][vbocounter+4] = NORMAL_OF_DIRECTION[it->d][2];
						
						// vertex
						area->vbopointer[i][vbocounter+5] = POINTS_OF_DIRECTION[it->d][point][0]+diffx;
						area->vbopointer[i][vbocounter+6] = POINTS_OF_DIRECTION[it->d][point][1]+diffy;
						area->vbopointer[i][vbocounter+7] = POINTS_OF_DIRECTION[it->d][point][2]+diffz;
						
						vbocounter+=8;
					}

					if(generate_bullet && i != 9) {
						for(int point=2; point < POINTS_PER_POLYGON; point++) {
							has_mesh = 1;
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
				}
#ifdef USE_VBO
				glBufferData(GL_ARRAY_BUFFER, size*sizeof(GLfloat), area->vbopointer[i], GL_STATIC_DRAW);
				getGlError();
				delete [] area->vbopointer[i];
				area->vbopointer[i] = 0;
#endif
			}
			/*
			ushort* index = new ushort[max_counts];
			for(int i=0; i<max_counts; i++)
				index[i] = i;
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, area->vbo[0]);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_counts*sizeof(ushort), index, GL_STATIC_DRAW);
			
			delete [] index;
			*/
			if(generate_bullet && has_mesh) {
				
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
			}
			
		} else {
			area->delete_opengl();
			area->delete_collision(c->movement->dynamicsWorld);
		}
	}
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
		 && (abs(erg.data[0][1])/(abs(erg.data[0][0])+AREASIZE_X*1.7321) < angleOfVision*(double(c->ui->screenX) / c->ui->screenY) )	// seitlich ausm sichtbereich
		 && (abs(erg.data[0][2])/(abs(erg.data[0][0])+AREASIZE_X*1.7321) < angleOfVision ) // oben/unten aus dem Bildbereich
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

	// state for rendering vbos
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.4);
	glBindTexture( GL_TEXTURE_2D, texture[0] );				
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);				
	getGlError();
	
	int i=0;
	glColor4f(1,1,1,0.5);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
//		if(a->pos != areapos) continue;
		a->show = areaInViewport(a->pos, pos);
		if(a->state == Area::STATE_READY && (a->show || areasRendered < 0)) {
			renderArea(a, 0);
			if(a->show) {
				glPushMatrix();
				glTranslatef(a->pos.x,a->pos.y,a->pos.z);			
#ifdef USE_VBO
				glBindBuffer(GL_ARRAY_BUFFER, a->vbo[0]);
				getGlError();
				
				GLfloat* startpointer = 0;				
#else
				GLfloat* startpointer = a->vbopointer[0];
#endif
				//glInterleavedArrays(GL_T2F_N3F_V3F, sizeof(GLfloat)*8, startpointer);
				glTexCoordPointer(2, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer);
				glNormalPointer(GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+2);
				glVertexPointer(3, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+5);

				getGlError();
				glDrawArrays(GL_QUADS, 0, a->vbo_created[0]/8);
				//glDrawElements(GL_QUADS, area->vbo_created[i], GL_UNSIGNED_SHORT, 0);   //The starting point of the IBO
				getGlError();
				
				glPopMatrix();
			}
		}
		i++;
	}
	
	//std::cout << "anzahl gerenderte areas: " << i << " " << (void*)a << " " << (a?a->state:-1) << std::endl;
	
	
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_ZERO, GL_ONE);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		if(a->state == Area::STATE_READY && a->show && a->vbo_created[9]) {
			glPushMatrix();
			glTranslatef(a->pos.x,a->pos.y,a->pos.z);			
#ifdef USE_VBO
			glBindBuffer(GL_ARRAY_BUFFER, a->vbo[9]);
			getGlError();
			
			GLfloat* startpointer = 0;				
#else
			GLfloat* startpointer = a->vbopointer[9];
#endif
			//glInterleavedArrays(GL_T2F_N3F_V3F, sizeof(GLfloat)*8, startpointer);
			glTexCoordPointer(2, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer);
			glNormalPointer(GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+2);
			glVertexPointer(3, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+5);

			glDrawArrays(GL_QUADS, 0, a->vbo_created[9]/8);
			glPopMatrix();
		}
	}
	glDepthFunc(GL_EQUAL);	
	glBlendFunc(GL_ONE, GL_SRC_COLOR);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		if(a->state == Area::STATE_READY && a->show && a->vbo_created[9]) {
			glPushMatrix();
			glTranslatef(a->pos.x,a->pos.y,a->pos.z);			
#ifdef USE_VBO
			glBindBuffer(GL_ARRAY_BUFFER, a->vbo[9]);
			getGlError();
			
			GLfloat* startpointer = 0;				
#else
			GLfloat* startpointer = a->vbopointer[9];
#endif
			//glInterleavedArrays(GL_T2F_N3F_V3F, sizeof(GLfloat)*8, startpointer);
			glTexCoordPointer(2, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer);
			glNormalPointer(GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+2);
			glVertexPointer(3, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+5);

			glDrawArrays(GL_QUADS, 0, a->vbo_created[9]/8);
			glPopMatrix();
		}
	}
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);	
	
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
