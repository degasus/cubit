#include "controller.h"

Movement::Movement(Controller* controller)
{
	c = controller;
	speedX = 0.0f;
	speedY = 0.0f;
	speedZ = 0.0f;
	position.x = 0.0;
	position.y = 0.0;
	position.z = 0.0;
	position.orientationHorizontal = 0.0;
	position.orientationVertical = 0.0;
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
}

void Movement::init()
{

}

void Movement::performAction(ActionEvent event)
{

}

PlayerPosition Movement::getPosition()
{
	return position;
}

void Movement::setPosition(PlayerPosition pos)
{

}

void Movement::triggerNextFrame()
{

}