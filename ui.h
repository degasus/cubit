#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>


#ifndef _UI_H_
#define _UI_H_

class UInterface;


#include "renderer.h"
#include "map.h"
#include <FTGL/ftgl.h>

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

	void show_message(std::string msg);
	
	//Text
	FTFont *font;
	//FTTextureFont *font;
	void renderText(double x, double y, const char* text);
	
	//Frame conditions
	//current frame size
	int screenX;
	int screenY;

private:
	//Meta
	bool sandboxMode;
	
	bool initWindow();
	
	void redraw();
	
	void handleKeyDownEvents(SDL_KeyboardEvent e);
	void handleKeyUpEvents(SDL_KeyboardEvent e);
	void handleMouseDownEvents(SDL_MouseButtonEvent e);
	void handleMouseUPEvents(SDL_MouseButtonEvent e);
	void handleMouseEvents(SDL_MouseMotionEvent e);
	void drawHUD(int time);

	boost::filesystem::path workingDirectory;
	boost::filesystem::path dataDirectory;
	boost::filesystem::path localDirectory;
	
	int visualRange;
	double angleOfVision;
	
	//default frame size on no fullscreen
	int noFullX;
	int noFullY;
	//fullscreen on/off
	bool isFullscreen;

	//multi sampling
	bool enableAntiAliasing;
	
	bool enable3d;
	
	bool catchMouse;
	double turningSpeed;

	//SDL vars
	SDL_Surface *screen;
	bool done;
	int lastframe;


	
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
	int k_throw;
	int k_lastMat;
	int k_nextMat;
	int k_selMat;
	int k_quit;
	int k_catchMouse;
	int k_music;
	int k_toggle_walk;
	int k_toggle_debug;
	bool f_key_pressed;
	bool debugBar;

	//HUD
	double cubeTurn[NUMBER_OF_MATERIALS];
	int fadingProgress;
	int lastMaterial;
	
	//Music
	Mix_Music *ingameMusic;
	bool musicPlaying;

	//Message
	int cur_msg_timeout;
	int msg_display_time;
	std::string msg;
	
	float stats[4];
	std::string maps_debug;
	int debug_time;
};


#endif
