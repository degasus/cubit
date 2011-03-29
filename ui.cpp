#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

#include <SDL_mixer.h>
#include <SDL_image.h>
#include <FTGL/ftgl.h>

#include "controller.h"
#include "movement.h"
#include "ui.h"


namespace fs = boost::filesystem;

UInterface::UInterface(Controller *controller)
{
	c = controller;
	done = 0;
	catchMouse = 1;
	for(int i = 0; i < NUMBER_OF_MATERIALS; i++)
		cubeTurn[i] = 120.0/NUMBER_OF_MATERIALS;
	fadingProgress = 0;
	lastMaterial = 1;
	musicPlaying = false;
}

void UInterface::init()
{
	// Slightly different SDL initialization
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0 ) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}

	// load support for the JPG and PNG image formats
	int flags=IMG_INIT_JPG|IMG_INIT_PNG;
	int initted=IMG_Init(flags);
	if((initted & flags) != flags) {
		printf("IMG_Init: Failed to init required jpg and png support!\n");
		printf("IMG_Init: %s\n", IMG_GetError());
		// handle error
	}

	/* We're going to be requesting certain things from our audio
	device, so we set them up beforehand */
	int audio_rate = 44100;
	Uint16 audio_format = AUDIO_S16; /* 16-bit stereo */
	int audio_channels = 2;
	int audio_buffers = 4096;

	/* This is where we open up our audio device.  Mix_OpenAudio takes
	as its parameters the audio format we'd /like/ to have. */
	if(Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
		printf("Unable to open audio!\n");
	}

	// Create a pixmap font from a TrueType file.
	font = new FTTextureFont("freefont-ttf/sfd/FreeSans.ttf");

	// If something went wrong, bail out.
	if(font->Error())
		printf("Unable to open Font!\n");
	
	// Set the font size
	font->FaceSize(20);

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	if(enableAntiAliasing){
		std::cout << SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1) << std::endl;
		std::cout << SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4) << std::endl;
	}

	initWindow();
//#ifndef _WIN32
	fs::path filename = fs::path("sound") / "music" / "forest.ogg";

	//load music
	if(!((ingameMusic = Mix_LoadMUS((dataDirectory / filename).file_string().c_str()))||
		(ingameMusic = Mix_LoadMUS((workingDirectory / filename).file_string().c_str())) ||
		(ingameMusic = Mix_LoadMUS((localDirectory / filename).file_string().c_str())) ||
		(ingameMusic = Mix_LoadMUS((filename).file_string().c_str())))
	) {
		std::cout << "Could not find the music file " << filename << " Error: " << Mix_GetError() <<  std::endl;
	}
//#endif
}

void UInterface::config(const boost::program_options::variables_map &c)
{
	sandboxMode	= c["sandboxMode"].as<bool>();
	
	noFullX 		= c["noFullX"].as<int>();
	noFullY 		= c["noFullY"].as<int>();
	isFullscreen 	= c["fullscreen"].as<bool>();
	if(!isFullscreen) {
		screenX = noFullX;
		screenY = noFullY;
	}
	enableAntiAliasing = c["enableAntiAliasing"].as<bool>();

	workingDirectory = c["workingDirectory"].as<fs::path>();
	dataDirectory = c["dataDirectory"].as<fs::path>();
	localDirectory = c["localDirectory"].as<fs::path>();
	
	visualRange = c["visualRange"].as<int>();

	k_forward		= c["k_forward"].as<int>();
	k_backwards		= c["k_backwards"].as<int>();
	k_left			= c["k_left"].as<int>();
	k_right			= c["k_right"].as<int>();
	k_moveFast		= c["k_moveFast"].as<int>();
	k_catchMouse	= c["k_catchMouse"].as<int>();
	k_jump			= c["k_jump"].as<int>();
	k_duck			= c["k_duck"].as<int>();
	k_throw			= c["k_throw"].as<int>();
	k_fly			= c["k_fly"].as<int>();
	k_lastMat		= c["k_lastMat"].as<int>();
	k_nextMat		= c["k_nextMat"].as<int>();
	k_selMat		= c["k_selMat"].as<int>();
	k_quit			= c["k_quit"].as<int>();
	k_music			= c["k_music"].as<int>();

	turningSpeed	= c["turningSpeed"].as<double>();
}


