#include <iostream>

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <cmath>


using namespace std;


const int RUN_GAME_LOOP = 1;

//GL vars
const int numberOfTex = 4;
GLuint texture[numberOfTex];
GLuint box;

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

float speedOnX = 0.2f;
float speedOnZ = 0.2f;

float speedY = 0.0f;
float accelY = -0.02f;

float fastSpeedMultiplier = 5.0f;

bool xDown = false;
bool xUp = false;
bool zDown = false;
bool zUp = false;
bool jump = false;
bool fastSpeed = false;

//Frame size
int screenX = 1024;
int screenY = 768;


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

// Landschaft
int xsize = 32;
int ysize = 32;
int zsize = 32;
unsigned char *landschaft;


//PROCEDURES

Uint32 GameLoopTimer(Uint32 interval, void* param);
void HandleUserEvents(SDL_Event* event);
void draw();
void sphereSpeed();
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

  SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

  screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_RESIZABLE | SDL_FULLSCREEN);
  if ( !screen ) {
	  printf("Unable to set video mode: %s\n", SDL_GetError());
	}

  // Set the OpenGL state after creating the context with SDL_SetVideoMode

	glEnable(GL_TEXTURE_2D);						// Enable Texture Mapping
	glShadeModel(GL_SMOOTH);						// Enable Smooth Shading
	glClearColor(0.1f, 0.1f, 0.1f, 0.5f);					// Black Background
	glClearDepth(1.0f);							// Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);						// Enables Depth Testing
	glDepthFunc(GL_LEQUAL);							// The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);			// Really Nice Perspective Calculations


	glViewport(0, 0, screenX, screenY);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();							// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)screenX/(GLfloat)screenY,0.01f,1000.0f);

	glMatrixMode(GL_MODELVIEW);						// Select The Modelview Matrix
	glLoadIdentity();							// Reset The Modelview Matrix

  GLfloat LightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f };
  GLfloat LightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f };
  GLfloat LightPosition[]= { 0.5f, 1.0f, 5.0f, 1.0f };

  glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
  glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
  glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);
  glEnable(GL_LIGHT1);
  glEnable(GL_LIGHTING);
  
  enable_rotate = 1;
	SDL_ShowCursor(SDL_DISABLE);
	x_orig = screenX/2;
	y_orig = screenY/2;

  //generate Textures
  glGenTextures( numberOfTex, texture );
  loadTexture("tex/holz.bmp", 1);
  loadTexture("tex/ziegel.bmp", 2);
  loadTexture("tex/gras.bmp", 3);

  activateTexture(2);

  gen_land();
  gen_gllist();


	SDL_TimerID timer = SDL_AddTimer(40,GameLoopTimer,0);

}

void EventLoop(void)
{
    SDL_Event event;

    while((!done) && (SDL_WaitEvent(&event))) {
        switch(event.type) {
            case SDL_USEREVENT:
                HandleUserEvents(&event);
                sphereSpeed();
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
                        fastSpeed = true;
                        break;
                    case k_QUIT:
                        done = true;
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
			    break;
		    case SDLK_MINUS:
			    xsize/=2;
			    zsize/=2;
			    ysize/=2;
			    gen_land();
			    gen_gllist();
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
                    default:
                        //print keycode of unregistered key
                        //cout << event.key.keysym.sym << endl;
                        break;
                }
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
		    SDL_SetVideoMode( event.resize.w, event.resize.h, 32, SDL_OPENGL | SDL_RESIZABLE);
		    glViewport(0, 0, event.resize.w, event.resize.h);						// Reset The Current Viewport

			glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
			glLoadIdentity();							// Reset The Projection Matrix

			// Calculate The Aspect Ratio Of The Window
			gluPerspective(45.0f,(GLfloat)event.resize.w/(GLfloat)event.resize.h,0.1,100.0f);

			glMatrixMode(GL_MODELVIEW);

		    break;


            default:
	//	cout << "default" << endl;
                break;
        }

    }

}

