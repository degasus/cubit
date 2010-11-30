#include <iostream>

#include "matrix.h"
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <cmath>


using namespace std;


const int RUN_GAME_LOOP = 1;

//GL vars
const int numberOfTex = 10; //Number of textures (including Air, 0)
const char* texFiles[numberOfTex] = {"", "tex/wood.bmp", "tex/bricks.bmp", "tex/grass.bmp", "tex/grass2.bmp", "tex/BlackMarble.bmp", "tex/BlueCrackMarble.bmp", "tex/BrightPurpleMarble.bmp",  "tex/BrownSwirlMarble.bmp", "tex/SwirlyGrayMarble.bmp"};
GLuint texture[numberOfTex];
GLuint box;
//fog
GLfloat fogColor[4]= {0.6f, 0.7f, 0.8f, 1.0f};		// Fog Color
float fogDense = 0.6f;
float fogStartFactor = 0.40f;

//SDL vars
SDL_Surface *screen;
bool done = 0;


//VARIABLES

//drehen
int x_orig;
int y_orig;
float x = 135;
float y = 20;
bool enable_rotate = 0;

//bewegen
float posX = 0.0f;
float posY = -20.0f;
float posZ = 0.0f;

float offset = 0.3f;
float offsetFallen = 0.2;
float offsetY = 0.1f;

float speedY = 0.0f;
float accelY = -0.02f;

float personSize = 1.5f;

float slowMovementSpeed = 0.03f;
float fastMovementSpeed = 0.2f;
float movementSpeed = 0.2f;
float fastSpeedMultiplier = 5.72341f;

//PointingOn
int lastPointingOn = -2;
int pointingOnX = -1;
int pointingOnY = -1;
int pointingOnZ = -1;

//Frame counter since last building/removing a block during addBlock or removeBlock is true
int lastAdd = 0;
int lastRemove = 0;
//Delay until building/removing next block in Frames
int addDelay = 20;
int removeDelay = 20;

bool xDown = false;
bool xUp = false;
bool zDown = false;
bool zUp = false;
bool jump = false;
bool fastSpeed = false;
bool duck = false;
bool removeBlock = false;
bool addBlock = false;

//Frame size
int screenX = 1024;
int screenY = 768;
int noFullX = 640;
int noFullY = 480;
bool isFullscreen = true;


//CONSTANTS

//Keys
const int k_xDown = SDLK_a;
const int k_xUp = SDLK_d;
const int k_yDown = SDLK_e;
const int k_yUp = SDLK_q;
const int k_zDown = SDLK_w;
const int k_zUp = SDLK_s;
const int k_jump = SDLK_SPACE;
const int k_fastSpeed = SDLK_f;
const int k_QUIT = SDLK_ESCAPE;
const int k_Duck = SDLK_LSHIFT;

// Landschaft
int xsize = 32;
int ysize = 32;
int zsize = 32;
unsigned char *landschaft;


//PROCEDURES HEAD

Uint32 GameLoopTimer(Uint32 interval, void* param);
void HandleUserEvents(SDL_Event* event);
void draw();
void calcBuilding();
void calcMovement();
void calcPointingOn();
void highlightPointingOn();
int calcPointingOnInBlock(double *vectorX, double *vectorY, double *vectorZ);
void loadTexture(const char*, int);
void activateTexture(int);
void gen_gllist();
void gen_land();