void UInterface::initWindow()
{
	if (isFullscreen) {
		const SDL_VideoInfo *vi = SDL_GetVideoInfo();
		screenX = vi->current_w;
		screenY = vi->current_h;
		screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_FULLSCREEN );
	} else {
		noFullX = screenX;
		noFullY = screenY;
		screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_RESIZABLE );
	}

	SDL_WM_SetCaption("Cubit Alpha 0.0.3","Cubit Alpha 0.0.3");

	if ( !screen ) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
		SDL_Quit();
	} else {

		if(catchMouse) {
			SDL_ShowCursor(SDL_DISABLE);
			SDL_WarpMouse(screenX/2, screenY/2);
		} else {
			SDL_ShowCursor(SDL_ENABLE);
		}

		glViewport(0, 0, screenX, screenY);	// Reset The Current Viewport
		glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
		glLoadIdentity();					// Reset The Projection Matrix

		// Calculate The Aspect Ratio Of The Window
		gluPerspective(45.0f, (GLfloat) screenX / (GLfloat) screenY, 0.01f, visualRange * AREASIZE_X);

		glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
		glLoadIdentity();					// Reset The Projection Matrix
	}
}

Uint32 GameLoopTimer(Uint32 interval, void* param)
{
		// Create a user event to call the game loop.
		SDL_Event event;

		event.type = SDL_USEREVENT;
		event.user.code = 0;
		event.user.data1 = 0;
		event.user.data2 = 0;

		SDL_PushEvent(&event);

		return interval;
}


void UInterface::run()
{
	SDL_TimerID timer = SDL_AddTimer(40,GameLoopTimer,0);

	SDL_Event event;

	while((!done) && (SDL_WaitEvent(&event))) {
		switch(event.type) {
			case SDL_USEREVENT:			handleUserEvents(event.user);			break;
			case SDL_KEYDOWN:			handleKeyDownEvents(event.key); 		break;
			case SDL_KEYUP:				handleKeyUpEvents(event.key); 			break;
			case SDL_MOUSEBUTTONDOWN: 	handleMouseDownEvents(event.button); 	break;
			case SDL_MOUSEBUTTONUP: 	handleMouseUPEvents(event.button); 		break;
			case SDL_MOUSEMOTION: 		handleMouseEvents(event.motion); 		break;
			case SDL_QUIT: 				done = true;  							break;

			case SDL_VIDEORESIZE:
				std::cout << "video-resize-event" << std::endl;
				screenX = event.resize.w;
				screenY = event.resize.h;
				initWindow();
				break;
		}
	}
}

void UInterface::handleKeyDownEvents(SDL_KeyboardEvent e)
{
	ActionEvent ae;
	ae.name = ActionEvent::NONE;
	int nextMat;

	int code = (int)e.keysym.sym;
	std::cout << "KeyPressed: " << code << std::endl;

	if(code == k_forward){
		ae.name = ActionEvent::PRESS_FORWARD;
	}
	if(code == k_backwards){
		ae.name = ActionEvent::PRESS_BACKWARDS;
	}
	if(code == k_left){
		ae.name = ActionEvent::PRESS_LEFT;
	}
	if(code == k_right){
		ae.name = ActionEvent::PRESS_RIGHT;
	}
	if(code == k_moveFast){
		ae.name = ActionEvent::PRESS_FAST_SPEED;
	}
	if(code == k_catchMouse){
		SDL_WarpMouse(screenX/2, screenY/2);
		catchMouse = !catchMouse;
		if(catchMouse) {
			SDL_ShowCursor(SDL_DISABLE);
		} else {
			SDL_ShowCursor(SDL_ENABLE);
		}
	}
	if(code == k_jump){
		ae.name = ActionEvent::PRESS_JUMP;
	}
	if(code == k_duck){
		ae.name = ActionEvent::PRESS_DUCK;
	}
	if(code == k_throw){
		ae.name = ActionEvent::PRESS_THROW_BLOCK;
	}
	if(code == k_fly){
		ae.name = ActionEvent::PRESS_FLY;
	}
	if(code == k_nextMat){
		ae.name = ActionEvent::SELECT_MATERIAL;
		ae.iValue = c->movement->getNextAvailableMaterial(c->movement->getSelectedMaterial());
	}
	if(code == k_lastMat){
		ae.name = ActionEvent::SELECT_MATERIAL;
		ae.iValue = c->movement->getLastAvailableMaterial(c->movement->getSelectedMaterial());
	}
	if(code == k_selMat){
		ae.name = ActionEvent::SELECT_MATERIAL;
		BlockPosition bp;
		DIRECTION dir;
		if(c->movement->getPointingOn(&bp, &dir))
			ae.iValue = c->map->getBlock(bp+dir);
		else
			ae.name = ActionEvent::NONE;
	}
	if(code == k_quit){
		done = 1;
	}

	if(ae.name != ActionEvent::NONE)
		c->movement->performAction(ae);
}

