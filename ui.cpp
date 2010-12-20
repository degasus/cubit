#include <iostream>

#include "controller.h"
#include "movement.h"

UInterface::UInterface(Controller *controller)
{
	c = controller;
	done = 0;
	catchMouse = 1;
	for(int i = 0; i < NUMBER_OF_MATERIALS; i++)
		cubeTurn[i] = 120.0/NUMBER_OF_MATERIALS;
}

void UInterface::init()
{
	// Slightly different SDL initialization
	if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0 ) {
		printf("Unable to initialize SDL: %s\n", SDL_GetError());
	}
	
	initWindow();
	
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

}

void UInterface::config(const boost::program_options::variables_map &c)
{
	noFullX 		= c["noFullX"].as<int>();
	
	noFullY 		= c["noFullY"].as<int>();
	isFullscreen 	= c["fullscreen"].as<bool>();

	k_forward		= c["k_forward"].as<int>();
	k_backwards		= c["k_backwards"].as<int>();
	k_left			= c["k_left"].as<int>();
	k_right			= c["k_right"].as<int>();
	k_moveFast		= c["k_moveFast"].as<int>();
	k_catchMouse	= c["k_catchMouse"].as<int>();
	k_jump			= c["k_jump"].as<int>();
	k_duck			= c["k_duck"].as<int>();
	k_fly			= c["k_fly"].as<int>();
	k_quit			= c["k_quit"].as<int>();	

	turningSpeed	= c["turningSpeed"].as<double>();
}


void UInterface::initWindow()
{
	if (isFullscreen) {
		const SDL_VideoInfo *vi = SDL_GetVideoInfo();
		screenX = vi->current_w;
		screenY = vi->current_h;
		screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_FULLSCREEN);
	} else {
		screenX = noFullX;
		screenY = noFullY;
		screen = SDL_SetVideoMode( screenX, screenY, 32, SDL_OPENGL | SDL_RESIZABLE);
	}

	if ( !screen ) {
		printf("Unable to set video mode: %s\n", SDL_GetError());
	}
	
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
	gluPerspective(45.0f, (GLfloat) screenX / (GLfloat) screenY, 0.01f, 1000.0f);
	
	glMatrixMode(GL_MODELVIEW);	// Select The Modelview Matrix
	glLoadIdentity();					// Reset The Projection Matrix

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
	if(code == k_fly){
		ae.name = ActionEvent::PRESS_FLY;
	}
	if(code == k_quit){
		done = 1;
	}
	
	if(ae.name != ActionEvent::NONE)
		c->movement.performAction(ae);
}

void UInterface::handleKeyUpEvents(SDL_KeyboardEvent e)
{
	ActionEvent ae;
	ae.name = ActionEvent::NONE;
	
	int code = (int)e.keysym.sym;
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
	if(code == k_moveFast){
		ae.name = ActionEvent::RELEASE_FAST_SPEED;
	}
	if(code == k_duck){
		ae.name = ActionEvent::RELEASE_DUCK;
	}
	
	if(ae.name != ActionEvent::NONE)
		c->movement.performAction(ae);
}

void UInterface::handleUserEvents(SDL_UserEvent e)
{
	c->movement.triggerNextFrame();
	
	SDL_Event next;	
	//if(SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, SDL_USEREVENT) == 0) {
		PlayerPosition pos = c->movement.getPosition();
		c->map.setPosition(pos);
		c->renderer.render(pos);
		BlockPosition block;
		DIRECTION direct;
		if(c->movement.getPointingOn(&block, &direct))
			c->renderer.highlightBlockDirection(block, direct);
		
		drawHUD();
		
		SDL_GL_SwapBuffers();
	//}
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
			c->movement.performAction(ae);
	}
}

