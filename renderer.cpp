#include <iostream>
#include <fstream>
#include <vector>
#include <stack>
#include <SDL_image.h> 
#include <SDL_thread.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "config.h"
#include "controller.h"
#include "renderer.h"
#include "map.h"

#include "matrix.h"


using namespace std;
namespace fs = boost::filesystem;


void getGlError(std::string meldung = "") { 
	
	GLenum errCode;
	const GLubyte *errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = gluErrorString(errCode);
		fprintf (stderr, "%s OpenGL Error: %s\n",meldung.c_str(), errString);
	}
	
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
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);					// Really Nice Perspective Calculations
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);					// Really Nice Perspective Calculations
	glShadeModel(GL_FLAT);
	glDisable(GL_DITHER);
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
	
	glGenTextures( 2, texture );
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

			unsigned char* p = (unsigned char*)surface->pixels;
			for(int x=0; x<64; x++) for(int y=0; y<64; y++) {
				
				for(int c=0; c<3; c++) 
					pixels[c + x*4 + y*4*1024 + (i%16)*64*4 + (i/16) * (1024*64*4)] = p[c + x*3 + y*3*64];
				
				pixels[3 + x*4 + y*4*1024 + (i%16)*64*4 + (i/16) * (1024*64*4)] = ((x/4)%3)&&((y/4)%3)?0:255;
				
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
	//glBindTexture(GL_TEXTURE_2D_ARRAY,texture[1]);
	//glTexImage3D(GL_TEXTURE_2D_ARRAY,0,GL_RGBA, 64, 64, NUMBER_OF_MATERIALS, 0, GL_RGBA, GL_FLOAT, pixels);
	
	// Bind the texture object
	glBindTexture( GL_TEXTURE_2D, texture[0] );
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// Set the texture's stretching properties
	if ( textureFilterMethod >= 4 && glewIsSupported( "GL_EXT_texture_filter_anisotropic" ) ) {
		float maxAni;
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAni );  
	} 
	if(textureFilterMethod >= 3 && glewIsSupported( "glGenerateMipmap" ) ){
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );		
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, true);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
	}
	else if(textureFilterMethod >= 2){
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	else{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}

	glTexImage2D(GL_TEXTURE_2D, 0, 4, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	
	delete [] pixels;
	/*
	std::cout << "Shader: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	
	shader_po = glCreateProgramObjectARB();
	getGlError("glCreateProgramObjectARB");
	
	shader_vs = glCreateShaderObjectARB(GL_VERTEX_SHADER);
	getGlError("glCreateShaderObjectARB vs");
	
	shader_fs = glCreateShaderObjectARB(GL_FRAGMENT_SHADER);
	getGlError("glCreateShaderObjectARB fs");
	
	std::string shader = "";
	const char *shader_src = shader.c_str();
	int shader_src_length = shader.length();
	glShaderSourceARB(shader_vs, 1, &shader_src, &shader_src_length);
	getGlError("glShaderSourceARB");
	
	glCompileShaderARB(shader_vs);
	getGlError("glCompileShaderARB vs");
	
	
	glCompileShaderARB(shader_fs);
	getGlError("glCompileShaderARB fs");
	*/
}


void Renderer::generateArea(Area* area) {
	
	if(area->empty) {
		area->needupdate_gl = 0;
		area->delete_opengl();
		area->delete_collision(c->movement->dynamicsWorld);
		return;
	}
	
	bool generate_bullet = (c->movement->getPosition() - area->pos) < 50*50;
	bool delete_bullet = (c->movement->getPosition() - area->pos) > 60*60;
	
	if(area->bullet_generated && delete_bullet) {
		area->delete_collision(c->movement->dynamicsWorld);
	}
	
	if((area->needupdate_poly || area->needupdate_gl || (!area->bullet_generated && generate_bullet)) && (areasRendered <= areasPerFrame)) {
		
		areasRendered++;
		
		area->delete_opengl();
		area->delete_collision(c->movement->dynamicsWorld);
		
		area->bullet_generated = generate_bullet;
		
		if(area->needupdate_poly)
			area->recalc_polys();
		
		area->needupdate_gl = 0;
		
		if(area->polys_list) {	
			bool has_mesh = 0;
			
			if(generate_bullet)
				area->mesh = new btTriangleMesh();
			
#ifdef USE_VBO 
			area->vbo_generated = 1;
			glGenBuffers(NUMBER_OF_LISTS, area->vbo);
#endif
			
#ifdef USE_GLLIST
			area->gllist_generated = 1;
			area->gllist = glGenLists(NUMBER_OF_LISTS);
#endif
			
			int max_counts = 0;
      
			for(int i=0; i<NUMBER_OF_LISTS; i++) {
				if(!area->polys_list_size[i]) continue;
				
				int points[] = {0,1,2,0,2,3};
				
				int size = area->polys_list_size[i]*8*sizeof(points)/sizeof(int);
				
				if(max_counts < size)
					max_counts = size;
				
				area->vbo_length[i] = size;
				area->vbopointer[i] = new GLfloat[size];
				int vbocounter = 0;
				
				for(int k=area->polys_list_start[i]; k<area->polys_list_size[i]+area->polys_list_start[i]; k++) {
					polygon it = area->polys_list[k];
					
					int diffx = it.posx;
					int diffy = it.posy;
					int diffz = it.posz;
					
					//std::cout << k << " " << diffx << " " << diffy << " " << diffz << std::endl;
					
					for(int j=0; j < sizeof(points)/sizeof(int); j++) {
						int point = points[j];
						//assert(vbocounter < size);
						
						// texture
						area->vbopointer[i][vbocounter+0] = (TEXTUR_POSITION_OF_DIRECTION[it.dir][point][0]+it.m%16)/16.0;
						area->vbopointer[i][vbocounter+1] = (TEXTUR_POSITION_OF_DIRECTION[it.dir][point][1]+it.m/16)/16.0;
						
						// normale
						area->vbopointer[i][vbocounter+2] = NORMAL_OF_DIRECTION[it.dir][0];
						area->vbopointer[i][vbocounter+3] = NORMAL_OF_DIRECTION[it.dir][1];
						area->vbopointer[i][vbocounter+4] = NORMAL_OF_DIRECTION[it.dir][2];
						
						// vertex
						area->vbopointer[i][vbocounter+5] = it.sizex * POINTS_OF_DIRECTION[it.dir][point][0]+diffx;
						area->vbopointer[i][vbocounter+6] = it.sizey * POINTS_OF_DIRECTION[it.dir][point][1]+diffy;
						area->vbopointer[i][vbocounter+7] = it.sizez * POINTS_OF_DIRECTION[it.dir][point][2]+diffz;
						
						vbocounter+=8;
					}

					if(generate_bullet && i != 6) {
						for(int point=2; point < POINTS_PER_POLYGON; point++) {
							has_mesh = 1;
							area->mesh->addTriangle(
								btVector3(
									it.sizex * POINTS_OF_DIRECTION[it.dir][0][0]+diffx,
									it.sizey * POINTS_OF_DIRECTION[it.dir][0][1]+diffy,
									it.sizez * POINTS_OF_DIRECTION[it.dir][0][2]+diffz
								),
								btVector3(
									it.sizex * POINTS_OF_DIRECTION[it.dir][point-1][0]+diffx,
									it.sizey * POINTS_OF_DIRECTION[it.dir][point-1][1]+diffy,
									it.sizez * POINTS_OF_DIRECTION[it.dir][point-1][2]+diffz
								),
								btVector3(
									it.sizex * POINTS_OF_DIRECTION[it.dir][point][0]+diffx,
									it.sizey * POINTS_OF_DIRECTION[it.dir][point][1]+diffy,
									it.sizez * POINTS_OF_DIRECTION[it.dir][point][2]+diffz
								)							
							);
						}
					}
				}

#ifdef USE_VBO
				glBindBuffer(GL_ARRAY_BUFFER, area->vbo[i]);
				getGlError();
				glBufferData(GL_ARRAY_BUFFER, size*sizeof(GLfloat), area->vbopointer[i], GL_STATIC_DRAW);
				getGlError();
				delete [] area->vbopointer[i];
				area->vbopointer[i] = 0;
#endif
				
				
#ifdef USE_GLLIST
				glNewList(area->gllist+i, GL_COMPILE);
				getGlError();
				
				glPushMatrix();
				glTranslatef(area->pos.x,area->pos.y,area->pos.z);

#ifdef USE_VBO			
				GLfloat* startpointer = 0;				
#else
				GLfloat* startpointer = area->vbopointer[i];
#endif
				glTexCoordPointer(2, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer);
				glNormalPointer(GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+2);
				glVertexPointer(3, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+5);

				getGlError();
				glDrawArrays(GL_TRIANGLES, 0, area->vbo_length[i]/8);
			
				glPopMatrix();
				glEndList();
				
#ifndef USE_VBO
				delete [] area->vbopointer[i];
				area->vbopointer[i] = 0;
#endif
#endif

			}
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
			//c->map->areas_with_gllist.erase(area);
			area->delete_opengl();
			area->delete_collision(c->movement->dynamicsWorld);
		}
	}
}



void Renderer::renderArea(Area* a, int l) {
	if(a->vbo_length[l]) {
#ifdef USE_GLLIST
		glCallList(a->gllist + l);
#else		
		glPushMatrix();
		glTranslatef(a->pos.x,a->pos.y,a->pos.z);
#ifdef USE_VBO
		glBindBuffer(GL_ARRAY_BUFFER, a->vbo[l]);
		getGlError();
	
		GLfloat* startpointer = 0;				
#else
		GLfloat* startpointer = a->vbopointer[l];
#endif
		//glInterleavedArrays(GL_T2F_N3F_V3F, sizeof(GLfloat)*8, startpointer);
		glTexCoordPointer(2, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer);
		glNormalPointer(GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+2);
		glVertexPointer(3, GL_FLOAT, sizeof(GL_FLOAT)*8, startpointer+5);

		getGlError();
		glDrawArrays(GL_TRIANGLES, 0, a->vbo_length[l]/8);
			
		//glDrawElements(GL_QUADS, area->vbo_created[i], GL_UNSIGNED_SHORT, 0);   //The starting point of the IBO
		getGlError();
		
		glPopMatrix();
#endif
	
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

void Renderer::render(PlayerPosition pos, double eye)
{
	pos.x += -eye*std::sin(pos.orientationHorizontal/180*M_PI);
	pos.y += eye*std::cos(pos.orientationHorizontal/180*M_PI);

	int start = SDL_GetTicks();
	
	long long vertex_saved = 0;
	long long area_saved = 0;
	int vertex_displayed = 0;
	int areas_in_viewport = 0;
	int displayed_vbos = 0;
	
	if(areasRendered<0) areasRendered = 0;
	areasRendered -= areasPerFrame;
	generateViewPort(pos);
	
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		a->show = areaInViewport(a->pos, pos);
		areas_in_viewport += a->show;
		if(a->state == Area::STATE_READY && (a->show || areasRendered < 0)) {
			if(a->needupdate_poly) {
				SDL_LockMutex(c->map->queue_mutex);
				c->map->to_generate.push_front(a);
				a->state = Area::STATE_GENERATE;
				SDL_UnlockMutex(c->map->queue_mutex);
			} else {
				generateArea(a);
			}
		}
		vertex_saved += a->vbo_size();
		area_saved += AREASIZE*sizeof(Material);
	}
	
	int generate = SDL_GetTicks()-start;

	if(enableFog)
		glEnable(GL_FOG);
	else
		glDisable(GL_FOG);
	
	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	glTranslatef(eye,0.0,0.0);
	// Calculate The Aspect Ratio Of The Window
	gluPerspective(angleOfVision/(1/360.0*M_PI), (GLfloat) c->ui->screenX / (GLfloat) c->ui->screenY, 0.01f, (visualRange>0?visualRange:1.0) * AREASIZE_X);

	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	
	glLoadIdentity();							// Reset The View
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
	
	int init = SDL_GetTicks()-generate-start;
	
	
	
	// state for rendering vbos
//	glEnable(GL_ALPHA_TEST);
//	glAlphaFunc(GL_GREATER, 0.3);
	glBindTexture( GL_TEXTURE_2D, texture[0] );
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);				
	getGlError();
	
	int k=0;
	glColor4f(1,1,1,0.5);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		if(a->state >= Area::STATE_GENERATE && a->show) {
			for(int d=0; d<DIRECTION_COUNT; d++) if(!a->dijsktra_direction_used[d]) {
				renderArea(a,d);
				vertex_displayed += a->polygons_count(d);
				displayed_vbos++;
			}
		}
	}
	
	renderObjects();
	
	int solid = SDL_GetTicks()-generate-init-start;
	
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_ZERO, GL_ONE);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		if(a->state >= Area::STATE_GENERATE && a->show) {
			renderArea(a,6);
			vertex_displayed += a->polygons_count(6);
			displayed_vbos++;
		}
	}
	
	glDepthFunc(GL_EQUAL);	
	glBlendFunc(GL_ONE, GL_SRC_COLOR);
	for(std::set<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		if(a->state >= Area::STATE_GENERATE && a->show) {
			renderArea(a,6);
			vertex_displayed += a->polygons_count(6);
			displayed_vbos++;
		}
	}
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDepthFunc(GL_LEQUAL);
	
	int trans = SDL_GetTicks()-solid-generate-init-start;
	
	
	stats[0] = std::max<double>(stats[0]*0.99, init);
	stats[1] = std::max<double>(stats[1]*0.99, generate);
	stats[2] = std::max<double>(stats[2]*0.99, solid);
	stats[3] = std::max<double>(stats[3]*0.99, trans);
	
	std::ostringstream out1(std::ostringstream::out);
	out1 << "Speicher: " << area_saved/1024/1024 << "+" << vertex_saved/1024/1024 << " MB, Polygone: " << vertex_displayed/1000 << " k, Areas: " << areas_in_viewport << ", VBOs: " << displayed_vbos;
	debug_output[0] = out1.str();
	
	std::ostringstream out2(std::ostringstream::out);
	out2 << "Init: " << stats[0] << ", Generate: " << stats[1] << ", Solid: " << stats[2] << ", Transparent: " << stats[3];
	debug_output[1] = out2.str();
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
	glBindTexture( GL_TEXTURE_2D, texture[0] );
	{
#ifdef ENABLE_OBJETS
		std::list<MovingObject*>::iterator it;
		for(it = c->map->objects.begin(); it != c->map->objects.end(); it++) {
			glPushMatrix();
			
			btTransform trans;
			(*it)->m->getWorldTransform(trans);		
			glTranslatef(trans.getOrigin().getX(),trans.getOrigin().getY(),trans.getOrigin().getZ());
			glRotatef( trans.getRotation().getAngle()*180/M_PI, trans.getRotation().getAxis().getX(), trans.getRotation().getAxis().getY(), trans.getRotation().getAxis().getZ());
			
			//glBindTexture( GL_TEXTURE_2D, texture[(*it)->tex] );
			glBegin( GL_QUADS );

			for(int i=0; i<DIRECTION_COUNT; i++) {
				glNormal3f( NORMAL_OF_DIRECTION[i][0], NORMAL_OF_DIRECTION[i][1], NORMAL_OF_DIRECTION[i][2]);                                     // Normal Pointing Towards Viewer
						
				for(int point=0; point < POINTS_PER_POLYGON; point++) {
					glTexCoord2f(
						(TEXTUR_POSITION_OF_DIRECTION[i][point][0]+(*it)->tex%16)/16.0,
						(TEXTUR_POSITION_OF_DIRECTION[i][point][1]+(*it)->tex/16)/16.0
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
#endif
	} {
		std::map<int, PlayerPosition>::iterator it;
		for(it = c->map->otherPlayers.begin(); it != c->map->otherPlayers.end(); it++) {
			glPushMatrix();
			
			PlayerPosition p = it->second;
			int tex = it->first % (NUMBER_OF_MATERIALS-1) + 1;
			
			glTranslatef(p.x, p.y, p.z);
			glRotatef(p.orientationHorizontal,0.0f,0.0f,1.0f);
			glRotatef(-p.orientationVertical,0.0f,1.0f,0.0f);
			
			//glBindTexture( GL_TEXTURE_2D, texture[(*it)->tex] );
			glBegin( GL_QUADS );

			for(int i=0; i<DIRECTION_COUNT; i++) {
				glNormal3f( NORMAL_OF_DIRECTION[i][0], NORMAL_OF_DIRECTION[i][1], NORMAL_OF_DIRECTION[i][2]);                                     // Normal Pointing Towards Viewer
						
				for(int point=0; point < POINTS_PER_POLYGON; point++) {
					glTexCoord2f(
						(TEXTUR_POSITION_OF_DIRECTION[i][point][0]+tex%16)/16.0,
						(TEXTUR_POSITION_OF_DIRECTION[i][point][1]+tex/16)/16.0
					);
					glVertex3f(
						(POINTS_OF_DIRECTION[i][point][0]*2-1)*0.6/2,
						(POINTS_OF_DIRECTION[i][point][1]*2-1)*0.6/2,
						(POINTS_OF_DIRECTION[i][point][2]*2-1)*1.5/2-0.5
					);
				}
			}
			glEnd();

			glPopMatrix();
		}
	}
}
