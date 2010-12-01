#ifndef _MOVEMENT_H_
#define _MOVEMENT_H_

#include <boost/program_options.hpp>


class Movement;
struct PlayerPosition;
struct ActionEvent;

#include "controller.h"

/**
 * Definiert die Position eines Spielers
 * @param x x Position (West -> Ost)
 * @param y y Position (Süd -> Nord)
 * @param z z Position (Unten -> Oben), origin = eye level
 * @param orientation_horizontal horizontal orientation in degrees, 0 = east
 * @param orientation_vertical vertical orientation in degrees, 0 = horizontal
 */
struct PlayerPosition {
	double x;
	double y;
	double z;
	double orientationHorizontal;
	double orientationVertical;
};

struct ActionEvent {
	double value;
	
	enum type {
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
	};
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

	void performAction(ActionEvent event);

	//
	void triggerNextFrame();
	
	//Get and set the position
	PlayerPosition getPosition();
	void setPosition(PlayerPosition pos);

private:
	//current position
	PlayerPosition position;

	//offsets for collision detection
	float offset;
	float offsetFalling;
	float offsetTop;

	//Current speed
	float speedX;
	float speedY;
	float speedZ;

	//Current acceleration
	float curAccelHorizontal;
	float curAccelVertical;

	//Acceleration to set on movment
	float accelHorizontal;
	float accelVertical;

	//Size of person
	float personSize;
	
	float slowMovementSpeed;
	float normalMovementSpeed;
	float movementSpeed;
	float fastSpeedMultiplier;
	
	Controller *c;
	
};
#endif