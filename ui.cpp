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
	// Slightly different SDL initialization
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0 ) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}
	if(SDLNet_Init()) {
		printf("Unable to initialize SDL-NET: %s\n", SDLNet_GetError());
	}
	

	
	c = controller;
	done = 0;
	catchMouse = 1;
	for(int i = 0; i < NUMBER_OF_MATERIALS; i++)
		cubeTurn[i] = 120.0/NUMBER_OF_MATERIALS;
	fadingProgress = 0;
	lastMaterial = 1;
	musicPlaying = false;
	lastframe = 0;
}

void UInterface::init()
{

	// load support for the JPG and PNG image formats
	int flags=IMG_INIT_JPG;
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
	font = new FTPolygonFont((dataDirectory/"fonts"/"FreeSans.ttf").string().c_str());
	if(font->Error()) {
		font = new FTPolygonFont((workingDirectory/"fonts"/"FreeSans.ttf").string().c_str());
		if(font->Error()) {
			font = new FTPolygonFont((localDirectory/"fonts"/"FreeSans.ttf").string().c_str());
			if(font->Error()) {
				font = new FTPolygonFont((fs::path("fonts")/"FreeSans.ttf").string().c_str());
				if(font->Error()) {
					printf("Unable to open Font!\n");
				}
			}
		}
	}
	
	// Set the font size
	font->FaceSize(20);

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	if(enableAntiAliasing){
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}

	bool success = initWindow();
	
	if(!success && enableAntiAliasing) {
		enableAntiAliasing = 0;
		std::cout << "Antialiasing not supported, so it will be disabled" << std::endl;
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		
		success = initWindow();
	}
	
	if(!success) {
		std::cout << "Could not create display" << std::endl;
		SDL_Quit();
	}

	int err = glewInit();
	if (GLEW_OK != err) {
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}
	
	//#ifndef _WIN32
	fs::path filename = fs::path("sound") / "music" / "forest.ogg";

	//load music
	if(!((ingameMusic = Mix_LoadMUS((dataDirectory / filename).string().c_str()))||
		(ingameMusic = Mix_LoadMUS((workingDirectory / filename).string().c_str())) ||
		(ingameMusic = Mix_LoadMUS((localDirectory / filename).string().c_str())) ||
		(ingameMusic = Mix_LoadMUS((filename).string().c_str())))
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
	angleOfVision = c["angleOfVision"].as<double>();
	enable3d 		= c["enable3D"].as<bool>();
	
	k_forward		= c["k_forward"].as<int>();
	k_backwards		= c["k_backwards"].as<int>();
	k_left			= c["k_left"].as<int>();
	k_right			= c["k_right"].as<int>();
	k_moveFast		= c["k_moveFast"].as<int>();
	k_catchMouse	= c["k_catchMouse"].as<int>();
	k_jump			= c["k_jump"].as<int>();
	k_duck			= c["k_duck"].as<int>();
	k_throw			= c["k_throw"].as<int>();
	k_fly				= c["k_fly"].as<int>();
	k_lastMat		= c["k_lastMat"].as<int>();
	k_nextMat		= c["k_nextMat"].as<int>();
	k_selMat			= c["k_selMat"].as<int>();
	k_quit			= c["k_quit"].as<int>();
	k_music			= c["k_music"].as<int>();

	turningSpeed	= c["turningSpeed"].as<double>();
}


bool UInterface::initWindow()
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

	SDL_WM_SetCaption("Cubit Alpha 0.0.6","Cubit Alpha 0.0.6");

	if ( !screen ) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
		return 0;
	} else {

		if(catchMouse) {
			SDL_ShowCursor(SDL_DISABLE);
			SDL_WarpMouse(screenX/2, screenY/2);
		} else {
			SDL_ShowCursor(SDL_ENABLE);
		}

		glViewport(0, 0, screenX, screenY);	// Reset The Current Viewport
	}
	return 1;
}

/*
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
*/

