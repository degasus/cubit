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
			if ( textureFilterMethod >= 4 /*&& glewIsSupported( "GL_EXT_texture_filter_anisotropic" )*/ ) {
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
	
	bool generate_bullet = (c->movement->getPosition() - area->pos) < 50*50;
	bool delete_bullet = (c->movement->getPosition() - area->pos) > 60*60;
	
	if(area->bullet_generated && delete_bullet)
		area->delete_collision(c->movement->dynamicsWorld);
	
	if((area->needupdate || (!area->bullet_generated && generate_bullet)) && (areasRendered <= areasPerFrame)) {
		areasRendered++;
		
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
						next_m = 1;
					
					// MAterial 99 = water
					if(!next_m || (next_m == 99 && now != next_m )) {
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
			
			if(generate_bullet)
				area->mesh = new btTriangleMesh();
			
			if(!area->gllist_generated) {
				area->gllist_generated = 1;
				area->gllist = glGenLists(1);
			}
			glNewList(area->gllist,GL_COMPILE);
			
			int diff_oldx = 0;
			int diff_oldy = 0;
			int diff_oldz = 0;
      
      
			for(int i=0; i<NUMBER_OF_MATERIALS; i++) {
				if(i==99) continue;
				bool texture_used = 0;
				for(std::vector<polygon>::iterator it = polys[i].begin(); it != polys[i].end(); it++) {
					
					// switch texture
					if(!texture_used) {
						texture_used = 1;
						glBindTexture( GL_TEXTURE_2D, texture[i] );
						if(i == 99){
							//glDisable(GL_CULL_FACE);
							glEnable(GL_BLEND);
							//glDisable(GL_LIGHTING);
							glBlendFunc(GL_ZERO, GL_ONE);
							//glColor4f(0.5f, 0.5f, 0.5f, 0.1f);
						}
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
					if(generate_bullet && i != 99) {
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
				}
				if(texture_used){
					glEnd();
					if(i == 99){
						//glEnable(GL_CULL_FACE);
						//glEnable(GL_LIGHTING);
						glDisable(GL_BLEND);
					} 
				}
			}
//			glTranslatef(-diff_oldx, -diff_oldy, -diff_oldz);
			glEndList();
			
			if(generate_bullet) {
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
			
			int i = 99;
				
			if( polys[i].begin() != polys[i].end()) {
				
				if(!area->gllist_has_blend) {
					area->gllist_has_blend = 1;
					area->gllist_blend = glGenLists(1);
				}
				glNewList(area->gllist_blend,GL_COMPILE);      
		
				
				glBindTexture( GL_TEXTURE_2D, texture[i] );

				glBegin( GL_QUADS );
						
				for(std::vector<polygon>::iterator it = polys[i].begin(); it != polys[i].end(); it++) {
					
					int diffx = it->pos.x-area->pos.x;
					int diffy = it->pos.y-area->pos.y;
					int diffz = it->pos.z-area->pos.z;
						
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
				}
				glEnd();
	//			glTranslatef(-diff_oldx, -diff_oldy, -diff_oldz);
				glEndList();
				
			}
		} else {
			if(area->gllist_generated) {
				glDeleteLists(area->gllist,1);
				if(area->gllist_has_blend)
					glDeleteLists(area->gllist_blend,1);
			}
			area->gllist_generated = 0;
			area->gllist_has_blend = 0;
			area->gllist = 0;
			area->gllist_blend = 0;
			area->delete_collision(c->movement->dynamicsWorld);
		}
	}
	if(area->gllist_generated) {
		if(show) {
			glCallList(area->gllist);
			//if(area->gllist_has_blend)
			//	glCallList(area->gllist_blend);
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

	int i=0;
	glColor4f(1,1,1,0.5);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
//		if(a->pos != areapos) continue;
		bool inview = areaInViewport(a->pos, pos);
		if(a->state == Area::STATE_READY && (inview || areasRendered < 0)) {
			renderArea(a, inview);
		}
		i++;
	}
	//std::cout << "anzahl geränderte areas: " << i << std::endl;

	
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_ZERO, GL_ONE);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		bool inview = areaInViewport(a->pos, pos);
		if(a->state == Area::STATE_READY && inview && a->gllist_has_blend) {
			glPushMatrix();
			glTranslatef(a->pos.x,a->pos.y,a->pos.z);
			glCallList(a->gllist_blend);
			glPopMatrix();
		}
	}
	glDepthFunc(GL_EQUAL);	
	glBlendFunc(GL_ONE, GL_SRC_COLOR);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		bool inview = areaInViewport(a->pos, pos);
		if(a->state == Area::STATE_READY && inview && a->gllist_has_blend) {
			glPushMatrix();
			glTranslatef(a->pos.x,a->pos.y,a->pos.z);
			glCallList(a->gllist_blend);
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