void initGL() {
	// Slightly different SDL initialization
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 ) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}

	const SDL_VideoInfo *vi = SDL_GetVideoInfo();
	screenX = vi->current_w;
	screenY = vi->current_h;

	//screenX = 1366;
	//screenY = 786;

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

	screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_RESIZABLE | SDL_FULLSCREEN);
	if ( !screen ) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
	}

	// Set the OpenGL state after creating the context with SDL_SetVideoMode

	glEnable(GL_TEXTURE_2D);											// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);											// Enable Smooth Shading
	glClearColor(fogColor[0],fogColor[1], fogColor[2], fogColor[3]);	// Black Background
	glClearDepth(1.0f);													// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);											// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);												// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);					// Really Nice Perspective Calculations
	glHint(GL_LINE_SMOOTH, GL_NICEST);
	glEnable(GL_LINE_SMOOTH);

	glViewport(0, 0, screenX, screenY);	// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat) screenX / (GLfloat) screenY, 0.01f, 1000.0f);

	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	glLoadIdentity();			// Reset The Modelview Matrix

	GLfloat LightAmbient[]  = { 0.5f, 0.5f, 0.5f, 1.0f };
	GLfloat LightDiffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat LightPosition[] = { 0.5f, 1.0f, 5.0f, 1.0f };

	glLightfv(GL_LIGHT1, GL_AMBIENT,  LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE,  LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);

	glFogi(GL_FOG_MODE, GL_LINEAR);		// Fog Mode
	glFogfv(GL_FOG_COLOR, fogColor);	// Set Fog Color
	glFogf(GL_FOG_DENSITY, fogDense);	// How Dense Will The Fog Be
	glHint(GL_FOG_HINT, GL_DONT_CARE);	// Fog Hint Value
	glFogf(GL_FOG_START, xsize*fogStartFactor);
	glFogf(GL_FOG_END, xsize);
  	glEnable(GL_FOG);					// Enables GL_FOG

	enable_rotate = 1;
	SDL_ShowCursor(SDL_DISABLE);
	x_orig = screenX/2;
	y_orig = screenY/2;

  //generate Textures
  glGenTextures( numberOfTex, texture );

  for(int i = 1; i < numberOfTex; i++){
    loadTexture(texFiles[i], i);
  }

  gen_land();
  
  // reserviert Speicher für eine OpenGL Liste
	box=glGenLists(1);
  gen_gllist();


	SDL_TimerID timer = SDL_AddTimer(40,GameLoopTimer,0);

}
int PointingOn;
void debug(){
	double x = posX - floor(posX);
	double y = posY + personSize - floor(posY + personSize);
	double z = posZ - floor(posZ);
	PointingOn =  calcPointingOnInBlock(&x, &y, &z);
	
/*	cout << "x: " << x << endl;
	cout << "y: " << y << endl;
	cout << "z: " << z << endl;
*/	
}

