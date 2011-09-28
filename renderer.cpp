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
#include "utils.h"


using namespace std;
namespace fs = boost::filesystem;


void getGlError(std::string meldung = "") { 

	
	GLenum errCode;
	const GLubyte *errString;

	if ((errCode = glGetError()) != GL_NO_ERROR) {
		errString = gluErrorString(errCode);
		fprintf (stderr, "%s OpenGL Error: %s\n",meldung.c_str(), errString);
		//throw std::exception();
	}
	
}

void printInfoLog(GLhandleARB obj)
{
    int infologLength = 0;
    int charsWritten  = 0;
    char *infoLog;

    glGetObjectParameterivARB(obj, GL_OBJECT_INFO_LOG_LENGTH_ARB,
					 &infologLength);

    if (infologLength > 0)
    {
	infoLog = (char *)malloc(infologLength);
	glGetInfoLogARB(obj, infologLength, &charsWritten, infoLog);
	printf("%s\n",infoLog);
	free(infoLog);
    }
}

Renderer::Renderer(Controller* controller)
{
	c = controller;
	time = 0;
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
	texture_size		= c["textureSize"].as<int>();

	workingDirectory = c["workingDirectory"].as<fs::path>();
	dataDirectory = c["dataDirectory"].as<fs::path>();
	localDirectory = c["localDirectory"].as<fs::path>();

	areasPerFrame		= c["areasPerFrameRendering"].as<int>();
	highlightWholePlane	= c["highlightWholePlane"].as<bool>();
	textureFilterMethod = c["textureFilterMethod"].as<int>();
}