void UInterface::run()
{
//	SDL_TimerID timer = SDL_AddTimer(40,GameLoopTimer,0);

	while(!done) {
		redraw();
		
		SDL_Event event;
		while(SDL_PollEvent (&event)) {
			switch(event.type) {
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
}

void UInterface::handleKeyDownEvents(SDL_KeyboardEvent e)
{
	ActionEvent ae;
	ae.name = ActionEvent::NONE;
	int nextMat;

	int code = (int)e.keysym.sym;
	//std::cout << "KeyPressed: " << code << std::endl;

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
	//std::cout << "KeyReleased: " << code << std::endl;

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

void UInterface::redraw()
{
	int start = SDL_GetTicks();
	int time = start-lastframe;
	lastframe = start;
	
	c->movement->triggerNextStep(time);
	c->renderer->time += time;
	PlayerPosition pos = c->movement->getPosition();
	int movement = SDL_GetTicks()-start;
	
	c->map->setPosition(pos);
	int map = SDL_GetTicks()-start-movement;
	
	// Clear the screen before drawing
	SDL_GL_SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);			// Clear The Screen And The Depth Buffer
	
	if(enable3d) {
		glColorMask(1,0,0,0);
		c->renderer->render(pos,0.1);
	} else {
		c->renderer->render(pos,0);
	}
	// Highlighted Block
	BlockPosition block;
	DIRECTION direct;
	bool show_pointing_on = c->movement->getPointingOn(&block, &direct);
	if(show_pointing_on)
		c->renderer->highlightBlockDirection(block, direct);
	
	if(enable3d) {
		glClear(GL_DEPTH_BUFFER_BIT);	
		glColorMask(0,1,1,0);
		c->renderer->render(pos,-0.1);
		if(show_pointing_on)
			c->renderer->highlightBlockDirection(block, direct);
		glColorMask(1,1,1,1);
	}
	
	int renderer = SDL_GetTicks()-start-movement-map;

	//glDisable(GL_DEPTH_TEST);
	//c->movement->dynamicsWorld->debugDrawWorld();
	//glEnable(GL_DEPTH_TEST);

	drawHUD(time);
	int hud = SDL_GetTicks()-start-movement-map-renderer;
	
	stats[0] = std::max<double>(stats[0]*0.99, movement);
	stats[1] = std::max<double>(stats[1]*0.99, map);
	stats[2] = std::max<double>(stats[2]*0.99, renderer);
	stats[3] = std::max<double>(stats[3]*0.99, hud);
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

void UInterface::drawHUD(int time) {
	
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

	cubeTurn[c->movement->getSelectedMaterial()] += (10/1000.)*time;
	if(cubeTurn[c->movement->getSelectedMaterial()] > 360)
		cubeTurn[c->movement->getSelectedMaterial()] -= 360;

	int selectedMaterial = c->movement->getSelectedMaterial();

	if(fadingProgress>0)
		fadingProgress -= (150/1000.)*time;
	else if(fadingProgress < 0)
		fadingProgress += (150/1000.)*time;

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
	
	glUseProgram(c->renderer->shader.solid_po);
	glUniform1f(c->renderer->shader.visualRange, 0);
	
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

		glBegin(GL_QUADS);
			for(int dir=0; dir < DIRECTION_COUNT; dir++) {
				for(int point=0; point < POINTS_PER_POLYGON; point++) {
					glVertexAttrib4f( c->renderer->shader.tPos,
						TEXTUR_POSITION_OF_DIRECTION[dir][point][0],
						TEXTUR_POSITION_OF_DIRECTION[dir][point][1],
						mat,
						dir
					);
					glVertexAttrib4f( c->renderer->shader.bPos,
						(POINTS_OF_DIRECTION[dir][point][0]),
						(POINTS_OF_DIRECTION[dir][point][1]),
						(POINTS_OF_DIRECTION[dir][point][2]),
						1.0
					);
				}
			}
		glEnd();
		
		glPopMatrix();
		if(!sandboxMode){
			glDisable(GL_LIGHT3);
			glEnable(GL_LIGHT2);
			glScalef(1.0/cubeSize,1.0/cubeSize,1.0/cubeSize);
			glColor4f(0.0f, 0.0f, 0.0f, 0.9f);
			renderText(-cubeSize*0.8,-cubeSize*0.8, boost::lexical_cast<std::string>( c->movement->getCountInInventory(mat) ).c_str() );
			startMatSBMode = c->movement->getNextAvailableMaterial(startMatSBMode);
		}
	}
	
	glUniform1f(c->renderer->shader.visualRange, visualRange*AREASIZE_X);
	glUseProgram(0);

	///////////////////////////////////
	//print some Text
	///////////////////////////////////
	
	//a += 0.1f;
	
	if(sandboxMode) {
		
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
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_LIGHTING);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0,1.0,1.0,0.5);
		
		glBegin(GL_QUADS);
			glVertex3f(0.0, screenY-0.0, 0.0);
			glVertex3f(0.0, screenY-120, 0.0);
			glVertex3f(screenX, screenY-120, 0.0);
			glVertex3f(screenX, screenY-0.0, 0.0);
			
		glEnd();
		glColor4f(0.0,0.0,0.0,1.0);
		
		int progress = c->movement->getCurrentRemoveProgress();
		if(progress > 0){
			std::string output = boost::lexical_cast<std::string>(progress) + "%";
			renderText(20, 20, output.c_str());
		}
		
		glTranslatef(0.0f, screenY, 0.0f);
		glScalef(0.7f, 0.7f,1.0f);
		
		debug_time += time;
		if(debug_time > 1000) {
			debug_time = 0;
			maps_debug = c->map->debug_msg();
		}

		renderText(20, -30, c->movement->debug_msg().c_str());
		renderText(20, -60, maps_debug.c_str());
		renderText(20, -90, c->renderer->debug_output[0].c_str());
		renderText(20, -120, c->renderer->debug_output[1].c_str());
		
		std::ostringstream out(std::ostringstream::out);
		out << "Movement: " << int(stats[0]) << ", Map: " << int(stats[1]) << ", Renderer: " << int(stats[2]) << ", HUD: " << int(stats[3]);
		renderText(20,-150, out.str().c_str());
	}
	
	/////////////////////////////
	//reset the view
	////////////////////////////
	glDisable(GL_BLEND);	
	glEnable(GL_DEPTH_TEST);
}
