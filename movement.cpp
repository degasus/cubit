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
	pointingDistance	= c["pointingDistance"].as<int>();
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

PlayerPosition Movement::getPosition(){
	return position;
}

void Movement::setPosition(PlayerPosition pos){
	position = pos;
}

void Movement::getPointingOn(BlockPosition* block, DIRECTION* plane){
	*block = pointingOnBlock;
	*plane = pointingOnPlane;
}

void Movement::calcDucking(){
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

/*void Movement::calcPointingOn(){
	PlayerPosition lastPos = position;
	lastPos.x = lastPos.x - floor(lastPos.x);
	lastPos.y = lastPos.y - floor(lastPos.y);
	lastPos.z = lastPos.z - floor(lastPos.z);

	//int blockX = floor(posX);
	//int blockY = floor(posY + personSize);
	//int blockZ = floor(posZ);
	
	pointingOnPlane = -2;
	
	double distanceQ = 0;
	int counter = 0;
	
	while(c->map.getBlock(lastPos.block()) == 0 && distanceQ <= pointingDistance*pointingDistance && counter <= 30 && pointingOnPlane != -1){
		counter++;
		lastPointingOn = calcPointingOnInBlock(&lastX, &lastY, &lastZ);
		switch (lastPointingOn) {
			//Back
			case 0: blockZ--; lastZ++; break;
			//Front
			case 1: blockZ++; lastZ--; break;
			//Bottom
			case 2: blockY--; lastY++; break;
			//Top
			case 3: blockY++; lastY--; break;
			//Left
			case 4: blockX--; lastX++; break;
			//Right
			case 5: blockX++; lastX--; break;
		}
		
		double dx = blockX+lastX-posX;
		double dy = blockY+lastY-posY-personSize;
		double dz = blockZ+lastZ-posZ;
		
		distanceQ = dx*dx + dy*dy + dz*dz;
	}
	
	
	if(landschaft[blockX*ysize*zsize + blockY*zsize  + blockZ] != 0){
		pointingOnX = floor(blockX);
		pointingOnY = floor(blockY);
		pointingOnZ = floor(blockZ);
	}
	else{
		pointingOnX = -1;
		pointingOnY = -1;
		pointingOnZ = -1;
	}
}

//Berechnet die Fläche, auf die von der Startposition aus (Parameter) mit der aktuellen Blickrichtung
//@return: ID der Fläche, auf die man zeigt
//Am Ende sind die Parameter auf den Schnittpunkt gesetzt
int Movement::calcPointingOnInBlock(double* startX, double* startY, double* startZ){
	Matrix<double,3,3> left(0);
	Matrix<double,1,3> right(0);
	Matrix<double,1,3> result(0);
	
	//bleibt immer gleich (Blickrichtung)
	left.data[2][0] = sin(M_PI*x/180.) * cos(M_PI*y/180.);
	left.data[2][1] = -sin(M_PI*y/180.);
	left.data[2][2] = -cos(M_PI*x/180.) * cos(M_PI*y/180.);
	
	//Fläche 0 (Front)
	left.data[0][0] = 1;
	left.data[1][1] = 1;
	right.data[0][0] = 1-*startX;
	right.data[0][1] = 1-*startY;
	right.data[0][2] = -*startZ;
	result = left.LU().solve(right);
	if( 0 <= result.data[0][0] && result.data[0][0] <= 1
		&& 0 <= result.data[0][1] && result.data[0][1] <= 1
		&& 0 < result.data[0][2]) {
		*startX = 1-result[0][0];
	*startY = 1-result[0][1];
	*startZ = 0;
	return 0;
		}
		
		//Fläche 1 (Back)
		right.data[0][2] = 1-*startZ;
		result = left.LU().solve(right);
		if( 0 <= result.data[0][0] && result.data[0][0] <= 1
			&& 0 <= result.data[0][1] && result.data[0][1] <= 1
			&& 0 < result.data[0][2]) {
			*startX = 1-result[0][0];
		*startY = 1-result[0][1];
		*startZ = 1;
		return 1;
			}
			
			//Fläche 2 (Top)
			left.data[1][1] = 0;
			left.data[1][2] = 1;
			right.data[0][1] = -*startY;
			result = left.LU().solve(right);
			if( 0 <= result.data[0][0] && result.data[0][0] <= 1
				&& 0 <= result.data[0][1] && result.data[0][1] <= 1
				&& 0 < result.data[0][2]){
				*startX = 1-result[0][0];
			*startY = 0;
			*startZ = 1-result[0][1];
			return 2;
				}
				
				//Fläche 3 (Bottom)
				right.data[0][1] = 1-*startY;
				result = left.LU().solve(right);
				if( 0 <= result.data[0][0] && result.data[0][0] <= 1
					&& 0 <= result.data[0][1] && result.data[0][1] <= 1
					&& 0 < result.data[0][2]){
					*startX = 1-result[0][0];
				*startY = 1;
				*startZ = 1-result[0][1];
				return 3;
					}
					
					//Fläche 4 (Right)
					left.data[0][0] = 0;
					left.data[0][1] = 1;
					right.data[0][0] = -*startX;
					result = left.LU().solve(right);
					if( 0 <= result.data[0][0] && result.data[0][0] <= 1
						&& 0 <= result.data[0][1] && result.data[0][1] <= 1
						&& 0 < result.data[0][2]){
						*startX = 0;
					*startY = 1-result[0][0];
					*startZ = 1-result[0][1];
					return 4;
						}
						
						//Fläche 5 (Left)
						right.data[0][0] = 1-*startX;
						result = left.LU().solve(right);
						if( 0 <= result.data[0][0] && result.data[0][0] <= 1
							&& 0 <= result.data[0][1] && result.data[0][1] <= 1
							&& 0 < result.data[0][2]){
							*startX = 1;
						*startY = 1-result[0][0];
						*startZ = 1-result[0][1];
						return 5;
							}
							
							//Falls keine Austrittsebene gefunden wird Error
							return -1;
}*/

void Movement::calcBuilding(){

}

void Movement::triggerNextFrame(){
	calcDucking();
	calcNewSpeed();
	calcCollisionAndMove();
	//calcPointingOn();
	//calcBuilding();
}