void Renderer::init()
{	
	
	getGlError();
	// Set the OpenGL state
	glClearColor(bgColor[0],bgColor[1], bgColor[2], bgColor[3]);	// Background
	glClearDepth(1.0f);													// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);											// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);												// The Type Of Depth Testing To Do
	//glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);					// Really Nice Perspective Calculations
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);					// Really Nice Perspective Calculations
	glShadeModel(GL_FLAT);
	glDisable(GL_DITHER);
	glEnable(GL_CULL_FACE);
	
	unsigned char* pixels = new unsigned char[4*texture_size*texture_size*NUMBER_OF_MATERIALS];
	for(int i=0; i<4*texture_size*texture_size*NUMBER_OF_MATERIALS; i++) {
		pixels[i] = 255;
	}

	if(glewIsSupported("GL_EXT_texture_array")) {
    TEXTURE_TYPE = GL_TEXTURE_2D_ARRAY;
		std::cout << "GL_EXT_texture_array is supported" << std::endl;
  } else {
    TEXTURE_TYPE = GL_TEXTURE_3D;
    std::cout << "disabling texture arrays" << std::endl;
  }

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
			for(int x=0; x<texture_size; x++) for(int y=0; y<texture_size; y++) {
				
				for(int c=0; c<3; c++) 
					pixels[c + x*4 + y*4*texture_size + i*4*texture_size*texture_size] = p[c + (x*surface->w/texture_size)*3 + (y*surface->h/texture_size)*surface->pitch];
				
				pixels[3 + x*4 + y*4*texture_size + i*4*texture_size*texture_size] = ((x/4)%3)&&((y/4)%3)?0:255;
				
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
	
	glBindTexture( TEXTURE_TYPE, texture[0] );
	getGlError("glBindTexture");

	// Set the texture's stretching properties
	glTexParameterf( TEXTURE_TYPE, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( TEXTURE_TYPE, GL_TEXTURE_WRAP_T, GL_REPEAT );
	
	if(!glewIsSupported("GL_EXT_texture_array") && textureFilterMethod > 2)
		textureFilterMethod = 2;
	if(!glewIsSupported( "GL_ARB_framebuffer_object") && textureFilterMethod > 2)
		textureFilterMethod = 2;
	if(!glewIsSupported( "GL_EXT_texture_filter_anisotropic") && textureFilterMethod > 3)
		textureFilterMethod = 3;
	
	if ( textureFilterMethod >= 4) {
		float maxAni;
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAni );
		glTexParameterf( TEXTURE_TYPE, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAni );  
	} 
	if(textureFilterMethod >= 3){
		glTexParameteri( TEXTURE_TYPE, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glTexParameteri( TEXTURE_TYPE, GL_TEXTURE_MAG_FILTER, GL_LINEAR );		
		glTexParameteri( TEXTURE_TYPE, GL_GENERATE_MIPMAP, true);
	}
	else if(textureFilterMethod >= 2){
		glTexParameteri( TEXTURE_TYPE, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( TEXTURE_TYPE, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}
	else{
		glTexParameteri( TEXTURE_TYPE, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexParameteri( TEXTURE_TYPE, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	}
	
	glTexImage3D(TEXTURE_TYPE, 0,GL_RGBA, texture_size, texture_size, NUMBER_OF_MATERIALS, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	delete [] pixels;
	getGlError("glTexImage");
	
	if (GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader)
		printf("Ready for GLSL\n");
	else {
		printf("Not totally ready for GLSL :(  \n");
		exit(1);
	}
	
	std::cout << "Shader: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
	
	shader.solid_po = glCreateProgram();
	getGlError("glCreateProgram");
	
	shader.solid_vs = glCreateShader(GL_VERTEX_SHADER);
	getGlError("glCreateShader vs");
	
	shader.solid_fs = glCreateShader(GL_FRAGMENT_SHADER);
	getGlError("glCreateShader fs");
	
	char shader_src[16*1024];
	const char *shader_pointer = shader_src;
	int shader_src_length;
	
	std::ifstream shader_stream("shader/solid_vertex.c");
	shader_stream.read(shader_src, 16*1024);
	shader_src_length = shader_stream.gcount();
	
	glShaderSource(shader.solid_vs, 1, &shader_pointer, &shader_src_length);
	getGlError("glShaderSource");
	
	glCompileShader(shader.solid_vs);
	getGlError("glCompileShader vs");
	printInfoLog(shader.solid_vs);
	
	std::ifstream shader_stream2("shader/solid_fragment.c");
	shader_stream2.read(shader_src, 16*1024);
	shader_src_length = shader_stream2.gcount();
	
	glShaderSource(shader.solid_fs, 1, &shader_pointer, &shader_src_length);
	getGlError("glShaderSource fs");
	
	glCompileShader(shader.solid_fs);
	getGlError("glCompileShader fs");
	printInfoLog(shader.solid_fs);
	
	glAttachShader(shader.solid_po, shader.solid_vs);
	getGlError("glAttachShader vs");
	glAttachShader(shader.solid_po, shader.solid_fs);
	getGlError("glAttachShader fs");
	
	glBindAttribLocation(shader.solid_po,0,"bPos");
	
	glLinkProgram(shader.solid_po);
	getGlError("glLinkProgram shader_po");
	printInfoLog(shader.solid_po);
	
	glUseProgram(shader.solid_po);
	
	// get uniform vars
	shader.position = glGetUniformLocation(shader.solid_po, "position");
	shader.bgColor = glGetUniformLocation(shader.solid_po,"bgColor");
	shader.tex = glGetUniformLocation(shader.solid_po, "tex");
	shader.time = glGetUniformLocation(shader.solid_po, "time");
	shader.visualRange = glGetUniformLocation(shader.solid_po, "visualRange");
	shader.fogStart = glGetUniformLocation(shader.solid_po, "fogStart");
	shader.LightAmbient = glGetUniformLocation(shader.solid_po, "LightAmbient");
	shader.LightDiffuseDirectionA = glGetUniformLocation(shader.solid_po, "LightDiffuseDirectionA");
	shader.LightDiffuseDirectionB = glGetUniformLocation(shader.solid_po, "LightDiffuseDirectionB");
	
	// get attribute vars
	shader.normal = glGetAttribLocation(shader.solid_po, "normal");
	shader.bPos = glGetAttribLocation(shader.solid_po, "bPos");
	shader.tPos = glGetAttribLocation(shader.solid_po, "tPos");

	// and set it
	glUniform4fv(shader.bgColor,1,bgColor);
	glUniform1i(shader.tex, 0);
	glUniform1f(shader.time, 0.0);
	glUniform1f(shader.visualRange, visualRange*AREASIZE_X);
	glUniform1f(shader.fogStart, visualRange*AREASIZE_X*fogStartFactor);
	glUniform4f(shader.LightAmbient, 0.3, 0.3, 0.3, 1.0);
	glUniform3f(shader.LightDiffuseDirectionA, 0.7, 0.3, sqrt(1.0-0.49-0.09));
	glUniform3f(shader.LightDiffuseDirectionB, -0.4, -0.4, sqrt(1.0-0.16-0.16));
	
	getGlError();
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
			
			int max_counts = 0;
      
			for(int i=0; i<NUMBER_OF_LISTS; i++) {
				if(!area->polys_list_size[i]) continue;
				
				int points[] = {0,1,2,0,2,3};
				
				int size = area->polys_list_size[i]*8*sizeof(points)/sizeof(int);
				
				if(max_counts < size)
					max_counts = size;
				
				area->vbo_length[i] = size;
				area->vbopointer[i] = new unsigned char[size];
				int vbocounter = 0;
				
				for(int k=area->polys_list_start[i]; k<area->polys_list_size[i]+area->polys_list_start[i]; k++) {
					polygon it = area->polys_list[k];
					
					int diffx = it.posx;
					int diffy = it.posy;
					int diffz = it.posz;
					
					//std::cout << k << " " << diffx << " " << diffy << " " << diffz << std::endl;
					
					for(int j=0; j < sizeof(points)/sizeof(int); j++) {
						int point = points[j];
						
						int zoomx = 1;
						int zoomy = 1;
						if(NORMAL_OF_DIRECTION[it.dir][0]) {
							zoomx = it.sizey;
							zoomy = it.sizez;
						} else if(NORMAL_OF_DIRECTION[it.dir][1]) {
							zoomx = it.sizex;
							zoomy = it.sizez;
						} else {
							zoomx = it.sizex;
							zoomy = it.sizey;
							
						}
						
						// texture
						area->vbopointer[i][vbocounter+0] = zoomx * TEXTUR_POSITION_OF_DIRECTION[it.dir][point][0];
						area->vbopointer[i][vbocounter+1] = zoomy * TEXTUR_POSITION_OF_DIRECTION[it.dir][point][1];
						area->vbopointer[i][vbocounter+2] = it.m;
						
						// normale
						//area->vbopointer[i][vbocounter+2] = NORMAL_OF_DIRECTION[it.dir][0];
						//area->vbopointer[i][vbocounter+3] = NORMAL_OF_DIRECTION[it.dir][1];
						//area->vbopointer[i][vbocounter+4] = NORMAL_OF_DIRECTION[it.dir][2];
						area->vbopointer[i][vbocounter+3] = it.dir;
						
						// vertex
						area->vbopointer[i][vbocounter+4] = it.sizex * POINTS_OF_DIRECTION[it.dir][point][0]+diffx;
						area->vbopointer[i][vbocounter+5] = it.sizey * POINTS_OF_DIRECTION[it.dir][point][1]+diffy;
						area->vbopointer[i][vbocounter+6] = it.sizez * POINTS_OF_DIRECTION[it.dir][point][2]+diffz;
						
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
				glBufferData(GL_ARRAY_BUFFER, size, area->vbopointer[i], GL_STATIC_DRAW);
				getGlError();
				delete [] area->vbopointer[i];
				area->vbopointer[i] = 0;
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
		getGlError();
		glPushMatrix();
		glTranslatef(a->pos.x,a->pos.y,a->pos.z);
#ifdef USE_VBO
		glBindBuffer(GL_ARRAY_BUFFER, a->vbo[l]);
		getGlError();
	
		unsigned char* startpointer = 0;				
#else
		unsigned char* startpointer = a->vbopointer[l];
#endif		
		glVertexAttribPointer(shader.bPos,3,GL_UNSIGNED_BYTE,GL_FALSE,8,startpointer+4);
		glVertexAttribPointer(shader.normal,1,GL_UNSIGNED_BYTE,GL_FALSE,8,startpointer+3);
		glVertexAttribPointer(shader.tPos,3,GL_UNSIGNED_BYTE,GL_FALSE,8,startpointer+0);

		getGlError();
		glDrawArrays(GL_TRIANGLES, 0, a->vbo_length[l]/8);
			
		//glDrawElements(GL_QUADS, area->vbo_created[i], GL_UNSIGNED_SHORT, 0);   //The starting point of the IBO
		getGlError();
		glPopMatrix();
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
		 && (abs(erg.data[0][1])/(abs(erg.data[0][0])+AREASIZE_X*1.7321) < 1.5*(double(c->ui->screenX) / c->ui->screenY) )	// seitlich ausm sichtbereich
		 && (abs(erg.data[0][2])/(abs(erg.data[0][0])+AREASIZE_X*1.7321) < 1.5 ) // oben/unten aus dem Bildbereich
		 ;
}

void Renderer::render(PlayerPosition pos, double eye)
{
	glUseProgram(shader.solid_po);
	glUniform1f(shader.time, float(time)/1000);
	glUniform3f(shader.position, pos.x, pos.y, pos.z);
	
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
	
	for(std::list<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
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

	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	glTranslatef(eye,0.0,0.0);
	// Calculate The Aspect Ratio Of The Window
	gluPerspective(angleOfVision/(1/360.0*M_PI), (GLfloat) c->ui->screenX / (GLfloat) c->ui->screenY, 0.01f, (visualRange>0?visualRange:1.0) * AREASIZE_X);

	glScalef(-1,1,1);
	glRotatef(90.0,0.0f,0.0f,1.0f);
	glRotatef(90.0,0.0f,1.0f,0.0f);
		
	//Mausbewegung
	glRotatef(pos.orientationVertical,0.0f,1.0f,0.0f);
	glRotatef(-pos.orientationHorizontal,0.0f,0.0f,1.0f);
	
	//Eigene Position
	glTranslatef(-(pos.x), -(pos.y), -(pos.z));
	
	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	glLoadIdentity();							// Reset The View
	
	int init = SDL_GetTicks()-generate-start;
	
	// state for rendering vbos
	//glEnable(GL_ALPHA_TEST);
	//glAlphaFunc(GL_GREATER, 0.3);			
	glEnableVertexAttribArray(shader.normal);
	glEnableVertexAttribArray(shader.bPos);
	glEnableVertexAttribArray(shader.tPos);
	getGlError();
	
	int k=0;
	glColor4f(1,1,1,0.5);
	for(std::list<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		if(a->state >= Area::STATE_GENERATE && a->show) {
			for(int d=0; d<DIRECTION_COUNT; d++) if(!a->dijsktra_direction_used[d]) {
				renderArea(a,d);
				vertex_displayed += a->polygons_count(d);
				displayed_vbos++;
			}
		}
	}
	
	getGlError();
	renderObjects();
	
	getGlError();
	int solid = SDL_GetTicks()-generate-init-start;
	
	glEnable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_ZERO, GL_ONE);
	for(std::list<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
		Area* a = *it;
		if(a->state >= Area::STATE_GENERATE && a->show) {
			renderArea(a,6);
			vertex_displayed += a->polygons_count(6);
			displayed_vbos++;
		}
	}
	
	glDepthFunc(GL_EQUAL);	
	glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR);
	for(std::list<Area*>::iterator it = c->map->areas_with_gllist.begin(); it != c->map->areas_with_gllist.end(); it++)	{
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
	out1 << "Polygone: " << vertex_displayed/1000 << " k, Areas: " << areas_in_viewport << ", VBOs: " << displayed_vbos;
	debug_output[0] = out1.str();
	
	std::ostringstream out2(std::ostringstream::out);
	out2 << "Init: " << stats[0] << ", Generate: " << stats[1] << ", Solid: " << stats[2] << ", Transparent: " << stats[3];
	debug_output[1] = out2.str();
	
	glUseProgram(0);
}

void Renderer::highlightBlockDirection(BlockPosition block, DIRECTION direct){
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
}

void Renderer::renderObjects() {
	{
#ifdef ENABLE_OBJETS
		std::list<MovingObject*>::iterator it;
		for(it = c->map->objects.begin(); it != c->map->objects.end(); it++) {
			glPushMatrix();
			
			btTransform trans;
			(*it)->m->getWorldTransform(trans);		
			glTranslatef(trans.getOrigin().getX(),trans.getOrigin().getY(),trans.getOrigin().getZ());
			glRotatef( trans.getRotation().getAngle()*180/M_PI, trans.getRotation().getAxis().getX(), trans.getRotation().getAxis().getY(), trans.getRotation().getAxis().getZ());
			
			glBegin( GL_QUADS );

			for(int i=0; i<DIRECTION_COUNT; i++) {
				glVertexAttrib1f( shader.normal,i);
				for(int point=0; point < POINTS_PER_POLYGON; point++) {
					glVertexAttrib3f( shader.tPos,
						TEXTUR_POSITION_OF_DIRECTION[i][point][0],
						TEXTUR_POSITION_OF_DIRECTION[i][point][1],
						(*it)->tex
					);
					glVertexAttrib3f( shader.bPos,
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
	} 
		getGlError();
	{
		std::map<int, OtherPlayer>::iterator it;
		for(it = c->map->otherPlayers.begin(); it != c->map->otherPlayers.end(); it++) {
			if(it->second.visible){
				glPushMatrix();
				
				PlayerPosition p = it->second.pos;
				std::string pName = it->second.name;
				int tex = it->first % (NUMBER_OF_MATERIALS-1) + 1;
				
				glTranslatef(p.x, p.y, p.z);
				glRotatef(p.orientationHorizontal,0.0f,0.0f,1.0f);
				glRotatef(-p.orientationVertical,0.0f,1.0f,0.0f);

				//glBindTexture( GL_TEXTURE_2D, texture[(*it)->tex] );
				//c->ui->renderText(0.,1,pName.c_str());

				glBegin( GL_QUADS );
				
				for(int i=0; i<DIRECTION_COUNT; i++) {
					glVertexAttrib1f( shader.normal,i);
					for(int point=0; point < POINTS_PER_POLYGON; point++) {
						glVertexAttrib3f( shader.tPos,
							TEXTUR_POSITION_OF_DIRECTION[i][point][0],
							TEXTUR_POSITION_OF_DIRECTION[i][point][1],
							tex
						);
						glVertexAttrib3f( shader.bPos,
							(POINTS_OF_DIRECTION[i][point][0]*2-1)*0.6/2,
							(POINTS_OF_DIRECTION[i][point][1]*2-1)*0.6/2,
							(POINTS_OF_DIRECTION[i][point][2]*2-1)*1.5/2-0.5
						);
					}
				}
		getGlError();
				glEnd();
		getGlError();
				glTranslatef(0,0.3,0.35);
				glRotatef(-90,0,0,1);
				glRotatef(90,1,0,0);
				glScalef(0.01,0.01,0.01);
				glDisable(GL_CULL_FACE);
				glUseProgram(0);
				c->ui->renderText(0.,0,pName.c_str());
				glUseProgram(shader.solid_po);
				glEnable(GL_CULL_FACE);
				
				glPopMatrix();
			}
		}
	}
}