void EventLoop(void)
{
	SDL_Event event;

	while((!done) && (SDL_WaitEvent(&event))) {
		switch(event.type) {
			case SDL_USEREVENT:
				calcMovement();
				calcBuilding();
				HandleUserEvents(&event);
				debug();
				break;
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
					case k_xDown:
						xDown = true;
						break;
					case k_xUp:
						xUp = true;
						break;
					case k_zUp:
						zUp = true;
						break;
					case k_zDown:
						zDown = true;
						break;
					case k_jump:
						jump = true;
						break;
                    case k_fastSpeed:
						if(event.key.keysym.mod & KMOD_CTRL){
							if(isFullscreen){
								isFullscreen = false;
								screenX = noFullX;
								screenY = noFullY;

								screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_RESIZABLE);
							}
							else{
								isFullscreen = true;
					            const SDL_VideoInfo *vi = SDL_GetVideoInfo();
								screenX = vi->current_w;
								screenY = vi->current_h;

								screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_RESIZABLE | SDL_FULLSCREEN);
							}
							
							glViewport(0, 0, screenX, screenY);						// Reset The Current Viewport

							glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
							glLoadIdentity();							// Reset The Projection Matrix

							// Calculate The Aspect Ratio Of The Window
							gluPerspective(45.0f,(GLfloat)screenX/(GLfloat)screenY,0.1,100.0f);

							glMatrixMode(GL_MODELVIEW);

							x_orig = screenX/2;
							y_orig = screenY/2;
						}
						else{
                        	fastSpeed = true;
						}
                        break;
                    case k_QUIT:
                        done = true;
                        break;
                    case k_Duck:
                        duck = true;
                        offset *= 2;
                        offsetFallen *= 2;
                        movementSpeed = slowMovementSpeed;
                        break;
	                case SDLK_r:
		                  gen_land();
		                  gen_gllist();
		                  break;
	                case SDLK_PLUS:
		                  xsize*=2;
		                  zsize*=2;
		                  ysize*=2;
		                  gen_land();
		                  gen_gllist();
		                  glFogf(GL_FOG_START, xsize*fogStartFactor);
		                  glFogf(GL_FOG_END, xsize);
		                  break;
	                case SDLK_MINUS:
		                  xsize/=2;
		                  zsize/=2;
		                  ysize/=2;
		                  gen_land();
		                  gen_gllist();
		                  glFogf(GL_FOG_START, xsize*fogStartFactor);
		                  glFogf(GL_FOG_END, xsize);
		                  break;
                    case SDLK_u:
	                      fogDense += 0.1f;
	                      if (fogDense > 1)
	                        fogDense = 1;
                        glFogf(GL_FOG_DENSITY, fogDense);
                        break;
                    case SDLK_j:
	                      fogDense -= 0.1f;
	                      if (fogDense < 0)
	                        fogDense = 0;
                        glFogf(GL_FOG_DENSITY, fogDense);
                        break;
					case SDLK_m:
						if(event.key.keysym.mod & KMOD_CTRL){
			                enable_rotate = !enable_rotate;
							if(enable_rotate){
								SDL_ShowCursor(SDL_DISABLE);
							}
							else{
								SDL_ShowCursor(SDL_ENABLE);
							}
						}
						break;
                    default:
                        break;
    			}
			break;
		
            case SDL_KEYUP:
                switch(event.key.keysym.sym){
		            case k_xDown:
		                xDown = false;
		                break;
		            case k_xUp:
		                xUp = false;
		                break;
		            case k_zUp:
		                zUp = false;
		                break;
		            case k_zDown:
		                zDown = false;
		                break;
		            case k_fastSpeed:
		                fastSpeed = false;
		                break;
		            case k_jump:
		                jump = false;
		                break;
		            case k_Duck:
		                duck = false;
		                offset /= 2;
		                offsetFallen /= 2;
		                movementSpeed = fastMovementSpeed;
		                break;
				
                   default:
                        //print keycode of unregistered key
                        //cout << event.key.keysym.sym << endl;
                        break;
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                switch(event.button.button){
                    case SDL_BUTTON_LEFT:
                      lastAdd = addDelay;
                      addBlock = true;
			                break;
                    case SDL_BUTTON_RIGHT:
                      lastRemove = removeDelay;
                      removeBlock = true;
			                break;

                }
              break;

            case SDL_MOUSEBUTTONUP:
                switch(event.button.button){
                    case SDL_BUTTON_LEFT:
                      addBlock = false;
			                break;
                    case SDL_BUTTON_RIGHT:
                      removeBlock = false;
			                break;
                }
		          //if (landschaft[((int)posX)*ysize*zsize + ((int)(posY-0.5f))*zsize + (int)posZ] != 0){

		          //}
              break;

            case SDL_MOUSEMOTION:
		          if(enable_rotate) {
			          x += float(event.motion.x-x_orig)/10;
                      if (x >  360) x -= 360;
                      if (x < -360) x += 360;

			          y += float(event.motion.y-y_orig)/10;
                      if (y > 90) y = 90;
                      if (y < -90) y = -90;

			          SDL_WarpMouse(x_orig, y_orig);
		          }
              break;

            case SDL_QUIT:
              done = true;
              break;

	    case SDL_VIDEORESIZE:
        screenX = event.resize.w;
        screenY = event.resize.h;
		    screen = SDL_SetVideoMode( event.resize.w, event.resize.h, 32, SDL_OPENGL | SDL_RESIZABLE);
		    glViewport(0, 0, event.resize.w, event.resize.h);						// Reset The Current Viewport

			glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
			glLoadIdentity();							// Reset The Projection Matrix

			// Calculate The Aspect Ratio Of The Window
			gluPerspective(45.0f,(GLfloat)event.resize.w/(GLfloat)event.resize.h,0.1,100.0f);

			glMatrixMode(GL_MODELVIEW);

			x_orig = screenX/2;
			y_orig = screenY/2;

		    break;


            default:
	//	cout << "default" << endl;
                break;
        }

    }

}

