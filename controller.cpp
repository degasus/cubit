#include <boost/filesystem.hpp>
#include <iostream>
#include <time.h>
#include <fstream>

#include "controller.h"




namespace po = boost::program_options;
namespace fs = boost::filesystem;

Controller::Controller(int argc, char *argv[])
{
	srand ( time(NULL) );

	parse_command_line(argc, argv);
	
	ui = new UInterface(this);
	renderer = new Renderer(this);
	movement = new Movement(this);
	map = new Map(this);
	
	
}

Controller::~Controller() {
	delete ui;
	delete renderer;
	delete map;
	delete movement;
}

void Controller::run() {
	if(vm["help"].empty()) {
		
		ui->config(vm);
		renderer->config(vm);
		movement->config(vm);
		map->config(vm);

		init();
		ui->init();
		renderer->init();
		movement->init();
		map->init();

		ui->run();
	}
}

void Controller::init() {
	//creating working directory
	fs::create_directory( vm["workingDirectory"].as<fs::path>() );
	
	// init SQL
//	if(sqlite3_open((vm["workingDirectory"].as<fs::path>() / "cubit.db").string().c_str(), &database) != SQLITE_OK)
	// Es ist ein Fehler aufgetreten!
}


void Controller::parse_command_line(int argc, char *argv[]) {
	// Declare the supported options.
	po::options_description common("Common options");
	common.add_options()
		("help", "produce this short help message, add --verbose for the full help page")
		("fullscreen", po::value<bool>()->default_value(0), "start in fullscreen")
		("visualRange", po::value<int>()->default_value(8), "maximal distance for rendering")
		//Net
		("server", po::value<std::string>()->default_value("cubit-project.com"), "Server to connect to.")
#ifdef _WIN32
		("nick", po::value<std::string>()->default_value(std::getenv("USERNAME")), "Nickname to show to other players.");
#else
		("nick", po::value<std::string>()->default_value(std::getenv("USER")), "Nickname to show to other players.");
#endif

	po::options_description advanced("Advanced options");
	advanced.add_options()
		("verbose", "verbose output")
		("noFullX", po::value<int>()->default_value(1024), "set the default x-resolution")
		("noFullY", po::value<int>()->default_value(768), "set the default y-resolution")
		("enableAntiAliasing", po::value<bool>()->default_value(0), "enables Multi-Sampling")
		("textureFilterMethod", po::value<int>()->default_value(4), "set the texture filter method (1=nearest; 2=linear; 3=triliear)")
		("bgColorR", po::value<float>()->default_value(0.3), "Background Color Red")
		("bgColorG", po::value<float>()->default_value(0.5), "Background Color Green")
		("bgColorB", po::value<float>()->default_value(1.0), "Background Color Blue")
		("bgColorA", po::value<float>()->default_value(1.0), "Background Color Aplha")
		("fogDense", po::value<float>()->default_value(0.6), "Densitivity of Fog")
		("fogStartFactor", po::value<float>()->default_value(0.8), "Percental distance to fog start")
		("enableFog", po::value<bool>()->default_value(1), "enable Fog")
		("enable3D", po::value<bool>()->default_value(0), "enable 3D")
		("angleOfVision", po::value<double>()->default_value(60), "angle of vision in degrees")
		("textureSize", po::value<int>()->default_value(256), "alloc texturesize")
		("areasPerFrameRendering", po::value<int>()->default_value(20), "set the maximal rendered areas per frame")
		("areasPerFrameLoading", po::value<int>()->default_value(100), "set the maximal from hard disk loaded areas per frame")
		
		("destroyAreaFaktor", po::value<double>()->default_value(2), "distance for destroying areas")
		("generatorThreads", po::value<int>()->default_value(1), "number of threads for generating maps")

		("storeMaps", po::value<bool>()->default_value(1), "should maps be saved and loaded from harddisk")
	
		("offset", po::value<double>()->default_value(0.3), "offset for horizontal collision detection")
		("offsetAbove", po::value<double>()->default_value(0.1f), "offset above person for vertical collision detection")
		("accelHorizontal", po::value<double>()->default_value(1.0), "accelleration in horizontal diretion")
		("accelVertical", po::value<double>()->default_value(0.016), "accelleration in vertical diretion")
		("personSizeNormal", po::value<double>()->default_value(1.5), "normal size of person (should be between 1.01 and 1.99)")
		("personSizeDucked", po::value<double>()->default_value(1.2), "ducked size of person (should be between 1.01 and 1.99 && < personSizeNormal)")
		("pointingDistance", po::value<int>()->default_value(4), "range to build and remove blocks")
		("slowMovementSpeed", po::value<double>()->default_value(0.05), "speed when moving slowly")
		("normalMovementSpeed", po::value<double>()->default_value(0.1), "speed when moving normally")
		("fastSpeedMultiplier", po::value<double>()->default_value(5.72341), "speed multiplier when moving fast")
		("maxFallingSpeed", po::value<double>()->default_value(-0.99), "fastest reachable speed on falling")
		("turningSpeed", po::value<double>()->default_value(0.04), "speed factor for turning (\"mouse speed\")")
		("jumpSpeed", po::value<double>()->default_value(0.215), "initial speed when jumping")
#ifdef _WIN32
		("workingDirectory", po::value<fs::path>()->default_value(fs::path(std::getenv("APPDATA")) / "Cubit"), "Folder for saving areas")
		("dataDirectory", po::value<fs::path>()->default_value(fs::path(std::getenv("PROGRAMFILES")) / "Cubit"), "Folder for music and images")
		("localDirectory", po::value<fs::path>()->default_value(fs::path(std::getenv("PROGRAMFILES")) / "Cubit"), "Folder for music and images")
#else
		("workingDirectory", po::value<fs::path>()->default_value(fs::path(std::getenv("HOME")) / ".cubit"), "Folder for saving areas")
		("dataDirectory", po::value<fs::path>()->default_value(fs::path(CMAKE_INSTALL_PREFIX) / "share" / "games" / "cubit"), "Folder for music and images")
		("localDirectory", po::value<fs::path>()->default_value(fs::path(argv[0]).remove_filename()), "Folder for music and images")
#endif

		//Movement
		("enableFX", po::value<bool>()->default_value(1), "enable sound FX")
		("sandboxMode", po::value<bool>()->default_value(0), "run in sandbox mode")
		
		//Collision
		("maxMovingObjects", po::value<int>()->default_value(250), "maximum of moving objects in the world")
		
		//UI
		("highlightWholePlane", po::value<bool>()->default_value(1), "highlight the pointing on plane without depth test")
		("msg_display_time", po::value<int>()->default_value(3000), "Time to display messages in milliseconds")

		//Keys
		("k_forward", po::value<int>()->default_value(119), "KeyCode for moving forward (W)")
		("k_backwards", po::value<int>()->default_value(115), "KeyCode for moving backwards (S)")
		("k_left", po::value<int>()->default_value(97), "KeyCode for moving left (A)")
		("k_right", po::value<int>()->default_value(100), "KeyCode for moving right (D)")
		("k_moveFast", po::value<int>()->default_value(102), "KeyCode for moving fastly (F)")
		("k_catchMouse", po::value<int>()->default_value(109), "KeyCode for catching mouse (M)")
		("k_jump", po::value<int>()->default_value(32), "KeyCode for jumping (Space)")
		("k_throw", po::value<int>()->default_value(116), "KeyCode for throwing blocks (T)")
		("k_toggle_debug", po::value<int>()->default_value(284), "KeyCode for enabling/disabling debug bar (F3) - only F-Keys possible")
		("k_toggle_walk", po::value<int>()->default_value(113), "KeyCode for enabling/disabling auto walk (Q)")
		("k_fly", po::value<int>()->default_value(60), "KeyCode for enabling/disabling fly (<)")
		("k_duck", po::value<int>()->default_value(304), "KeyCode for ducking (Left-Shift)")
		("k_lastMat", po::value<int>()->default_value(281), "KeyCode for scrolling down the materiallist (PgDown)")
		("k_nextMat", po::value<int>()->default_value(280), "KeyCode for scrolling up the materiallist (PgUp)")
		("k_selMat", po::value<int>()->default_value(112), "KeyCode for enabling the pipette mode (selecting material) (P)")
		("k_music", po::value<int>()->default_value(46), "KeyCode for start/stop music (.)")
		("k_quit", po::value<int>()->default_value(27), "KeyCode for exiting (Esc)")
	;
	
	po::options_description cmdline_options;
	cmdline_options.add(common).add(advanced);
		
	try{
		//command-line args
		po::store(po::parse_command_line(argc, argv, cmdline_options), vm);

		po::notify(vm);
		
		//config file
		std::ifstream i((vm["workingDirectory"].as<fs::path>() / "cubit.conf").string().c_str());
		if (i.is_open()) {
			po::store(po::parse_config_file(i, cmdline_options), vm);
		}
		i.close();
		
		po::notify(vm);
	} catch(std::exception &e) {
		std::cout << "Error: " << e.what() << std::endl << std::endl;
		std::cout << common << std::endl;
		
		//TODO vm["help"] should be set
	}
	
	
	if (vm.count("help")) {
		if (vm.count("verbose")) {
			std::cout << cmdline_options << std::endl;
		} else {
			std::cout << common << std::endl;
		}
	}
}

std::string Controller::find_file(std::string str) {
	std::ifstream i;
	i.open(str.c_str());
	if(i.is_open()) return str;
	
	i.open((vm["localDirectory"].as<fs::path>() / str).string().c_str());
	if(i.is_open()) return (vm["localDirectory"].as<fs::path>() / str).string();
	
	i.open((vm["workingDirectory"].as<fs::path>() / str).string().c_str());
	if(i.is_open()) return (vm["workingDirectory"].as<fs::path>() / str).string();
	
	i.open((vm["dataDirectory"].as<fs::path>() / str).string().c_str());
	if(i.is_open()) return (vm["dataDirectory"].as<fs::path>() / str).string();
	
	std::cout << "File not found: " << str << std::endl;
	return "";
}

