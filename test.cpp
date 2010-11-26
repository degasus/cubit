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

float speedOnX = 0.5f;
float speedOnY = 0.5f;
float speedOnZ = 0.5f;

float fastSpeedMultiplier = 10.0f;

bool xDown = false;
bool xUp = false;
bool yDown = false;
bool yUp = false;
bool zDown = false;
bool zUp = false;
bool fastSpeed = false;

//Frame size
int screenX = 1024;
int screenY = 768;


//CONSTANTS

//Keys
const int k_xDown = SDLK_d;
const int k_xUp = SDLK_a;
const int k_yDown = SDLK_e;
const int k_yUp = SDLK_q;
const int k_zDown = SDLK_s;
const int k_zUp = SDLK_w;
const int k_fastSpeed = SDLK_SPACE;
const int k_QUIT = SDLK_ESCAPE;

// Landschaft
int xsize = 64;
int ysize = 16;
int zsize = 64;
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
	gluPerspective(45.0f,(GLfloat)screenX/(GLfloat)screenY,0.1f,1000.0f);

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
                    case k_yUp:
                        yUp = true;
                        break;
                    case k_yDown:
                        yDown = true;
                        break;
                    case k_zUp:
                        zUp = true;
                        break;
                    case k_zDown:
                        zDown = true;
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
			    gen_land();
			    gen_gllist();
			    break;
		    case SDLK_MINUS:
			    xsize/=2;
			    zsize/=2;
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
                    case k_yUp:
                        yUp = false;
                        break;
                    case k_yDown:
                        yDown = false;
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
                    default:
                        //print keycode of unregistered key
                        //cout << event.key.keysym.sym << endl;
                        break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
		        if(event.button.button == SDL_BUTTON_LEFT) {
			        enable_rotate = 1;
			        SDL_ShowCursor(SDL_DISABLE);
			        x_orig = event.motion.x;
			        y_orig = event.motion.y;
		        }
		        if(event.button.button == SDL_BUTTON_RIGHT && enable_rotate == 1) {
			        enable_rotate = 0;
			        SDL_ShowCursor(SDL_ENABLE);
			        SDL_WarpMouse(x_orig, y_orig);
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
    if (xDown){
        posX -= speedOnX*cos(2*M_PI*x/360)*mult;
        posZ -= speedOnZ*sin(2*M_PI*x/360)*mult;
    }
    if (xUp){
        posX += speedOnX*cos(2*M_PI*x/360)*mult;
        posZ += speedOnZ*sin(2*M_PI*x/360)*mult;
    }
    if (yUp){
        posY += speedOnY*mult;
    }
    if (yDown){
        posY -= speedOnY*mult;
    }
    if (zDown){
        posZ -= speedOnZ*cos(2*M_PI*x/360)*mult;
        posX += speedOnX*sin(2*M_PI*x/360)*mult;
    }
    if (zUp){
        posZ += speedOnZ*cos(2*M_PI*x/360)*mult;
        posX -= speedOnX*sin(2*M_PI*x/360)*mult;
    }
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

		if(z == zsize-1 || !landschaft[x*ysize*zsize + y*zsize  + z+1]) {
			glNormal3f( 0.0f, 0.0f, 1.0f);					// Normal Pointing Towards Viewer
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 1 (Front)
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 2 (Front)
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Front)
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 4 (Front)
		}
		// Back Face
		if(z == 0 || !landschaft[x*ysize*zsize + y*zsize  + z-1]) {
			glNormal3f( 0.0f, 0.0f,-1.0f);					// Normal Pointing Away From Viewer
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Back)
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 2 (Back)
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 3 (Back)
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 4 (Back)
		}
		// Top Face
		if(y == ysize-1 || !landschaft[x*ysize*zsize + (y+1)*zsize  + z]) {
			glNormal3f( 0.0f, 1.0f, 0.0f);					// Normal Pointing Up
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-0.5+x,  0.5+y, -0.5+z);	// Point 1 (Top)
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-0.5+x,  0.5+y,  0.5+z);	// Point 2 (Top)
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Top)
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 4 (Top)
		}
		// Bottom Face
		if(y == 0 || !landschaft[x*ysize*zsize + (y-1)*zsize  + z]) {
			glNormal3f( 0.0f,-1.0f, 0.0f);					// Normal Pointing Down
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-0.5+x, -0.5+y, -0.5+z);	// Point 1 (Bottom)
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 2 (Bottom)
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 3 (Bottom)
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-0.5+x, -0.5+y,  0.5+z);	// Point 4 (Bottom)
		}
		// Right face
		if(x == xsize-1 || !landschaft[(x+1)*ysize*zsize + y*zsize  + z]) {
			glNormal3f( 1.0f, 0.0f, 0.0f);					// Normal Pointing Right
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y, -0.5+z);	// Point 1 (Right)
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y, -0.5+z);	// Point 2 (Right)
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5+x,  0.5+y,  0.5+z);	// Point 3 (Right)
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5+x, -0.5+y,  0.5+z);	// Point 4 (Right)
		}
		// Left Face
		if(x == 0 || !landschaft[(x-1)*ysize*zsize + y*zsize  + z]) {
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
	glTranslatef(posX,posY,posZ);

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
			hoehe[x][z] = (hoehe[x-1][z-1] + hoehe[x-1][z] + hoehe[x-1][z+1] + hoehe[x][z-1] + rand()%12 - 4) / 4;
			if(hoehe[x][z] < 1) hoehe[x][z] = 1;
			if(hoehe[x][z] > ysize-1) hoehe[x][z] = ysize-1;

		}
	}

	if(landschaft)	delete [] landschaft;
	landschaft = new unsigned char[xsize*ysize*zsize];

	for(int x=0; x<xsize; x++) for(int y=0; y<ysize; y++) for(int z=0; z<zsize; z++) {
		if(y>hoehe[x][z])
			landschaft[x*ysize*zsize + y*zsize  + z] = 0;
		if(y<hoehe[x][z])
			landschaft[x*ysize*zsize + y*zsize  + z] = 1;
	}
}