void calcBuilding(){
	if(addBlock){
		lastAdd++;
		if((lastAdd >= addDelay || fastSpeed)  && pointingOnX+pointingOnY+pointingOnZ != -3
		   ){
			int x = pointingOnX;
			int y = pointingOnY;
			int z = pointingOnZ;
			//Front
			if(lastPointingOn == 1){
				z--;
			}
			//Back
			if(lastPointingOn == 0){
				z++;
			}
			//Top
			if(lastPointingOn == 3){
				y--;
			}
			//Bottom
			if(lastPointingOn == 2){
				y++;
			}
			//Right
			if(lastPointingOn == 5){
				x--;
			}
			//Left
			if(lastPointingOn == 4){
				x++;
			}
			if(x != floor(posX) || (y != floor(posY) && y != floor(posY)+1) || z != floor(posZ)){
				landschaft[x*ysize*zsize + y*zsize  + z] = 1;
				gen_gllist();
				lastAdd = 0;
		   }
		}
	}
	else if(removeBlock && pointingOnX+pointingOnY+pointingOnZ != -3){
		lastRemove++;
		if(lastRemove >= removeDelay || fastSpeed){
			landschaft[pointingOnX*ysize*zsize + pointingOnY*zsize  + pointingOnZ] = 0;
			gen_gllist();
			lastRemove = 0;
		}
	}
}

void calcMovement()
{
    float mult = 1.0f;
    if (fastSpeed){
      mult = fastSpeedMultiplier;
    }

    //Y-Bewegung

    //Fallen
    if ((landschaft[((int)(posX+offsetFallen))*ysize*zsize + ((int)posY)*zsize + (int)posZ] == 0)
        && (landschaft[((int)(posX-offsetFallen))*ysize*zsize + ((int)(posY))*zsize + (int)posZ] == 0)
        && (landschaft[((int)posX)*ysize*zsize + ((int)(posY))*zsize + (int)(posZ+offsetFallen)] == 0)
        && (landschaft[((int)posX)*ysize*zsize + ((int)posY)*zsize + (int)(posZ-offsetFallen)] == 0))
    {
      speedY += accelY;
      if (speedY >= 0.99f)
        speedY = 0.99f;
    }
    else if(speedY < 0){
      speedY = 0;
    }

    //Springen
    if (landschaft[((int)posX)*ysize*zsize + ((int)(posY-0.1f))*zsize + (int)posZ] != 0 && jump)
      speedY = 0.24f*mult;

    //Movement
    float posXold = posX;
    float posYold = posY;
    float posZold = posZ;

    //Horizontale Bewegung
    if (xDown){
        posX -= movementSpeed*cos(2*M_PI*x/360)*mult;
        posZ -= movementSpeed*sin(2*M_PI*x/360)*mult;
    }
    if (xUp){
        posX += movementSpeed*cos(2*M_PI*x/360)*mult;
        posZ += movementSpeed*sin(2*M_PI*x/360)*mult;
    }
    if (zDown){
        posZ -= movementSpeed*cos(2*M_PI*x/360)*mult;
        posX += movementSpeed*sin(2*M_PI*x/360)*mult;
    }
    if (zUp){
        posZ += movementSpeed*cos(2*M_PI*x/360)*mult;
        posX -= movementSpeed*sin(2*M_PI*x/360)*mult;
    }
    //Vertikale Bewegung
    posY += speedY;

    //Collision Detection
    //X-Richtung
    if (   (landschaft[((int)(posX+offset))*ysize*zsize + ((int)posYold)*zsize + (int)posZold] != 0 && posX > posXold)
        || (landschaft[((int)(posX+offset))*ysize*zsize + ((int)(posYold+1.0f))*zsize + (int)posZold] != 0 && posX > posXold)
        || (landschaft[((int)(posX-offset))*ysize*zsize + ((int)posYold)*zsize + (int)posZold] != 0 && posX < posXold)
        || (landschaft[((int)(posX-offset))*ysize*zsize + ((int)(posYold+1.0f))*zsize + (int)posZold] != 0 && posX < posXold))
      posX = posXold;
    //Z-Richtung
    if (   (landschaft[((int)posXold)*ysize*zsize + ((int)posYold)*zsize + (int)(posZ+offset)] != 0 && posZ > posZold)
        || (landschaft[((int)posXold)*ysize*zsize + ((int)(posYold+1.0f))*zsize + (int)(posZ+offset)] != 0 && posZ > posZold)
        || (landschaft[((int)posXold)*ysize*zsize + ((int)posYold)*zsize + (int)(posZ-offset)] != 0 && posZ < posZold)
        || (landschaft[((int)posXold)*ysize*zsize + ((int)(posYold+1.0f))*zsize + (int)(posZ-offset)] != 0 && posZ < posZold))
      posZ = posZold;
    //Y-Richtung
    if ((landschaft[((int)posX)*ysize*zsize + ((int)(posY+personSize+offsetY))*zsize + (int)(posZ)] != 0)){
      posY = posYold;
      speedY = 0;
    }

    //Randbegrenzung
    if(posX<=0) posX = 0;
    if(posX>=xsize-1) posX = xsize-1;
    if(posZ<=0) posZ = 0;
    if(posZ>=zsize-1) posZ = zsize-1;
    if(posY<=1) posY = 1;
    if(posY>=ysize-2) posY = ysize-2;


    //Falls Person in Block "Aufzug" nach oben
    if (landschaft[((int)posX)*ysize*zsize + ((int)posY)*zsize + (int)posZ] != 0){
      posY = int(posY)+1;
      speedY = 0;
    }

    //Ducken animieren
    if (duck && personSize > 1.3f){
      personSize -= 0.05;
    }
    if (!duck && personSize < 1.5f){
      personSize += 0.05;
    }
}

