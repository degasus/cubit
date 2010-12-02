#include <cmath>
#include <iostream>

#include "controller.h"

#include "movement.h"



Movement::Movement(Controller* controller)
{
	c = controller;
	speedForward = 0.0f;
	speedRight = 0.0f;
	speedUp = 0.0f;
	position.x = 0.0;
	position.y = 0.0;
	position.z = 0.0;
	position.orientationHorizontal = 0.0;
	position.orientationVertical = 0.0;
	forwardPressed = false;
	rightPressed = false;
	leftPressed = false;
	backwardsPressed = false;
	duckPressed = false;
	jumpPressed = false;
	buildBlockPressed = false;
	removeBlockPressed = false;
	moveFast = false;
}

void Movement::config(const boost::program_options::variables_map& c)
{
	offset				= c["offset"].as<float>();
	offsetFalling		= c["offsetFalling"].as<float>();
	offsetTop			= c["offsetTop"].as<float>();
	accelHorizontal		= c["accelHorizontal"].as<float>();
	accelVertical		= c["accelVertical"].as<float>();
	personSize			= c["personSize"].as<float>();
	slowMovementSpeed	= c["slowMovementSpeed"].as<float>();
	normalMovementSpeed	= c["normalMovementSpeed"].as<float>();
	fastSpeedMultiplier	= c["fastSpeedMultiplier"].as<float>();
	maxFallingSpeed		= c["maxFallingSpeed"].as<double>();

	movementSpeed = normalMovementSpeed;
}

void Movement::init()
{

}

void Movement::performAction(ActionEvent event)
{
	switch(event.name){
		case ActionEvent::PRESS_FORWARD:
			forwardPressed = true;
			break;
		case ActionEvent::RELEASE_FORWARD:
			forwardPressed = false;
			break;
			
		case ActionEvent::PRESS_BACKWARDS:
			backwardsPressed = true;
			break;
		case ActionEvent::RELEASE_BACKWARDS:
			backwardsPressed = false;
			break;
			
		case ActionEvent::PRESS_LEFT:
			leftPressed = true;
			break;
		case ActionEvent::RELEASE_LEFT:
			leftPressed = false;
			break;
			
		case ActionEvent::PRESS_RIGHT:
			rightPressed = true;
			break;
		case ActionEvent::RELEASE_RIGHT:
			rightPressed = false;
			break;
			
		case ActionEvent::PRESS_JUMP:
			jumpPressed = true;
			break;
		case ActionEvent::RELEASE_JUMP:
			jumpPressed = false;
			break;

		case ActionEvent::PRESS_FAST_SPEED:
			movementSpeed *= fastSpeedMultiplier;
			break;
		case ActionEvent::RELEASE_FAST_SPEED:
			movementSpeed /= fastSpeedMultiplier;
			break;
			
		case ActionEvent::PRESS_DUCK:
			duckPressed = true;
			offsetFalling *= 2;
			movementSpeed = slowMovementSpeed;
			break;
		case ActionEvent::RELEASE_DUCK:
			duckPressed = false;
			offsetFalling /= 2;
			movementSpeed = normalMovementSpeed;
			break;
			
		case ActionEvent::PRESS_BUILD_BLOCK:
			buildBlockPressed = true;
			break;
		case ActionEvent::RELEASE_BUILD_BLOCK:
			buildBlockPressed = false;
			break;
			
		case ActionEvent::PRESS_REMOVE_BLOCK:
			removeBlockPressed = true;
			break;
		case ActionEvent::RELEASE_REMOVE_BLOCK:
			removeBlockPressed = false;
			break;

		case ActionEvent::ROTATE_HORIZONTAL:
			//std::cout << "r_hor: " << event.value << std::endl;
			position.orientationHorizontal += event.value;
			if(position.orientationHorizontal > 360)
				position.orientationHorizontal -= 360;
			if(position.orientationHorizontal < 0)
				position.orientationHorizontal += 360;
			break;
		case ActionEvent::ROTATE_VERTICAL:
			//std::cout << "r_ver: " << event.value << std::endl;
			position.orientationVertical += event.value;
			if(position.orientationVertical > 90)
				position.orientationVertical = 90;
			if(position.orientationVertical < -90)
				position.orientationVertical = -90;
			break;

		default:
			break;
	}
}

PlayerPosition Movement::getPosition()
{
	return position;
}

void Movement::setPosition(PlayerPosition pos)
{
	position = pos;
}

void Movement::calcNewSpeed()
{
	if(forwardPressed){
		if(speedForward <= movementSpeed)
			speedForward += accelHorizontal;
		else
			speedForward = movementSpeed;
	}
	else if(speedForward > 0){
		speedForward -= accelHorizontal;
		if(speedForward < 0)
			speedForward = 0;
	}
	if(backwardsPressed){
		if(speedForward >= -movementSpeed)
			speedForward -= accelHorizontal;
		else
			speedForward = -movementSpeed;
	}
	else if(speedForward < 0){
		speedForward += accelHorizontal;
		if(speedForward > 0)
			speedForward = 0;
	}
	if(rightPressed){
		if(speedRight <= movementSpeed)
			speedRight += accelHorizontal;
		else
			speedRight = movementSpeed;
	}
	else if(speedRight > 0){
		speedRight -= accelHorizontal;
		if(speedRight < 0)
			speedRight = 0;
	}
	if(leftPressed){
		if(speedRight >= movementSpeed)
			speedRight -= accelHorizontal;
		else
			speedRight = -movementSpeed;
	}
	else if(speedRight < 0){
		speedRight += accelHorizontal;
		if(speedRight > 0)
			speedRight = 0;
	}
	if(c->map.getBlock(position.block()+DIRECTION_DOWN) != 0){
		if(speedUp >= maxFallingSpeed)
			speedUp -= accelVertical;
		else
			speedUp = -maxFallingSpeed;
	}
	else if(std::floor(position.z) > 0.1)
		speedUp = 0;
}

void Movement::calcCollisionAndMove()
{
	PlayerPosition oldPos = position;
	
	position.x += speedForward*cos(2*M_PI*position.orientationHorizontal/360);
	position.y += speedForward*sin(2*M_PI*position.orientationHorizontal/360);
	
	position.x += -speedRight*sin(2*M_PI*position.orientationHorizontal/360);
	position.y += speedRight*cos(2*M_PI*position.orientationHorizontal/360);

	position.z += speedUp;
}

void Movement::triggerNextFrame()
{
	calcNewSpeed();
	calcCollisionAndMove();
	//calcBuilding();
}