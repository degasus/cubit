#include <iostream>
#include <time.h>
#include <fstream>

#include "controller.h"




namespace po = boost::program_options;
using namespace std;

Controller::Controller(int argc, char *argv[])
: ui(this), renderer(this), movement(this), map(this)
{
	srand ( time(NULL) );

	parse_command_line(argc, argv);
}

void Controller::run() {
	if(vm["help"].empty()) {
	
		ui.config(vm);
		renderer.config(vm);
		movement.config(vm);
		map.config(vm);

		ui.init();
		renderer.init();
		movement.init();
		map.init();

		ui.run();
	}
}


void Controller::parse_command_line(int argc, char *argv[]) {
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("noFullX", po::value<int>()->default_value(1024), "set the default x-resolution")
		("noFullY", po::value<int>()->default_value(768), "set the default y-resolution")
		("fullscreen", po::value<bool>()->default_value(1), "start in fullscreen")
		("bgColorR", po::value<float>()->default_value(0.6), "Background Color Red")
		("bgColorG", po::value<float>()->default_value(0.7), "Background Color Green")
		("bgColorB", po::value<float>()->default_value(0.8), "Background Color Blue")
		("bgColorA", po::value<float>()->default_value(1.0), "Background Color Aplha")
		("fogDense", po::value<float>()->default_value(0.6), "Densitivity of Fog")
		("fogStartFactor", po::value<float>()->default_value(0.8), "Percental distance to fog start")
		("textureDirectory", po::value<string>()->default_value("/home/arny/.cubit/tex"), "Subdirectory for Textures")
		("texture01", po::value<string>()->default_value("grass.bmp"), "Grass")
		("texture02", po::value<string>()->default_value("wood.bmp"), "Wood")
		("texture03", po::value<string>()->default_value("bricks.bmp"), "Bricks")
		("texture04", po::value<string>()->default_value("alu.bmp"), "Aluminium")
		("texture05", po::value<string>()->default_value("marble.bmp"), "Black marble with white")
		("texture06", po::value<string>()->default_value("hopscotch.bmp"), "Hopscotch")
		("texture07", po::value<string>()->default_value("bee.bmp"), "Black/Yellow")
		("visualRange", po::value<float>()->default_value(25), "maximal distance for rendering")
		("enableFog", po::value<bool>()->default_value(1), "enable Fog")
		("areasPerFrameRendering", po::value<int>()->default_value(5), "set the maximal rendered areas per frame")
		("areasPerFrameLoading", po::value<int>()->default_value(10), "set the maximal from hard disk loaded areas per frame")
		
		("destroyAreaFaktor", po::value<double>()->default_value(2), "distance for destroying areas")

		("storeMaps", po::value<bool>()->default_value(1), "should maps be saved and loaded from harddisk")
	
		("offset", po::value<double>()->default_value(0.3), "offset for horizontal collision detection")
		("offsetAbove", po::value<double>()->default_value(0.1f), "offset above person for vertical collision detection")
		("accelHorizontal", po::value<double>()->default_value(0.04), "accelleration in horizontal diretion")
		("accelVertical", po::value<double>()->default_value(0.016), "accelleration in vertical diretion")
		("personSizeNormal", po::value<double>()->default_value(1.5), "normal size of person (should be between 1.01 and 1.99)")
		("personSizeDucked", po::value<double>()->default_value(1.2), "ducked size of person (should be between 1.01 and 1.99 && < personSizeNormal)")
		("pointingDistance", po::value<int>()->default_value(4), "range to build and remove blocks")
		("slowMovementSpeed", po::value<double>()->default_value(0.05), "speed when moving slowly")
		("normalMovementSpeed", po::value<double>()->default_value(0.1), "speed when moving normally")
		("fastSpeedMultiplier", po::value<double>()->default_value(5.72341), "speed multiplier when moving fast")
		("maxFallingSpeed", po::value<double>()->default_value(-0.99), "fastest reachable speed on falling")
		("turningSpeed", po::value<double>()->default_value(0.2), "speed factor for turning")
		("jumpSpeed", po::value<double>()->default_value(0.187), "initial speed when jumping")
		
		("mapDirectory", po::value<string>()->default_value("/home/arny/.cubit/maps"), "Folder for saving areas")
		
		//UI
		("highlightWholePlane", po::value<bool>()->default_value(1), "highlight the pointing on plane without depth test")
		
		("k_forward", po::value<int>()->default_value(119), "KeyCode for moving forward")
		("k_backwards", po::value<int>()->default_value(115), "KeyCode for moving backwards")
		("k_left", po::value<int>()->default_value(97), "KeyCode for moving left")
		("k_right", po::value<int>()->default_value(100), "KeyCode for moving right")
		("k_moveFast", po::value<int>()->default_value(102), "KeyCode for moving fastly")
		("k_catchMouse", po::value<int>()->default_value(109), "KeyCode for catching mouse")
		("k_jump", po::value<int>()->default_value(32), "KeyCode for jumping")
		("k_fly", po::value<int>()->default_value(60), "KeyCode for enabling/disabling fly")
		("k_duck", po::value<int>()->default_value(304), "KeyCode for ducking")
		("k_quit", po::value<int>()->default_value(27), "KeyCode for exiting")
	;

	//command-line args
	po::store(po::parse_command_line(argc, argv, desc), vm);

	//config file
	std::ifstream i;
	i.open((std::string(std::getenv("HOME")) + "/.cubit/cubit.conf").c_str());
	if (i.is_open()) {
		po::store(po::parse_config_file(i, desc), vm);
	}
	i.close();
	
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
	}
}