void UInterface::handleMouseUPEvents(SDL_MouseButtonEvent e)
{
	if(catchMouse) {
		ActionEvent ae;
		ae.name = ActionEvent::NONE;
		Material nextMat = 0;
		
		switch(e.button){
			case SDL_BUTTON_RIGHT:
				ae.name = ActionEvent::RELEASE_BUILD_BLOCK;
				break;
			case SDL_BUTTON_LEFT:
				ae.name = ActionEvent::RELEASE_REMOVE_BLOCK;
				break;
			case SDL_BUTTON_WHEELUP:
				ae.name = ActionEvent::SELECT_MATERIAL;
				nextMat = c->movement.getSelectedMaterial();
				if(nextMat >= NUMBER_OF_MATERIALS-1)
					nextMat = 1;
				else
					nextMat++;
				ae.iValue = nextMat;
				break;
			case SDL_BUTTON_WHEELDOWN:
				ae.name = ActionEvent::SELECT_MATERIAL;
				nextMat = c->movement.getSelectedMaterial();
				if(nextMat <= 1)
					nextMat = NUMBER_OF_MATERIALS-1;
				else
					nextMat--;
				ae.iValue = nextMat;
				break;
		}
		
		if(ae.name != ActionEvent::NONE)
			c->movement.performAction(ae);
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
		c->movement.performAction(ae);

		ae.name = ActionEvent::ROTATE_VERTICAL;
		ae.value = -y*turningSpeed;
		c->movement.performAction(ae);
		
		SDL_WarpMouse(screenX/2, screenY/2);
	}
}

void UInterface::drawHUD() {
	glLoadIdentity();
	glClear(GL_DEPTH_BUFFER_BIT);
	glScalef(-1,1,1);
	glRotatef(90.0,0.0f,0.0f,1.0f);
	glRotatef(90.0,0.0f,1.0f,0.0f);

	glColor4f(0.0f, 1.0f, 1.0f, 0.5f);
	glBlendFunc(GL_SRC_COLOR, GL_DST_COLOR);
	glEnable(GL_BLEND);

	glTranslatef(6.0f,0.0f,0.0f);
	float lineWidth = 0.012f;
	float lineLength = 0.25f;

	glBegin(GL_QUADS);						// Draw A Quad
		glVertex3f(0.0f, -lineWidth/2, lineLength/2);				// Top Left
		glVertex3f(0.0f, lineWidth/2, lineLength/2);				// Top Right
		glVertex3f(0.0f,  lineWidth/2, -lineLength/2);				// Bottom Right
		glVertex3f(0.0f, -lineWidth/2, -lineLength/2);				// Bottom Left
	glEnd();

	glBegin(GL_QUADS);						// Draw A Quad
		glVertex3f(0.0f,  lineLength/2, -lineWidth/2);				// Top Left
		glVertex3f(0.0f,  lineLength/2, lineWidth/2);				// Top Right
		glVertex3f(0.0f, -lineLength/2, lineWidth/2);				// Bottom Right
		glVertex3f(0.0f, -lineLength/2, -lineWidth/2);				// Bottom Left
	glEnd();

	glDisable(GL_BLEND);

	glColor4f(0.1f, 0.1f, 0.1f, 0.0f);

	//glTranslatef(6.0f,-7.2f,-3.7f);
	cubeTurn[c->movement.getSelectedMaterial()] += 2;
	if(cubeTurn[c->movement.getSelectedMaterial()] > 360)
		cubeTurn[c->movement.getSelectedMaterial()] -= 360;
	/*glRotatef(cubeTurn, 0.0, 0.0, 1.0);
	glRotatef(35.2644, 0.0, 1.0, 0.0);
	glRotatef(45, 1.0, 0.0, 0.0);
	glTranslatef(-0.5, -0.5, -0.5);*/

	for(int mat = 1; mat < NUMBER_OF_MATERIALS; mat++){
		glLoadIdentity();
		glScalef(-1,1,1);
		glRotatef(90.0,0.0f,0.0f,1.0f);
		glRotatef(90.0,0.0f,1.0f,0.0f);
		
		glTranslatef(24.0f,-14.4f+(mat-1)*2.0,-8.5f);
		glRotatef(cubeTurn[mat], 0.0, 0.0, 1.0);
		glRotatef(35.264389683, 0.0, 1.0, 0.0);
		glRotatef(45, 1.0, 0.0, 0.0);
		glTranslatef(-0.5, -0.5, -0.5);
		
		glBindTexture( GL_TEXTURE_2D, c->renderer.texture[mat] );
		if(c->movement.getSelectedMaterial() == mat){
			glEnable(GL_LIGHT1);
			glEnable(GL_LIGHTING);
		}
		else{
			glDisable(GL_LIGHT1);
			glDisable(GL_LIGHTING);
		}
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
	}

	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHTING);
	glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
}
