#include <iostream>

#include "controller.h"



namespace po = boost::program_options;
using namespace std;

Controller::Controller(int argc, char *argv[]) {
	parse_command_line(argc, argv);
}

void Controller::run() {
	ui.config(vm);
	renderer.config(vm);
	movement.config(vm);
	ui.init();
	renderer.init();
	movement.init();
}


void Controller::parse_command_line(int argc, char *argv[]) {
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("noFullX", po::value<int>()->default_value(640), "set the default x-resolution")
		("noFullY", po::value<int>()->default_value(480), "set the default y-resolution")
		("fullscreen", po::value<bool>()->default_value(1), "start in fullscreen")
		("bgColorR", po::value<float>()->default_value(0.6), "Background Color Red")
		("bgColorG", po::value<float>()->default_value(0.7), "Background Color Green")
		("bgColorB", po::value<float>()->default_value(0.8), "Background Color Blue")
		("bgColorA", po::value<float>()->default_value(1.0), "Background Color Aplha")
		("fogDense", po::value<float>()->default_value(0.6), "Densitivity of Fog")
		("fogStartFactor", po::value<float>()->default_value(0.4), "Percental distance to fog start")
		("textureDirectory", po::value<string>()->default_value("tex"), "Subdirectory for Textures")
		("texture01", po::value<string>()->default_value("grass.bmp"), "Grass")
		("texture02", po::value<string>()->default_value("wood.bmp"), "Wood")
		("texture03", po::value<string>()->default_value("bricks.bmp"), "Bricks")
		("texture04", po::value<string>()->default_value("marble.bmp"), "Marble")
		("visualRange", po::value<float>()->default_value(64), "maximal distance for rendering")
		
		
		
		
		
		
		
	;

	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
		std::cout << desc << "\n";
	}
}
