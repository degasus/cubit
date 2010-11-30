#ifndef _CONTROLLER_H_
#define _CONTROLLER_H_

class Controller;

#include "ui.h"
#include "config.h"
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
	Controller();

	void quit();


private:
	UInterface* ui;
	Config* config;
	Renderer* renderer;
	Movement* movement;
	Map* map;

};





#endif