void calcPointingOn(){
	double lastX = (posX) - floor(posX);
	double lastY = posY - personSize - floor(posY - personSize);
	double lastZ = (posZ) - floor(posZ);

	int blockX = floor(posX);
	int blockY = floor(posY + personSize);
	int blockZ = floor(posZ);
	
	lastPointingOn = -2;

	double distanceQ = 0;
	int counter = 0;

	while(landschaft[blockX*ysize*zsize + blockY*zsize  + blockZ] == 0 && distanceQ <= 25 && counter <= 30 && lastPointingOn != -1){
		counter++;
		lastPointingOn = calcPointingOnInBlock(&lastX, &lastY, &lastZ);
		switch (lastPointingOn) {
		//Back
		case 0: blockZ--; lastZ++; break;
		//Front
		case 1: blockZ++; lastZ--; break;
		//Bottom
		case 2: blockY--; lastY++; break;
		//Top
		case 3: blockY++; lastY--; break;
		//Left
		case 4: blockX--; lastX++; break;
		//Right
		case 5: blockX++; lastX--; break;
		}

		double dx = blockX+lastX-posX;
		double dy = blockY+lastY-posY-personSize;
		double dz = blockZ+lastZ-posZ;
		
		distanceQ = dx*dx + dy*dy + dz*dz;
	}
	

	if(landschaft[blockX*ysize*zsize + blockY*zsize  + blockZ] != 0){
		pointingOnX = floor(blockX);
		pointingOnY = floor(blockY);
		pointingOnZ = floor(blockZ);
	}
	else{
		pointingOnX = -1;
		pointingOnY = -1;
		pointingOnZ = -1;
	}
}

