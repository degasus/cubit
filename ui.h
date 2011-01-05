#ifndef _UI_H_
#define _UI_H_

class UInterface;


#include <boost/program_options.hpp>
#include "renderer.h"
#include "map.h"

/**
 *
 *
 */
class UInterface {
public:
	/**
	 *
	 *
	 */
	UInterface(Controller *controller);

	/**
	 *
	 */
	void init();

	void config(const boost::program_options::variables_map &c);
	
	void run();
	
	//Frame conditions
	//current frame size
	int screenX;
	int screenY;

private:
	void initWindow();
	
	void handleUserEvents(SDL_UserEvent e);
	void handleKeyDownEvents(SDL_KeyboardEvent e);
	void handleKeyUpEvents(SDL_KeyboardEvent e);
	void handleMouseDownEvents(SDL_MouseButtonEvent e);
	void handleMouseUPEvents(SDL_MouseButtonEvent e);
	void handleMouseEvents(SDL_MouseMotionEvent e);
	void drawHUD();

	std::string workingDirectory;

	//default frame size on no fullscreen
	int noFullX;
	int noFullY;
	//fullscreen on/off
	bool isFullscreen;

	//multi sampling
	bool enableAntiAliasing;
	
	bool catchMouse;
	double turningSpeed;

	//SDL vars
	SDL_Surface *screen;
	bool done;
	
	Controller *c;

	//Keys
	int k_forward;
	int k_backwards;
	int k_left;
	int k_right;
	int k_jump;
	int k_duck;
	int k_moveFast;
	int k_Duck;
	int k_fly;
	int k_quit;
	int k_catchMouse;
	int k_music;

	//HUD
	double cubeTurn[NUMBER_OF_MATERIALS];
	int fadingProgress;
	int lastMaterial;
	
	//Music
	Mix_Music *ingameMusic;
	bool musicPlaying;
};


#endif
