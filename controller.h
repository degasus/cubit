#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

class Controller;

#include <boost/program_options.hpp>

#include "ui.h"
#include "renderer.h"
#include "movement.h"
#include "map.h"


/**
 * Controllerklasse des Spieles Openminecraft
 * Aufgaben:
 *  - Initialisierung aller Module
 *  -
 */
class Controller {
public:
	/**
	 * Startet das Spiel und initialisiert alles
	 */
	Controller(int argc, char *argv[]);

	void quit();

	void run();


private:
//	UInterface ui;
//	Renderer renderer;
//	Movement movement;
//	Map map;
	
	void parse_command_line(int argc, char *argv[]);
	
	boost::program_options::variables_map vm;

};





#endif