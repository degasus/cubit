#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <SDL_mixer.h>
#include <cmath>
#include <LinearMath/btAlignedObjectArray.h>
#include <btBulletDynamicsCommon.h>
#include "GLDebugDrawer.h"

#ifndef _MOVEMENT_H_
#define _MOVEMENT_H_



class Movement;
struct PlayerPosition;
struct ActionEvent;

#include "controller.h"
#include "map.h"

/**
 * Definiert die Position eines Spielers
 * @param x x Position (West -> Ost)
 * @param y y Position (Süd -> Nord)
 * @param z z Position (Unten -> Oben), origin = eye level
 * @param orientationHorizontal horizontal orientation in degrees, 0 = east, 90 = south, 180 = west, 270 = north
 * @param orientationVertical vertical orientation in degrees, 0 = horizontal, 90 = up, -90 = down
 */
struct PlayerPosition{
	double x;
	double y;
	double z;
	double orientationHorizontal;
	double orientationVertical;
	
	btQuaternion rotate;

	inline BlockPosition block(){
		BlockPosition b;
		b.x=std::floor(x+0.00001);
		b.y=std::floor(y+0.00001);
		b.z=std::floor(z+0.00001);

		return b;
	}

	inline std::string to_string(){

		std::ostringstream oss (std::ostringstream::out);

		oss << "pPos X = " << x << "; Y = " << y << "; Z = " << z
			<< "; Orientation: (" << orientationHorizontal << "°, " << orientationVertical << "°);";
			
		return oss.str();
	}
};

struct ActionEvent {
	double value;
	int iValue;
	
	enum type {
		//DUMMY
		NONE,
		
		// move forward
		PRESS_FORWARD, RELEASE_FORWARD,
		
		//move backwards
		PRESS_BACKWARDS, RELEASE_BACKWARDS,
		
		//move left
		PRESS_LEFT, RELEASE_LEFT,
		
		//move right
		PRESS_RIGHT, RELEASE_RIGHT,
		
		//jumping
		PRESS_JUMP, RELEASE_JUMP,
		
		//fast speed
		PRESS_FAST_SPEED, RELEASE_FAST_SPEED,
		
		//ducking
		PRESS_DUCK, RELEASE_DUCK,
		
		//building blocks
		PRESS_BUILD_BLOCK, RELEASE_BUILD_BLOCK,
		
		//removing blocks
		PRESS_REMOVE_BLOCK, RELEASE_REMOVE_BLOCK,
		
		//turning around
		ROTATE_HORIZONTAL, ROTATE_VERTICAL,

		//flying
		PRESS_FLY, RELEASE_FLY,

		//Select a Material
		SELECT_MATERIAL,
	} name;
};

/**
 *
 */
class Movement {
public:
	/**
	 *
	 */
	Movement(Controller *controller);

	~Movement();
	
	void init();
	void config(const boost::program_options::variables_map &c);

	bool loadPosition();
	void savePosition();

	//handle ActionEvents like pressing a key
	void performAction(ActionEvent event);

	//Things to do before rendering next frame
	void triggerNextFrame();
	
	//Get and set the position
	PlayerPosition getPosition();
	void setPosition(PlayerPosition pos);

	//Selected Material
	Material getSelectedMaterial();

	//Get block and plane the user is pointing on
	bool getPointingOn(BlockPosition* block, DIRECTION* plane);

//private:
	bool movDebug;

	void initPhysics();

	//player stats
	PlayerPosition position;
	Material selectedMaterial;

	//offsets for collision detection
	double offset;
	double offsetAbove;

	//Current speed
	float speedForward;
	float speedRight;
	float speedUp;

	//Current acceleration
	float curAccelHorizontal;
	float curAccelVertical;

	//Acceleration to set on movment
	float accelHorizontal;
	float accelVertical;
	double normalAccelVertical;

	//Size of person
	double personSize;
	double personSizeNormal;
	double personSizeDucked;

	//Pointing on
	BlockPosition pointingOnBlock;
	DIRECTION pointingOnPlane;
	double pointingDistance;
	bool isPointingOn;

	//Building
	int lastBuild;
	void buildBlock();
	void removeBlock();
	
	//Movement settings
	float slowMovementSpeed;
	float normalMovementSpeed;
	float movementSpeed;
	float fastSpeedMultiplier;
	double maxFallingSpeed;
	double jumpSpeed;
	bool enableFly;
	boost::filesystem::path workingDirectory;
	boost::filesystem::path dataDirectory;
	boost::filesystem::path localDirectory;
	
	//Steps
	int stepProgress;
	
	Controller *c;
	bool forwardPressed;
	bool rightPressed;
	bool leftPressed;
	bool backwardsPressed;
	bool duckPressed;
	bool jumpPressed;
	bool buildBlockPressed;
	bool removeBlockPressed;
	bool moveFast;
	bool fastPressed;

	void calcBuilding();
	void calcNewSpeed();
	void calcCollisionAndMove();
	void calcDucking();
	void calcPointingOn();
	void calcPhysics();
	DIRECTION calcPointingOnInBlock(PlayerPosition*, BlockPosition);
	
	//Sounds
	Mix_Chunk *step;
	Mix_Chunk *putBlock;

	//BulletPhysics
	btDefaultCollisionConfiguration* collisionConfiguration;
	btCollisionDispatcher* dispatcher;
	btBroadphaseInterface* overlappingPairCache;
	btSequentialImpulseConstraintSolver* solver;
	btDiscreteDynamicsWorld* dynamicsWorld;
	btCollisionShape* groundShape;
	//btAlignedObjectArray<btCollisionShape*> collisionShapes;
	GLDebugDrawer debugDrawer;
	
	btTransform groundTransform;
	btRigidBody* body;
};
#endif
