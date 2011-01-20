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
	
	database = 0;
	sql_mutex = 0;

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
	
	if(database) sqlite3_close(database);
	if(sql_mutex) SDL_DestroyMutex(sql_mutex);
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
	if(sqlite3_open((vm["workingDirectory"].as<fs::path>() / "cubit.db").file_string().c_str(), &database) != SQLITE_OK)
	// Es ist ein Fehler aufgetreten!
	std::cout << "Fehler beim Öffnen: " << sqlite3_errmsg(database) << std::endl;
	
	// create tables
	sqlite3_exec(database,
		"CREATE TABLE IF NOT EXISTS area ( "
			"posx INT NOT NULL, "
			"posy INT NOT NULL, "
			"posz INT NOT NULL, "
			"empty BOOL NOT NULL DEFAULT 0, "
			"revision INT DEFAULT 0, "
			"full INT NOT NULL DEFAULT 0, "	
			"blocks INT NOT NULL DEFAULT -1, "
			"data BLOB(32768), "
			"PRIMARY KEY (posx, posy, posz) "
		");"
		, 0, 0, 0);
	sqlite3_exec(database, "PRAGMA synchronous = 0;", 0, 0, 0);
	sql_mutex = SDL_CreateMutex();
	
}


void Controller::parse_command_line(int argc, char *argv[]) {
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("noFullX", po::value<int>()->default_value(1024), "set the default x-resolution")
		("noFullY", po::value<int>()->default_value(768), "set the default y-resolution")
		("fullscreen", po::value<bool>()->default_value(0), "start in fullscreen")
		("enableAntiAliasing", po::value<bool>()->default_value(0), "enables Multi-Sampling")
		("textureFilterMethod", po::value<int>()->default_value(3), "set the texture filter method (1=nearest; 2=linear; 3=triliear)")
		("bgColorR", po::value<float>()->default_value(0.6), "Background Color Red")
		("bgColorG", po::value<float>()->default_value(0.7), "Background Color Green")
		("bgColorB", po::value<float>()->default_value(0.8), "Background Color Blue")
		("bgColorA", po::value<float>()->default_value(1.0), "Background Color Aplha")
		("fogDense", po::value<float>()->default_value(0.6), "Densitivity of Fog")
		("fogStartFactor", po::value<float>()->default_value(0.8), "Percental distance to fog start")
		("visualRange", po::value<int>()->default_value(4), "maximal distance for rendering")
		("enableFog", po::value<bool>()->default_value(1), "enable Fog")
		("areasPerFrameRendering", po::value<int>()->default_value(3), "set the maximal rendered areas per frame")
		("areasPerFrameLoading", po::value<int>()->default_value(20), "set the maximal from hard disk loaded areas per frame")
		
		("destroyAreaFaktor", po::value<double>()->default_value(2), "distance for destroying areas")

		("storeMaps", po::value<bool>()->default_value(1), "should maps be saved and loaded from harddisk")
	
		("offset", po::value<double>()->default_value(0.2), "offset for horizontal collision detection")
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
		("turningSpeed", po::value<double>()->default_value(0.2), "speed factor for turning (\"mouse speed\")")
		("jumpSpeed", po::value<double>()->default_value(0.215), "initial speed when jumping")
#ifdef _WIN32
		("workingDirectory", po::value<fs::path>()->default_value(fs::path(std::getenv("PROGRAMFILES")) / "Cubit"), "Folder for saving areas")
		("dataDirectory", po::value<fs::path>()->default_value(fs::path(std::getenv("PROGRAMFILES")) / "Cubit"), "Folder for music and images")
		("localDirectory", po::value<fs::path>()->default_value(fs::path(std::getenv("PROGRAMFILES")) / "Cubit"), "Folder for music and images")
#else
		("workingDirectory", po::value<fs::path>()->default_value(fs::path(std::getenv("HOME")) / ".cubit"), "Folder for saving areas")
		("dataDirectory", po::value<fs::path>()->default_value(fs::path(CMAKE_INSTALL_PREFIX) / "share" / "games" / "cubit"), "Folder for music and images")
		("localDirectory", po::value<fs::path>()->default_value(fs::path(argv[0]).remove_filename()), "Folder for music and images")
#endif
		//Collision
		("maxMovingObjects", po::value<int>()->default_value(250), "maximum of moving objects in the world")
		
		//UI
		("highlightWholePlane", po::value<bool>()->default_value(1), "highlight the pointing on plane without depth test")

		//Keys
		("k_forward", po::value<int>()->default_value(119), "KeyCode for moving forward (w)")
		("k_backwards", po::value<int>()->default_value(115), "KeyCode for moving backwards (s)")
		("k_left", po::value<int>()->default_value(97), "KeyCode for moving left (a)")
		("k_right", po::value<int>()->default_value(100), "KeyCode for moving right (d)")
		("k_moveFast", po::value<int>()->default_value(102), "KeyCode for moving fastly (f)")
		("k_catchMouse", po::value<int>()->default_value(109), "KeyCode for catching mouse (m)")
		("k_jump", po::value<int>()->default_value(32), "KeyCode for jumping (Space)")
		("k_throw", po::value<int>()->default_value(116), "KeyCode for throwing blocks (T)")
		("k_fly", po::value<int>()->default_value(60), "KeyCode for enabling/disabling fly (<)")
		("k_duck", po::value<int>()->default_value(304), "KeyCode for ducking (Left-Shift)")
		("k_lastMat", po::value<int>()->default_value(281), "KeyCode for scrolling down the materiallist (PgDown)")
		("k_nextMat", po::value<int>()->default_value(280), "KeyCode for scrolling up the materiallist (PgUp)")
		("k_selMat", po::value<int>()->default_value(112), "KeyCode for enabling the pipette mode (selecting material) (P)")
		("k_music", po::value<int>()->default_value(46), "KeyCode for start/stop music (.)")
		("k_quit", po::value<int>()->default_value(27), "KeyCode for exiting (Esc)")
	;
		
	//command-line args
	po::store(po::parse_command_line(argc, argv, desc), vm);

	//config file
	std::ifstream i((vm["workingDirectory"].as<fs::path>() / "cubit.conf").file_string().c_str());
	if (i.is_open()) {
		po::store(po::parse_config_file(i, desc), vm);
	}
	i.close();
	
	po::notify(vm);
	
	
	if (vm.count("help")) {
		std::cout << desc << "\n";
	}
}