void highlightPointingOn(){
	if(pointingOnX+pointingOnY+pointingOnZ != -3){
		glDisable(GL_LIGHT1);
  		glDisable(GL_LIGHTING);
		glEnable(GL_BLEND);
		glBegin(GL_QUADS);
		float x = pointingOnX;
		float y = pointingOnY;
		float z = pointingOnZ;

		switch (lastPointingOn) {
		// Front Face 0
		case 0:
			glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 1 (Front)
			glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 2 (Front)
			glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Front)
			glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 4 (Front)
			break;
		// Back Face 1
		case 1:
			glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Back)
			glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 2 (Back)
			glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 3 (Back)
			glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 4 (Back)
			break;
		// Top Face 2
		case 2:
			glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 1 (Top)
			glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 2 (Top)
			glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Top)
			glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 4 (Top)
			break;
		// Bottom Face 3
		case 3:
			glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Bottom)
			glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 2 (Bottom)
			glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 3 (Bottom)
			glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 4 (Bottom)
			break;
		// Right face 4
		case 4:
			glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 1 (Right)
			glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 2 (Right)
			glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Right)
			glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 4 (Right)
			break;
		// Left Face 5
		case 5:
			glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Left)
			glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 2 (Left)
			glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 3 (Left)
			glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 4 (Left)
			break;
		}
		glEnd();
		glEnable(GL_LIGHT1);
  		glEnable(GL_LIGHTING);
	}
}

//Berechnet die Fläche, auf die von der Startposition aus (Parameter) mit der aktuellen Blickrichtung
//@return: ID der Fläche, auf die man zeigt
//Am Ende sind die Parameter auf den Schnittpunkt gesetzt
int calcPointingOnInBlock(double* startX, double* startY, double* startZ){
	Matrix<double,3,3> left(0);
	Matrix<double,1,3> right(0);
	Matrix<double,1,3> result(0);

	//bleibt immer gleich (Blickrichtung)
	left.data[2][0] = sin(M_PI*x/180.) * cos(M_PI*y/180.);
	left.data[2][1] = -sin(M_PI*y/180.);
	left.data[2][2] = -cos(M_PI*x/180.) * cos(M_PI*y/180.);

	//Fläche 0 (Front)
	left.data[0][0] = 1;
	left.data[1][1] = 1;
	right.data[0][0] = 1-*startX;
	right.data[0][1] = 1-*startY;
	right.data[0][2] = -*startZ;
	result = left.LU().solve(right);
	if( 0 <= result.data[0][0] && result.data[0][0] <= 1
	   && 0 <= result.data[0][1] && result.data[0][1] <= 1
	   && 0 < result.data[0][2]) {
		*startX = 1-result[0][0];
		*startY = 1-result[0][1];
		*startZ = 0;
		return 0;
	}

	//Fläche 1 (Back)
	right.data[0][2] = 1-*startZ;
	result = left.LU().solve(right);
	if( 0 <= result.data[0][0] && result.data[0][0] <= 1
	   && 0 <= result.data[0][1] && result.data[0][1] <= 1
	   && 0 < result.data[0][2]) {
		*startX = 1-result[0][0];
		*startY = 1-result[0][1];
		*startZ = 1;
		return 1;
	}

	//Fläche 2 (Top)
	left.data[1][1] = 0;
	left.data[1][2] = 1;
	right.data[0][1] = -*startY;
	result = left.LU().solve(right);
	if( 0 <= result.data[0][0] && result.data[0][0] <= 1
	   && 0 <= result.data[0][1] && result.data[0][1] <= 1
	   && 0 < result.data[0][2]){
		*startX = 1-result[0][0];
		*startY = 0;
		*startZ = 1-result[0][1];
		return 2;
	}

	//Fläche 3 (Bottom)
	right.data[0][1] = 1-*startY;
	result = left.LU().solve(right);
	if( 0 <= result.data[0][0] && result.data[0][0] <= 1
	   && 0 <= result.data[0][1] && result.data[0][1] <= 1
	   && 0 < result.data[0][2]){
		*startX = 1-result[0][0];
		*startY = 1;
		*startZ = 1-result[0][1];
		return 3;
	}
	
	//Fläche 4 (Right)
	left.data[0][0] = 0;
	left.data[0][1] = 1;
	right.data[0][0] = -*startX;
	result = left.LU().solve(right);
	if( 0 <= result.data[0][0] && result.data[0][0] <= 1
	   && 0 <= result.data[0][1] && result.data[0][1] <= 1
	   && 0 < result.data[0][2]){
		*startX = 0;
		*startY = 1-result[0][0];
		*startZ = 1-result[0][1];
		return 4;
	}

	//Fläche 5 (Left)
	right.data[0][0] = 1-*startX;
	result = left.LU().solve(right);
	if( 0 <= result.data[0][0] && result.data[0][0] <= 1
	   && 0 <= result.data[0][1] && result.data[0][1] <= 1
	   && 0 < result.data[0][2]){
		*startX = 1;
		*startY = 1-result[0][0];
		*startZ = 1-result[0][1];
		return 5;
	}

	//Falls keine Austrittsebene gefunden wird Error
	return -1;
}

