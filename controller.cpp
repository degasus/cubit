#include <iostream>

#include "controller.h"



namespace po = boost::program_options;

Controller::Controller(int argc, char *argv[]) {
	parse_command_line(argc, argv);
}

void Controller::run() {

}


void Controller::parse_command_line(int argc, char *argv[]) {
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("noFullX", po::value<int>()->default_value(640), "set the default x-resolution")
		("noFullY", po::value<int>()->default_value(480), "set the default y-resolution")
	;

	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);    

	if (vm.count("help")) {
		std::cout << desc << "\n";
	}

	if (vm.count("noFullX")) {
		std::cout << "X resolution was set to " 
	<< vm["noFullX"].as<int>() << ".\n";
	} else {
		std::cout << "x resolution was not set.\n";
	}

}
