#ifndef _MOVEMENT_H_
#define _MOVEMENT_H_

#include <boost/program_options.hpp>


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
struct PlayerPosition {
	double x;
	double y;
	double z;
	double orientationHorizontal;
	double orientationVertical;

	inline BlockPosition block(){
		BlockPosition b;
		b.x=std::floor(x);
		b.y=std::floor(y);
		b.z=std::floor(z);

		return b;
	}
};

struct ActionEvent {
	double value;
	
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
	
	void init();
	void config(const boost::program_options::variables_map &c);

	//handle ActionEvents like pressing a key
	void performAction(ActionEvent event);

	//Things to do before rendering next frame
	void triggerNextFrame();
	
	//Get and set the position
	PlayerPosition getPosition();
	void setPosition(PlayerPosition pos);

private:
	//current position
	PlayerPosition position;

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

	//Size of person
	float personSize;
	double personSizeNormal;
	double personSizeDucked;
	
	float slowMovementSpeed;
	float normalMovementSpeed;
	float movementSpeed;
	float fastSpeedMultiplier;
	double maxFallingSpeed;
	double jumpSpeed;
	
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

	void calcBuilding();
	void calcNewSpeed();
	void calcCollisionAndMove();
	void calcDucking();
};
#endif