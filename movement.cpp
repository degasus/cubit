#include <cmath>
#include <iostream>

#include "controller.h"

#include "movement.h"
#include "map.h"



Movement::Movement(Controller* controller)
{
	c = controller;
	speedForward = 0.0f;
	speedRight = 0.0f;
	speedUp = 0.0f;
	position.x = 0.0;
	position.y = 0.0;
	position.z = 10.0;
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
	offset				= c["offset"].as<double>();
	offsetAbove			= c["offsetAbove"].as<double>();
	accelHorizontal		= c["accelHorizontal"].as<double>();
	accelVertical		= c["accelVertical"].as<double>();
	personSizeNormal	= c["personSizeNormal"].as<double>();
	personSizeDucked	= c["personSizeDucked"].as<double>();
	slowMovementSpeed	= c["slowMovementSpeed"].as<double>();
	normalMovementSpeed	= c["normalMovementSpeed"].as<double>();
	fastSpeedMultiplier	= c["fastSpeedMultiplier"].as<double>();
	maxFallingSpeed		= c["maxFallingSpeed"].as<double>();
	jumpSpeed			= c["jumpSpeed"].as<double>();

	movementSpeed = normalMovementSpeed;
	personSize = personSizeNormal;
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
			movementSpeed = slowMovementSpeed;
			break;
		case ActionEvent::RELEASE_DUCK:
			duckPressed = false;
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

void Movement::calcDucking()
{
	if(duckPressed && personSize > personSizeDucked){
		personSize -= 0.05;
		position.z -= 0.05;
		if(personSize < personSizeDucked)
			personSize = personSizeDucked;
	}
	if(!duckPressed && personSize < personSizeNormal){
		personSize += 0.05;
		position.z += 0.05;
		if(personSize > personSizeNormal)
			personSize = personSizeNormal;
	}
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
		if(speedRight >= -movementSpeed)
			speedRight -= accelHorizontal;
		else
			speedRight = -movementSpeed;
	}
	else if(speedRight < 0){
		speedRight += accelHorizontal;
		if(speedRight > 0)
			speedRight = 0;
	}

	int feetBlock = 1;
	PlayerPosition feetPos = position;
	feetPos.z -= personSize;
	try{
		feetBlock = c->map.getBlock(feetPos.block());
	}
	catch(NotLoadedException e){
		std::cout << "blockFeet NotLoadedException" << std::endl;
	}
	//Luft unten drunter -> fallen (inkl. Collision Detection on bottom)
	if(feetBlock == 0){
		if(speedUp >= maxFallingSpeed)
			speedUp -= accelVertical;
		else
			speedUp = maxFallingSpeed;
	}
	else if(speedUp < 0){
		speedUp = 0;
	}
}

void Movement::calcCollisionAndMove(){
	PlayerPosition oldPos = position;

	//Horizontal Movement
	//Forward/Back
	position.x += speedForward*cos(2*M_PI*position.orientationHorizontal/360);
	position.y += speedForward*sin(2*M_PI*position.orientationHorizontal/360);
	//Right/Left
	position.x += -speedRight*sin(2*M_PI*position.orientationHorizontal/360);
	position.y += speedRight*cos(2*M_PI*position.orientationHorizontal/360);
	//Z-Movement Up/Down
	position.z += speedUp;

	int posBlock = 0;
	int feetBlock = 1;
	PlayerPosition feetPos = position;
	feetPos.z -= personSize;
	try{
		posBlock = c->map.getBlock(position.block());
		feetBlock = c->map.getBlock(feetPos.block());
	}
	catch(NotLoadedException e){
		std::cout << "posBlock NotLoadedException" << std::endl;
		std::cout << "feetBlock NotLoadedException" << std::endl;
	}

	//Z-Collision
	//Jumping
	if(feetBlock != 0 && jumpPressed){
		std::cout << "jump" << std::endl;
		speedUp = jumpSpeed*(movementSpeed/normalMovementSpeed);
	}
	//Falling
	if((posBlock != 0 || feetBlock != 0) && speedUp <= 0){
		speedUp = 0.0;
		position.z = floor(position.z-personSize)+1.0+personSize;
	}

	//X-Collision
	//resetting vars
	posBlock = 0;
	feetBlock = 1;
	int offsetBlock = 1;
	int offsetFeetBlock = 1;
	feetPos = position;
	feetPos.z -= personSize;
	PlayerPosition offsetPos = position;
	PlayerPosition offsetFeetPos = feetPos;
	//x moving forward
	if(position.x > oldPos.x){
		offsetPos.x += offset;
		offsetFeetPos.x += offset;
	}
	//x moving backwards
	else if(position.x < oldPos.x){
		offsetPos.x -= offset;
		offsetFeetPos.x -= offset;
	}
	if(position.x != oldPos.x){
		try{
			posBlock = c->map.getBlock(position.block());
			feetBlock = c->map.getBlock(feetPos.block());
			offsetBlock = c->map.getBlock(offsetPos.block());
			offsetFeetBlock = c->map.getBlock(offsetFeetPos.block());
		}
		catch(NotLoadedException e){
			std::cout << "X-collision detection NotLoadedException" << std::endl;
		}
		int moveNot = posBlock + offsetBlock + feetBlock + offsetFeetBlock;
		if(moveNot != 0)
			position.x = oldPos.x;
	}

	//Y-Collision
	//resetting vars
	posBlock = 0;
	feetBlock = 1;
	offsetBlock = 1;
	offsetFeetBlock = 1;
	feetPos = position;
	feetPos.z -= personSize;
	offsetPos = position;
	offsetFeetPos = feetPos;
	//y moving forward
	if(position.y > oldPos.y){
		offsetPos.y += offset;
		offsetFeetPos.y += offset;
	}
	//y moving backwards
	else if(position.y < oldPos.y){
		offsetPos.y -= offset;
		offsetFeetPos.y -= offset;
	}
	if(position.y != oldPos.y){
		try{
			posBlock = c->map.getBlock(position.block());
			feetBlock = c->map.getBlock(feetPos.block());
			offsetBlock = c->map.getBlock(offsetPos.block());
			offsetFeetBlock = c->map.getBlock(offsetFeetPos.block());
		}
		catch(NotLoadedException e){
			std::cout << "Y-collision detection NotLoadedException" << std::endl;
		}
		int moveNot = posBlock + offsetBlock + feetBlock + offsetFeetBlock;
		if(moveNot != 0)
			position.y = oldPos.y;
	}
}

void Movement::triggerNextFrame()
{
	calcDucking();
	calcNewSpeed();
	calcCollisionAndMove();
	//calcBuilding();
}