void UInterface::handleKeyUpEvents(SDL_KeyboardEvent e)
{
	ActionEvent ae;
	ae.name = ActionEvent::NONE;

	int code = (int)e.keysym.sym;
	std::cout << "KeyReleased: " << code << std::endl;

	if(code == k_forward){
		ae.name = ActionEvent::RELEASE_FORWARD;
	}
	if(code == k_backwards){
		ae.name = ActionEvent::RELEASE_BACKWARDS;
	}
	if(code == k_left){
		ae.name = ActionEvent::RELEASE_LEFT;
	}
	if(code == k_right){
		ae.name = ActionEvent::RELEASE_RIGHT;
	}
	if(code == k_jump){
		ae.name = ActionEvent::RELEASE_JUMP;
	}
	if(code == k_throw){
		ae.name = ActionEvent::RELEASE_THROW_BLOCK;
	}
	if(code == k_moveFast){
		ae.name = ActionEvent::RELEASE_FAST_SPEED;
	}
	if(code == k_duck){
		ae.name = ActionEvent::RELEASE_DUCK;
	}
//#ifndef _WIN32
	if(code == k_music){
		if(!musicPlaying) {
			Mix_PlayMusic(ingameMusic, -1);
			musicPlaying = true;
		}
		else {
			Mix_HaltMusic();
			musicPlaying = false;
		}
	}
//#endif
	if(ae.name != ActionEvent::NONE)
		c->movement->performAction(ae);
}

void UInterface::renderText(double x, double y, const char* Text)
{
	font->Render(Text, -1, FTPoint(FTGL_DOUBLE(x), FTGL_DOUBLE(y)));
}

void UInterface::handleUserEvents(SDL_UserEvent e)
{
	c->movement->triggerNextFrame();

	SDL_Event next;
	SDL_PumpEvents();
	if(!SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, ~SDL_USEREVENT)) {
		PlayerPosition pos = c->movement->getPosition();
		c->map->setPosition(pos);
		c->renderer->render(pos);
		BlockPosition block;
		DIRECTION direct;
		if(c->movement->getPointingOn(&block, &direct))
			c->renderer->highlightBlockDirection(block, direct);

		//glDisable(GL_DEPTH_TEST);
		c->movement->dynamicsWorld->debugDrawWorld();
		//glEnable(GL_DEPTH_TEST);

		drawHUD();

		SDL_GL_SwapBuffers();
	}
}

void UInterface::handleMouseDownEvents(SDL_MouseButtonEvent e)
{
	if(catchMouse) {
		ActionEvent ae;
		ae.name = ActionEvent::NONE;

		switch(e.button){
			case SDL_BUTTON_RIGHT:
				ae.name = ActionEvent::PRESS_BUILD_BLOCK;
				break;
			case SDL_BUTTON_LEFT:
				ae.name = ActionEvent::PRESS_REMOVE_BLOCK;
				break;
		}

		if(ae.name != ActionEvent::NONE)
			c->movement->performAction(ae);
	}
}

void UInterface::handleMouseUPEvents(SDL_MouseButtonEvent e)
{
	if(catchMouse) {
		ActionEvent ae;
		ae.name = ActionEvent::NONE;
		int nextMat = 0;

		switch(e.button){
			case SDL_BUTTON_RIGHT:
				ae.name = ActionEvent::RELEASE_BUILD_BLOCK;
				break;
			case SDL_BUTTON_LEFT:
				ae.name = ActionEvent::RELEASE_REMOVE_BLOCK;
				break;
			case SDL_BUTTON_WHEELUP:
				ae.name = ActionEvent::SELECT_MATERIAL;
				ae.iValue = c->movement->getNextAvailableMaterial(c->movement->getSelectedMaterial());
				break;
			case SDL_BUTTON_WHEELDOWN:
				ae.name = ActionEvent::SELECT_MATERIAL;
				ae.iValue = c->movement->getLastAvailableMaterial(c->movement->getSelectedMaterial());
				break;
		}

		if(ae.name != ActionEvent::NONE)
			c->movement->performAction(ae);
	}
}