Uint32 GameLoopTimer(Uint32 interval, void* param)
{
    // Create a user event to call the game loop.
    SDL_Event event;

    event.type = SDL_USEREVENT;
    event.user.code = RUN_GAME_LOOP;
    event.user.data1 = 0;
    event.user.data2 = 0;

    SDL_PushEvent(&event);

    return interval;
}


void HandleUserEvents(SDL_Event* event)
{
    switch (event->user.code) {
        case RUN_GAME_LOOP:
			draw();
            break;

        default:
            break;
    }
}

void activateTexture(int id){
    glBindTexture( GL_TEXTURE_2D, texture[id] );
}

void loadTexture(const char *texFile, int id) {
    // Load the OpenGL texture

    SDL_Surface *surface; // Gives us the information to make the texture

    if ( (surface = SDL_LoadBMP(texFile)) ) {

        // Check that the image's width is a power of 2
        if ( (surface->w & (surface->w - 1)) != 0 ) {
            printf("warning: %s's width is not a power of 2\n", texFile);
        }

        // Also check if the height is a power of 2
        if ( (surface->h & (surface->h - 1)) != 0 ) {
            printf("warning: %s's height is not a power of 2\n", texFile);
        }

        // Bind the texture object
        glBindTexture( GL_TEXTURE_2D, texture[id] );

        // Set the texture's stretching properties
        //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );


        gluBuild2DMipmaps(GL_TEXTURE_2D, 3, surface->w, surface->h, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
    }
    else {
        printf("SDL could not load %s: %s\n", texFile, SDL_GetError());
        SDL_Quit();
    }

    // Free the SDL_Surface only if it was successfully created
    if ( surface ) {
        SDL_FreeSurface( surface );
    }
}

void gen_gllist() {
	glNewList(box,GL_COMPILE);


  for(int i=1; i < numberOfTex; i++){
    activateTexture(i);
	 	glBegin( GL_QUADS );
	  for(int x=0; x<xsize; x++) for(int y=0; y<ysize; y++) for(int z=0; z<zsize; z++) {
      	if(landschaft[x*ysize*zsize + y*zsize  + z] == i){
			// Front Face 0
		    if(z != zsize-1 && !landschaft[x*ysize*zsize + y*zsize  + z+1]) {
			    glNormal3f( 0.0f, 0.0f, 1.0f);					// Normal Pointing Towards Viewer
			    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 1 (Front)
			    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 2 (Front)
			    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Front)
			    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 4 (Front)
		    }
		    // Back Face 1
		    if(z != 0 && !landschaft[x*ysize*zsize + y*zsize  + z-1]) {
			    glNormal3f( 0.0f, 0.0f,-1.0f);					// Normal Pointing Away From Viewer
			    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Back)
			    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 2 (Back)
			    glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 3 (Back)
			    glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 4 (Back)
		    }
		    // Top Face 2
		    if(y != ysize-1 && !landschaft[x*ysize*zsize + (y+1)*zsize  + z]) {
			    glNormal3f( 0.0f, 1.0f, 0.0f);					// Normal Pointing Up
			    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 1 (Top)
			    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 2 (Top)
			    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Top)
			    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 4 (Top)
		    }
		    // Bottom Face 3
		    if(y != 0 && !landschaft[x*ysize*zsize + (y-1)*zsize  + z]) {
			    glNormal3f( 0.0f,-1.0f, 0.0f);					// Normal Pointing Down
			    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Bottom)
			    glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 2 (Bottom)
			    glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 3 (Bottom)
			    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 4 (Bottom)
		    }
		    // Right face 4
		    if(x != xsize-1 && !landschaft[(x+1)*ysize*zsize + y*zsize  + z]) {
			    glNormal3f( 1.0f, 0.0f, 0.0f);					// Normal Pointing Right
			    glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 1 (Right)
			    glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 2 (Right)
			    glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Right)
			    glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 4 (Right)
		    }
		    // Left Face 5
		    if(x != 0 && !landschaft[(x-1)*ysize*zsize + y*zsize  + z]) {
			    glNormal3f(-1.0f, 0.0f, 0.0f);				// Normal Pointing Left
			    glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Left)
			    glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 2 (Left)
			    glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 3 (Left)
			    glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 4 (Left)
		    }
		  }
    	}
    	glEnd();
	}

	glEndList();
}