void sphereSpeed()
{
    float mult = 1.0f;
    if (fastSpeed){
      mult = fastSpeedMultiplier;
    }
    float posXold = posX;
    float posZold = posZ;
    
    if (xDown){
        posX -= speedOnX*cos(2*M_PI*x/360)*mult;
        posZ -= speedOnZ*sin(2*M_PI*x/360)*mult;
    }
    if (xUp){
        posX += speedOnX*cos(2*M_PI*x/360)*mult;
        posZ += speedOnZ*sin(2*M_PI*x/360)*mult;
    }
    if (zDown){
        posZ -= speedOnZ*cos(2*M_PI*x/360)*mult;
        posX += speedOnX*sin(2*M_PI*x/360)*mult;
    }
    if (zUp){
        posZ += speedOnZ*cos(2*M_PI*x/360)*mult;
        posX -= speedOnX*sin(2*M_PI*x/360)*mult;
    }
    
    //Collision Detection
    if (   (landschaft[((int)(posX+offset))*ysize*zsize + ((int)posY)*zsize + (int)posZold] != 0 && posX > posXold)
        || (landschaft[((int)(posX+offset))*ysize*zsize + ((int)(posY+1.0f))*zsize + (int)posZold] != 0 && posX > posXold)
        || (landschaft[((int)(posX-offset))*ysize*zsize + ((int)posY)*zsize + (int)posZold] != 0 && posX < posXold) 
        || (landschaft[((int)(posX-offset))*ysize*zsize + ((int)(posY+1.0f))*zsize + (int)posZold] != 0 && posX < posXold))
      posX = posXold;
    if (   (landschaft[((int)posX)*ysize*zsize + ((int)posY)*zsize + (int)(posZ+offset)] != 0 && posZ > posZold)
        || (landschaft[((int)posX)*ysize*zsize + ((int)(posY+1.0f))*zsize + (int)(posZ+offset)] != 0 && posZ > posZold)
        || (landschaft[((int)posX)*ysize*zsize + ((int)posY)*zsize + (int)(posZ-offset)] != 0 && posZ < posZold)
        || (landschaft[((int)posX)*ysize*zsize + ((int)(posY+1.0f))*zsize + (int)(posZ-offset)] != 0 && posZ < posZold))
      posZ = posZold;
      
    //Randbegrenzung
    if(posX<=0) posX = 0;
    if(posX>=xsize-1) posX = xsize-1;
    if(posY<=1) posY = 1;
    if(posY>=ysize-2) posY = ysize-2;
    if(posZ<=0) posZ = 0;
    if(posZ>=zsize-1) posZ = zsize-1;
    
    //Fallen
    if (landschaft[((int)posX)*ysize*zsize + ((int)posY)*zsize + (int)posZ] == 0){
      speedY += accelY;
      if (speedY >= 0.99f)
        speedY = 0.99f;
    }
    posY += speedY;
    if (landschaft[((int)posX)*ysize*zsize + ((int)posY)*zsize + (int)posZ] != 0){
      posY = int(posY)+1;
      speedY = 0;
    }
    
    //Springen
    if (landschaft[((int)posX)*ysize*zsize + ((int)(posY-0.5f))*zsize + (int)posZ] != 0 && jump)
      speedY = 0.2f;
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
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
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
	box=glGenLists(1);
	glNewList(box,GL_COMPILE);

	glBegin( GL_QUADS );

	for(int x=0; x<xsize; x++) for(int y=0; y<ysize; y++) for(int z=0; z<zsize; z++) if(landschaft[x*ysize*zsize + y*zsize  + z]) {

		if(z != zsize-1 && !landschaft[x*ysize*zsize + y*zsize  + z+1]) {
			glNormal3f( 0.0f, 0.0f, 1.0f);					// Normal Pointing Towards Viewer
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 1 (Front)
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 2 (Front)
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Front)
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 4 (Front)
		}
		// Back Face
		if(z != 0 && !landschaft[x*ysize*zsize + y*zsize  + z-1]) {
			glNormal3f( 0.0f, 0.0f,-1.0f);					// Normal Pointing Away From Viewer
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Back)
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 2 (Back)
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 3 (Back)
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 4 (Back)
		}
		// Top Face
		if(y != ysize-1 && !landschaft[x*ysize*zsize + (y+1)*zsize  + z]) {
			glNormal3f( 0.0f, 1.0f, 0.0f);					// Normal Pointing Up
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 1 (Top)
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 2 (Top)
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Top)
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 4 (Top)
		}
		// Bottom Face
		if(y != 0 && !landschaft[x*ysize*zsize + (y-1)*zsize  + z]) {
			glNormal3f( 0.0f,-1.0f, 0.0f);					// Normal Pointing Down
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Bottom)
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 2 (Bottom)
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 3 (Bottom)
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 4 (Bottom)
		}
		// Right face
		if(x != xsize-1 && !landschaft[(x+1)*ysize*zsize + y*zsize  + z]) {
			glNormal3f( 1.0f, 0.0f, 0.0f);					// Normal Pointing Right
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 1 (Right)
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 2 (Right)
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Right)
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 4 (Right)
		}
		// Left Face
		if(x != 0 && !landschaft[(x-1)*ysize*zsize + y*zsize  + z]) {
			glNormal3f(-1.0f, 0.0f, 0.0f);				// Normal Pointing Left
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Left)
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 2 (Left)
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 3 (Left)
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 4 (Left)
		}

	}
	glEnd();
	glEndList();
}


float t = 0;
void draw() {

   // Clear the screen before drawing
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And The Depth Buffer
	glLoadIdentity();							// Reset The View

	t += 1;

	if (t>360) t -= 360;
	if (t<-360) t += 360;

	//Mausbewegung
	glRotatef(y,1.0f,0.0f,0.0f);
	glRotatef(x,0.0f,1.0f,0.0f);

	//Eigene Position
	glTranslatef(-posX+0.5f,-posY-1.0f,-posZ+0.5f);

	activateTexture(2);
	// Landschaft zeichen
	glCallList(box);


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
			landschaft[x*ysize*zsize + y*zsize  + z] = 1;
	}
}