void UInterface::handleMouseEvents(SDL_MouseMotionEvent e)
{
	if(catchMouse) {
		int x = e.x-screenX/2;
		int y = e.y-screenY/2;

		ActionEvent ae;
		ae.name = ActionEvent::ROTATE_HORIZONTAL;
		ae.value = x*turningSpeed;
		c->movement->performAction(ae);

		ae.name = ActionEvent::ROTATE_VERTICAL;
		ae.value = -y*turningSpeed;
		c->movement->performAction(ae);

		SDL_WarpMouse(screenX/2, screenY/2);
	}
}

//float a = -10.0;

void UInterface::drawHUD() {
	int cubeSize = 50;
	
	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix,
	glTranslatef(-1,-1,-1);
	glScalef(2.0/screenX, 2.0/screenY, 0);
	
	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	
	
	//Lighting fou
	GLfloat LightPosition[] = { screenX, screenY, 100, 1.0f };
	glLightfv(GL_LIGHT2, GL_POSITION, LightPosition);
	GLfloat LightAmbient[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glLightfv(GL_LIGHT2, GL_AMBIENT, LightAmbient);
	
	glEnable(GL_LIGHTING);
	glDisable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	
	glLightfv(GL_LIGHT3, GL_POSITION, LightPosition);
	GLfloat LightAmbient3[] = { 0.4f, 0.4f, 0.4f, 1.0f };
	glLightfv(GL_LIGHT3, GL_AMBIENT, LightAmbient3);
	GLfloat LightDiffuse[]  = { 0.4f, 0.4f, 0.4f, 1.0f };	
	glLightfv(GL_LIGHT3, GL_DIFFUSE,  LightDiffuse);
	glDisable(GL_LIGHT3);
	
	
	glDisable(GL_FOG);
	glClear(GL_DEPTH_BUFFER_BIT);
	glBindTexture( GL_TEXTURE_2D, c->renderer->texture[1] );

	glColor4f(0.5f, 0.5f, 0.5f, 0.5f);
	glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);
	glEnable(GL_BLEND);
	
	int lineWidth = 3;
	int lineLength = 50;

	glBegin(GL_QUADS);						// Draw A Quad
		glVertex3f( lineWidth/2+screenX/2, -lineLength/2+screenY/2, 0.0f);				// Top Left
		glVertex3f( lineWidth/2+screenX/2,  lineLength/2+screenY/2, 0.0f);				// Top Right
		glVertex3f(-lineWidth/2+screenX/2,  lineLength/2+screenY/2, 0.0f);				// Bottom Right
		glVertex3f(-lineWidth/2+screenX/2, -lineLength/2+screenY/2, 0.0f);				// Bottom Left
	glEnd();

	glBegin(GL_QUADS);						// Draw A Quad
		glVertex3f( lineLength/2+screenX/2, -lineWidth/2+screenY/2, 0.0f);				// Top Left
		glVertex3f( lineLength/2+screenX/2,  lineWidth/2+screenY/2, 0.0f);				// Top Right
		glVertex3f(-lineLength/2+screenX/2,  lineWidth/2+screenY/2, 0.0f);				// Bottom Right
		glVertex3f(-lineLength/2+screenX/2, -lineWidth/2+screenY/2, 0.0f);				// Bottom Left
	glEnd();
	
	glDisable(GL_BLEND);

	cubeTurn[c->movement->getSelectedMaterial()] += 2;
	if(cubeTurn[c->movement->getSelectedMaterial()] > 360)
		cubeTurn[c->movement->getSelectedMaterial()] -= 360;

	int selectedMaterial = c->movement->getSelectedMaterial();

	if(fadingProgress>0)
		fadingProgress -= 10;
	else if(fadingProgress < 0)
		fadingProgress += 10;

	if((lastMaterial == c->movement->getLastAvailableMaterial(selectedMaterial))){
		fadingProgress = 90;
	}
	else if((lastMaterial == c->movement->getNextAvailableMaterial(selectedMaterial))){
		fadingProgress = -90;
	}
	lastMaterial = selectedMaterial;

	double fading = 0;
	int numberOfHUDcubes = 7;
	if(fadingProgress != 0){
		numberOfHUDcubes += 1;
	}

	fading = sin(fadingProgress*M_PI/180);
	
	int startMatSBMode = selectedMaterial;
	
	if(!sandboxMode){
		for(int i = 0; i < numberOfHUDcubes/2; i++){
			startMatSBMode = c->movement->getLastAvailableMaterial(startMatSBMode);
		}
	}
	
	///////////////////////////
	// render the HUD cubes
	///////////////////////////
	for(int pos = -(numberOfHUDcubes/2); pos <= numberOfHUDcubes/2; pos++){
		int mat = 1;
		if(sandboxMode){
			mat = selectedMaterial+pos;
			while(mat < 1)
				mat = NUMBER_OF_MATERIALS + mat - 1;
			while(mat > (NUMBER_OF_MATERIALS - 1))
				mat = mat - NUMBER_OF_MATERIALS + 1;
		}
		else{			
			mat = startMatSBMode;
		}
		
		if(mat == selectedMaterial){
			glDisable(GL_LIGHT3);
			glEnable(GL_LIGHT2);
		}
		else{
			glDisable(GL_LIGHT2);
			glEnable(GL_LIGHT3);
		}
		
		float curCubeSize = cubeSize*(std::sqrt((-(pos+fading)*(pos+fading)+17)/17.0));
		glLoadIdentity();
		glTranslatef((screenX/2) + (pos+fading) * cubeSize * 2,curCubeSize + cubeSize*0.1,0.0);
		glScalef(curCubeSize,curCubeSize,-0.05f);
		glPushMatrix();
		
		glRotatef(cubeTurn[mat], 0.0, 1.0, 0.0);
		glRotatef(35.264389683, 1.0, 0.0, 0.0);
		glRotatef(45, 0.0, 0.0, 1.0);
		glTranslatef(-0.5, -0.5, -0.5);

		glBindTexture( GL_TEXTURE_2D, c->renderer->texture[mat] );
		glBegin(GL_QUADS);
			for(int dir=0; dir < DIRECTION_COUNT; dir++) {
				glNormal3f( NORMAL_OF_DIRECTION[dir][0], NORMAL_OF_DIRECTION[dir][1], NORMAL_OF_DIRECTION[dir][2]);					// Normal Pointing Towards Viewer
				for(int point=0; point < POINTS_PER_POLYGON; point++) {
					glTexCoord2f(
						TEXTUR_POSITION_OF_DIRECTION[dir][point][0],
						TEXTUR_POSITION_OF_DIRECTION[dir][point][1]
					);
					glVertex3f(
								POINTS_OF_DIRECTION[dir][point][0],
								  POINTS_OF_DIRECTION[dir][point][1],
								  POINTS_OF_DIRECTION[dir][point][2]
					);
				}
			}
		glEnd();
		
		if(!sandboxMode){
			glDisable(GL_LIGHT3);
			glEnable(GL_LIGHT2);
			glPopMatrix();
			glScalef(1.0/cubeSize,1.0/cubeSize,1.0/cubeSize);
			glColor4f(0.0f, 0.0f, 0.0f, 0.9f);
			renderText(-cubeSize*0.8,-cubeSize*0.8, boost::lexical_cast<std::string>( c->movement->getCountInInventory(mat) ).c_str() );
			startMatSBMode = c->movement->getNextAvailableMaterial(startMatSBMode);
		}
	}

	///////////////////////////////////
	//print some Text
	///////////////////////////////////
	
	//a += 0.1f;
	
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHT2);
	glDisable(GL_LIGHT3);
	glEnable(GL_LIGHT4);
	
	GLfloat LightPosition4[] = { screenX/2, screenY/2, 1000, 1.0f };
	glLightfv(GL_LIGHT4, GL_POSITION, LightPosition4);
	GLfloat LightAmbient4[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	glLightfv(GL_LIGHT4, GL_AMBIENT, LightAmbient4);
	GLfloat LightDiffuse4[]  = { 0.5f, 0.5f, 0.5f, 1.0f };	
	glLightfv(GL_LIGHT4, GL_DIFFUSE,  LightDiffuse4);
	
	
	glLoadIdentity();
	
	renderText(20, screenY-40, c->movement->getPosition().to_string().c_str());
	//renderText(20, screenY-80, boost::lexical_cast<std::string>(a).c_str());
	
	int progress = c->movement->getCurrentRemoveProgress();
	if(progress > 0){
		std::string output = boost::lexical_cast<std::string>(progress) + "%";
		renderText(20, 20, output.c_str());
	}
	
	/////////////////////////////
	//reset the view
	////////////////////////////
	glDisable(GL_BLEND);	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	glDisable(GL_LIGHT3);
	glDisable(GL_LIGHT4);
	
	glColor4f(1.0f, 1.0f, 1.0f, 0.0f);

	glDisable(GL_FOG);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);		// Select The Projection Matrix
	glLoadIdentity();					// Reset The Projection Matrix
	
	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat) screenX / (GLfloat) screenY, 0.01f, visualRange * AREASIZE_X);
	
	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	glLoadIdentity();					// Reset The Projection Matrix
}