//zeichnet die statischen Displayelemente (Anzeigen, ...).
//Sollte als letzte Aktion in draw aufgerufen werden
void drawHUD() {
  glLoadIdentity();
  glDisable(GL_DEPTH_TEST);

  glColor4f(0.0f, 1.0f, 1.0f, 0.5f);
  glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);
  glEnable(GL_BLEND);

  glTranslatef(0.0f,0.0f,-6.0f);
  float lineWidth = 0.012f;
  float lineLength = 0.25f;

  glBegin(GL_QUADS);						// Draw A Quad
		glVertex3f(-lineWidth/2, lineLength/2, 0.0f);				// Top Left
		glVertex3f( lineWidth/2, lineLength/2, 0.0f);				// Top Right
		glVertex3f( lineWidth/2,-lineLength/2, 0.0f);				// Bottom Right
		glVertex3f(-lineWidth/2,-lineLength/2, 0.0f);				// Bottom Left
	glEnd();

	glBegin(GL_QUADS);						// Draw A Quad
		glVertex3f( lineLength/2, -lineWidth/2, 0.0f);				// Top Left
		glVertex3f( lineLength/2,  lineWidth/2, 0.0f);				// Top Right
		glVertex3f(-lineLength/2,  lineWidth/2, 0.0f);				// Bottom Right
		glVertex3f(-lineLength/2, -lineWidth/2, 0.0f);				// Bottom Left
	glEnd();

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
}

void draw() {

	// Clear the screen before drawing
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And The Depth Buffer
	glLoadIdentity();							// Reset The View

	//Mausbewegung
	glRotatef(y,1.0f,0.0f,0.0f);
	glRotatef(x,0.0f,1.0f,0.0f);

	//Eigene Position
	glTranslatef(-posX+0.5f,-posY-personSize+0.5f,-posZ+0.5f);

	activateTexture(2);
	// Landschaft zeichen
	glCallList(box);
	calcPointingOn();
	highlightPointingOn();

	//statische Anzeigen zeichnen
	drawHUD();

	SDL_GL_SwapBuffers();
}

int main() {
	initGL();
	EventLoop();

	return 0;
}

void gen_land() {

	int hoehe[xsize][zsize];

	for(int x=0; x<xsize; x++) for(int z=0; z<zsize; z++) {
		hoehe[x][z] = ysize/2;

		if(x>0 && z>0 && z<zsize-1) {
			hoehe[x][z] = (hoehe[x-1][z-1] + hoehe[x-1][z] + hoehe[x-1][z+1] + rand()%4 - 2) / 3;
			if(hoehe[x][z] < 1) hoehe[x][z] = 1;
			if(hoehe[x][z] > ysize-2) hoehe[x][z] = ysize-2;

		}
	}

	if(landschaft)	delete [] landschaft;
	landschaft = new unsigned char[xsize*ysize*zsize];

	for(int x=0; x<xsize; x++) for(int y=0; y<ysize; y++) for(int z=0; z<zsize; z++) {
		if(y>hoehe[x][z])
			landschaft[x*ysize*zsize + y*zsize  + z] = 0;
		else
			landschaft[x*ysize*zsize + y*zsize  + z] = (rand()%(numberOfTex-1))+1;

	}
}

