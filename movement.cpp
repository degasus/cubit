#define _USE_MATH_DEFINES
#include <map>
#include <math.h>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <SDL_mixer.h>
#include <btBulletDynamicsCommon.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>

#include "controller.h"

#include "movement.h"
#include "map.h"


namespace fs = boost::filesystem;

Movement::Movement(Controller* controller)
{
	c = controller;
	speedForward = 0.0f;
	speedRight = 0.0f;
	speedUp = 0.0f;
	forwardPressed = false;
	rightPressed = false;
	leftPressed = false;
	backwardsPressed = false;
	duckPressed = false;
	jumpPressed = false;
	buildBlockPressed = false;
	removeBlockPressed = false;
	fastPressed = false;
	throwPressed = false;
	moveFast = false;
	isPointingOn = false;
	enableFly = false;
	lastBuild = 0;
	lastRemove = 0;
	normalMaxRemove = 10;
	stepProgress = 0;
	selectedMaterial = 1;
	movDebug = 0;
}

Movement::~Movement(){
	savePosition();
	saveInventory();
}

void Movement::config(const boost::program_options::variables_map& c){
	offset				= c["offset"].as<double>();
	offsetAbove			= c["offsetAbove"].as<double>();
	accelHorizontal		= c["accelHorizontal"].as<double>()/2;
	normalAccelVertical	= c["accelVertical"].as<double>();
	personSizeNormal	= c["personSizeNormal"].as<double>();
	personSizeDucked	= c["personSizeDucked"].as<double>();
	pointingDistance	= c["pointingDistance"].as<int>();
	slowMovementSpeed	= c["slowMovementSpeed"].as<double>();
	normalMovementSpeed	= c["normalMovementSpeed"].as<double>();
	fastSpeedMultiplier	= c["fastSpeedMultiplier"].as<double>();
	maxFallingSpeed		= c["maxFallingSpeed"].as<double>();
	jumpSpeed			= c["jumpSpeed"].as<double>();
	workingDirectory 	= c["workingDirectory"].as<fs::path>();
	dataDirectory 		= c["dataDirectory"].as<fs::path>();
	localDirectory 		= c["localDirectory"].as<fs::path>();
	maxMovingObjects	= c["maxMovingObjects"].as<int>();
	enableFX 			= c["enableFX"].as<bool>();
	sandboxMode			= c["sandboxMode"].as<bool>();

	accelVertical	= normalAccelVertical;
	movementSpeed 	= normalMovementSpeed;
	personSize 		= personSizeNormal;

	if(!loadPosition()){
		position.x = 0.01;
		position.y = 0.01;
		position.z = 9.7;
		position.orientationHorizontal = 0.0;
		position.orientationVertical = 0.0;
	}
	
	pointingOnBlock = position.block();
	pointingOnPlane = DIRECTION_DOWN;
	
	if(!loadInventory()){
		for(int i = 1; i < NUMBER_OF_MATERIALS; i++){
			inventory[i] = 0;
		}
	}
}

void Movement::init()
{
//	step = Mix_LoadWAV((workingDirectory + "/sound/fx/Footstep-stereo.ogg").c_str());
	//putBlock = Mix_LoadWAV((workingDirectory + "/sound/fx/nutfall.wav").c_str());
	
	
	fs::path filename = fs::path("sound") / "fx" / "nutfall.wav";
	//load and start music
	if((putBlock = Mix_LoadWAV((dataDirectory / filename).string().c_str()))||
		(putBlock = Mix_LoadWAV((workingDirectory / filename).string().c_str())) ||
		(putBlock = Mix_LoadWAV((localDirectory / filename).string().c_str())) ||
		(putBlock = Mix_LoadWAV((filename).string().c_str())) 
	) {
	} else {
		std::cout << "Could not find the sound file " << filename <<  std::endl;
	}

	//Bullet Physics
	std::cout << "init phys..." <<  std::endl;
	initPhysics();
	std::cout << "init done" <<  std::endl;
}

void	Movement::initPhysics(){
	collisionConfiguration = new btDefaultCollisionConfiguration();
	
	dispatcher = new	btCollisionDispatcher(collisionConfiguration);
	
	overlappingPairCache = new btDbvtBroadphase();
	overlappingPairCache->getOverlappingPairCache()->setInternalGhostPairCallback(new btGhostPairCallback());
	
	solver = new btSequentialImpulseConstraintSolver;
	
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,overlappingPairCache,solver,collisionConfiguration);
	dynamicsWorld->setDebugDrawer(&debugDrawer);
	dynamicsWorld->setGravity(btVector3(0,0,-5));

	//Character initialization
	ghost = new btPairCachingGhostObject();
	btConvexShape* cShape = new btCylinderShapeZ(btVector3(offset,0,personSizeNormal/2));
	cShape->setMargin(0.1f);
	//btConvexShape* cShape = colShape;
	
	ghost->setCollisionShape (cShape);
	ghost->setCollisionFlags (btCollisionObject::CF_CHARACTER_OBJECT);
	
	btTransform trans;
	trans.setIdentity ();
	trans.setOrigin(btVector3(position.x,position.y,position.z));
	ghost->setWorldTransform(trans);
	
	//ghost->setCcdMotionThreshold(1.0f);
	//ghost->setCcdSweptSphereRadius(0.2f); 
	
	kinCon = new btKinematicCharacterController(ghost, cShape, 0.3, 2);
	//kinCon->setFallSpeed(1);
	kinCon->setMaxJumpHeight(1.1);

	dynamicsWorld->addCollisionObject(ghost, btBroadphaseProxy::CharacterFilter, btBroadphaseProxy::StaticFilter|btBroadphaseProxy::DefaultFilter);
	
	dynamicsWorld->addAction(kinCon);
	
}

void Movement::calcPhysics(){ 
	dynamicsWorld->stepSimulation(time/1000.,5);
/*	if (body && body->getMotionState())
	{
		btTransform trans;
		body->getMotionState()->getWorldTransform(trans);
		c->renderer->itemPos.x = trans.getOrigin().getX();
		c->renderer->itemPos.y = trans.getOrigin().getY();
		c->renderer->itemPos.z = trans.getOrigin().getZ();
		
		c->renderer->itemPos.rotate = trans.getRotation();
		//printf("item pos = %f,%f,%f\n",float(trans.getOrigin().getX()),float(trans.getOrigin().getY()),float(trans.getOrigin().getZ()));
	}*/

	btTransform trans = ghost->getWorldTransform();
//	printf("char pos = %f,%f,%f\n",float(trans.getOrigin().getX()),float(trans.getOrigin().getY()),float(trans.getOrigin().getZ()));
	
	
	position.x = trans.getOrigin().getX();
	position.y = trans.getOrigin().getY();
	position.z = trans.getOrigin().getZ()-personSizeNormal/2+personSize;
}

void Movement::calcCharacter()
{
	btTransform xform;
	xform = ghost->getWorldTransform ();

	//btVector3 switchY(0,-1,0);
	
	btVector3 forwardDir = xform.getBasis()[0];
	btVector3 leftDir = xform.getBasis()[1];
	forwardDir.setY(-forwardDir.getY());
	leftDir.setY(-leftDir.getY());
	btVector3 upDir = xform.getBasis()[2];
	forwardDir.normalize ();
	upDir.normalize ();
	leftDir.normalize ();

	btVector3 walkDirection = btVector3(0.0, 0.0, 0.0);
	
	//rotate view
	if (leftPressed || rightPressed)
	{
		walkDirection -= leftDir*speedRight;
	}
	
	if (forwardPressed || backwardsPressed){
		walkDirection += forwardDir*speedForward;
	}

	if (jumpPressed)
		kinCon->jump();
	
	kinCon->setWalkDirection(walkDirection/2);
}

void Movement::savePosition() {
	std::ofstream of;
	of.open((workingDirectory / "position.dat").string().c_str());
	
	of << position.x << std::endl << position.y << std::endl << position.z << std::endl;
	of << position.orientationHorizontal << std::endl << position.orientationVertical << std::endl;
	
	of.close();
}
	
bool Movement::loadPosition()
{
	std::ifstream i;
	bool success = false;
	i.open((workingDirectory / "position.dat").string().c_str());
	if (i.is_open()) {
		i >> position.x >> position.y >> position.z >> position.orientationHorizontal >> position.orientationVertical;
		success = true;
	}
	i.close();

	return success;
}


void Movement::saveInventory() {
	std::ofstream of;
	of.open((workingDirectory / "inventory.dat").string().c_str());
	
	for(int i = 1; i < NUMBER_OF_MATERIALS; i++){
		of << inventory[i] << std::endl;
	}
	
	of.close();
}

bool Movement::loadInventory()
{
	std::ifstream i;
	bool success = false;
	i.open((workingDirectory / "inventory.dat").string().c_str());
	if (i.is_open()) {
		for(int j = 1; j < NUMBER_OF_MATERIALS; j++){
			i >> inventory[j];
		}
		success = true;
		selectedMaterial = getNextAvailableMaterial(selectedMaterial);
	}
	i.close();
	
	return success;
}

void Movement::performAction(ActionEvent event)
{
	btMatrix3x3 orn;
	btTransform trans;
	
	switch(event.name){
		case ActionEvent::PRESS_FORWARD:
			forwardPressed = true;
			break;
		case ActionEvent::RELEASE_FORWARD:
			forwardPressed = false;
			//Steps
			if(!duckPressed)
				personSize = personSizeNormal;
			else
				personSize = personSizeDucked;
			break;
			
		case ActionEvent::PRESS_BACKWARDS:
			backwardsPressed = true;
			break;
		case ActionEvent::RELEASE_BACKWARDS:
			backwardsPressed = false;
			//Steps
			if(!duckPressed)
				personSize = personSizeNormal;
			else
				personSize = personSizeDucked;
			break;
			
		case ActionEvent::PRESS_LEFT:
			leftPressed = true;
			break;
		case ActionEvent::RELEASE_LEFT:
			leftPressed = false;
			//Steps
			if(!duckPressed)
				personSize = personSizeNormal;
			else
				personSize = personSizeDucked;
			break;
			
		case ActionEvent::PRESS_RIGHT:
			rightPressed = true;
			break;
		case ActionEvent::RELEASE_RIGHT:
			rightPressed = false;
			//Steps
			if(!duckPressed)
				personSize = personSizeNormal;
			else
				personSize = personSizeDucked;
			break;
			
		case ActionEvent::PRESS_JUMP:
			kinCon->jump();
			jumpPressed = true;
			break;
		case ActionEvent::RELEASE_JUMP:
			jumpPressed = false;
			break;

		case ActionEvent::PRESS_FAST_SPEED:
			if(sandboxMode){
				fastPressed = true;
				movementSpeed *= fastSpeedMultiplier;
				dynamicsWorld->removeAction(kinCon);
				kinCon->setMaxJumpHeight(20.1);
				dynamicsWorld->addAction(kinCon);
			}
			break;
		case ActionEvent::RELEASE_FAST_SPEED:
			if(sandboxMode){
				fastPressed = false;
				movementSpeed /= fastSpeedMultiplier;
				kinCon->setMaxJumpHeight(1.1);
			}
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
			calcPointingOn();
			if(isPointingOn && buildBlockPressed){
				buildBlock();
				lastBuild = 0;
			}
			break;
		case ActionEvent::RELEASE_BUILD_BLOCK:
			buildBlockPressed = false;
			break;
			
		case ActionEvent::PRESS_REMOVE_BLOCK:
			removeBlockPressed = true;
			calcPointingOn();
			if(isPointingOn && removeBlockPressed){
				lastRemove = 0;
			}
			break;
		case ActionEvent::RELEASE_REMOVE_BLOCK:
			removeBlockPressed = false;
			break;

		case ActionEvent::PRESS_THROW_BLOCK:
			if(sandboxMode){
				lastThrow = 8;
				throwPressed = true;
			}
			break;
		case ActionEvent::RELEASE_THROW_BLOCK:
			throwPressed = false;
			break;

		case ActionEvent::ROTATE_HORIZONTAL:
			//std::cout << "r_hor: " << event.value << std::endl;
			position.orientationHorizontal += event.value;
			if(position.orientationHorizontal > 360)
				position.orientationHorizontal -= 360;
			if(position.orientationHorizontal < 0)
				position.orientationHorizontal += 360;
			trans = ghost->getWorldTransform();
			trans.setRotation(btQuaternion(btVector3(0,0,1),position.orientationHorizontal*(M_PI/180)));
			ghost->setWorldTransform(trans);
			break;
		case ActionEvent::ROTATE_VERTICAL:
			//std::cout << "r_ver: " << event.value << std::endl;
			position.orientationVertical += event.value;
			if(position.orientationVertical > 90)
				position.orientationVertical = 90;
			if(position.orientationVertical < -90)
				position.orientationVertical = -90;
			break;
		case ActionEvent::PRESS_FLY:
			if(sandboxMode){
				if(enableFly){
					enableFly = false;
					accelVertical = normalAccelVertical;
				}
				else{
					enableFly = true;
					accelVertical = 0.0;
				}
			}
			break;
		case ActionEvent::SELECT_MATERIAL:
			if(sandboxMode || getCountInInventory(event.iValue) > 0) {
				selectedMaterial = Material(event.iValue);
			}
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

Material Movement::getSelectedMaterial(){
	return selectedMaterial;
}

bool Movement::getPointingOn(BlockPosition* block, DIRECTION* plane){
	if(isPointingOn){
		*block = pointingOnBlock;
		*plane = pointingOnPlane;
	}
	return isPointingOn;
}

void Movement::calcDuckingAndSteps(){
	//Ducking
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
	//Steps
	if((rightPressed || leftPressed || forwardPressed || backwardsPressed) && kinCon->onGround() && !duckPressed){
		stepProgress++;
		if(stepProgress > 10){
			stepProgress = 1;
			//Mix_PlayChannel(-1, step, 0);
		}
		double stepChange = fabs(sin((M_PI/10)*stepProgress)*0.04);
		personSize = personSizeNormal+stepChange;
	}
}

void Movement::calcPointingOn(){
	lastPointingOnBlock = pointingOnBlock;
	lastPointingOnPlane = pointingOnPlane;
	PlayerPosition lastPos = position;
	pointingOnBlock = position.block();
	BlockPosition internalLastPointingOnBlock = pointingOnBlock;
	
	double distanceQ = 0;
	int counter = 0;
	try{
		while((c->map->getBlock(pointingOnBlock) == 0 || c->map->getBlock(pointingOnBlock) == c->map->getBlock(position.block())) && distanceQ <= pointingDistance*pointingDistance && counter <= pointingDistance*3+2){
			counter++;
			internalLastPointingOnBlock = pointingOnBlock;
			
			pointingOnPlane = calcPointingOnInBlock(&lastPos, pointingOnBlock);
			pointingOnBlock = pointingOnBlock + pointingOnPlane;

			double dx = lastPos.x - position.x;
			double dy = lastPos.y - position.y;
			double dz = lastPos.z - position.z;

			distanceQ = dx*dx + dy*dy + dz*dz;

		}

		if(c->map->getBlock(pointingOnBlock) == 0 || (c->map->getBlock(pointingOnBlock) == 9 && c->map->getBlock(position.block())))
			isPointingOn = false;
		else
			isPointingOn = true;

		pointingOnBlock = internalLastPointingOnBlock;
	} catch(NotLoadedException e){
		isPointingOn = false;
	}
}

//Berechnet die Fläche, auf die von der Startposition aus (Parameter) mit der aktuellen Blickrichtung gezeigt wird
//@return: ID der Fläche, auf die man zeigt
//Am Ende sind die Parameter auf den Schnittpunkt gesetzt
DIRECTION Movement::calcPointingOnInBlock(PlayerPosition* posIn, BlockPosition blockIn){
	
	Matrix<double,3,3> left(0);
	Matrix<double,1,3> right(0);
	Matrix<double,1,3> result(0);
	
	//bleibt immer gleich (Blickrichtung)
	left.data[2][0] = -cos(M_PI*posIn->orientationHorizontal/180.) * cos(M_PI*posIn->orientationVertical/180.);
	left.data[2][1] = -sin(M_PI*posIn->orientationHorizontal/180.) * cos(M_PI*posIn->orientationVertical/180.);
	left.data[2][2] = -sin(M_PI*posIn->orientationVertical/180.);

	for(int i = 0; i < DIRECTION_COUNT; i++){
		left.data[0][0] = POINTS_OF_DIRECTION[i][1][0] - POINTS_OF_DIRECTION[i][0][0];
		left.data[0][1] = POINTS_OF_DIRECTION[i][1][1] - POINTS_OF_DIRECTION[i][0][1];
		left.data[0][2] = POINTS_OF_DIRECTION[i][1][2] - POINTS_OF_DIRECTION[i][0][2];
		left.data[1][0] = POINTS_OF_DIRECTION[i][3][0] - POINTS_OF_DIRECTION[i][0][0];
		left.data[1][1] = POINTS_OF_DIRECTION[i][3][1] - POINTS_OF_DIRECTION[i][0][1];
		left.data[1][2] = POINTS_OF_DIRECTION[i][3][2] - POINTS_OF_DIRECTION[i][0][2];
		right.data[0][0] = posIn->x - blockIn.x - POINTS_OF_DIRECTION[i][0][0];
		right.data[0][1] = posIn->y - blockIn.y - POINTS_OF_DIRECTION[i][0][1];
		right.data[0][2] = posIn->z - blockIn.z - POINTS_OF_DIRECTION[i][0][2];
		result = left.LU().solve(right);
		if( 0 <= result.data[0][0] && result.data[0][0] <= 1
			&& 0 <= result.data[0][1] && result.data[0][1] <= 1
			&& 0 < result.data[0][2])
		{
			posIn->x = blockIn.x + POINTS_OF_DIRECTION[i][0][0]
						+ result[0][0] * left.data[0][0]
						+ result[0][1] * left.data[1][0];
			posIn->y = blockIn.y + POINTS_OF_DIRECTION[i][0][1]
						+ result[0][0] * left.data[0][1]
						+ result[0][1] * left.data[1][1];
			posIn->z = blockIn.z + POINTS_OF_DIRECTION[i][0][2]
						+ result[0][0] * left.data[0][2]
						+ result[0][1] * left.data[1][2];

			return (DIRECTION)i;
		}
	}
	return (DIRECTION)0;
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
}

void Movement::calcBuilding(){
	if(isPointingOn && buildBlockPressed){
		lastBuild++;
		if(lastBuild >= 10 || fastPressed){
			buildBlock();
			lastBuild = 0;
		}
	}
	
	if(isPointingOn && removeBlockPressed){
		if(lastPointingOnBlock+lastPointingOnPlane != pointingOnBlock+pointingOnPlane)
			lastRemove = 0;
		lastRemove++;
		if(lastRemove >= normalMaxRemove || fastPressed){
			removeBlock();
			lastRemove = 0;
		}
	}
}

int Movement::getCurrentRemoveProgress() {
	if(isPointingOn && removeBlockPressed)
		return (float(lastRemove)/float(normalMaxRemove))*100;
	else
		return 0;
}

void Movement::buildBlock()
{
	PlayerPosition pos[18];
	pos[0] = position;
	pos[0].z -= personSize;
	pos[1] = pos[0];
	pos[1].x += offset;
	pos[2] = pos[0];
	pos[2].x -= offset;
	pos[3] = pos[0];
	pos[3].y += offset;
	pos[4] = pos[0];
	pos[4].y -= offset;
	pos[5] = pos[0];
	pos[5].x += offset/2;
	pos[5].y += offset/2;
	pos[6] = pos[0];
	pos[6].x += offset/2;
	pos[6].y -= offset/2;
	pos[7] = pos[0];
	pos[7].x -= offset/2;
	pos[7].y += offset/2;
	pos[8] = pos[0];
	pos[8].x -= offset/2;
	pos[8].y -= offset/2;
	
	pos[9] = position;
	pos[10] = pos[9];
	pos[10].x += offset;
	pos[11] = pos[9];
	pos[11].x -= offset;
	pos[12] = pos[9];
	pos[12].y += offset;
	pos[13] = pos[9];
	pos[13].y -= offset;
	pos[14] = pos[9];
	pos[14].x += offset/2;
	pos[14].y += offset/2;
	pos[15] = pos[9];
	pos[15].x += offset/2;
	pos[15].y -= offset/2;
	pos[16] = pos[9];
	pos[16].x -= offset/2;
	pos[16].y += offset/2;
	pos[17] = pos[9];
	pos[17].x -= offset/2;
	pos[17].y -= offset/2;
	bool noBuild = false;
	for(int i = 0; i < 18; i++){
		if(pos[i].block() == pointingOnBlock)
			noBuild = true;
	}
	if(!sandboxMode)
		if(inventory[selectedMaterial] == 0)
			noBuild = true;
	if(!noBuild){
		try{
			if(!sandboxMode){
				inventory[selectedMaterial]--;
				c->map->setBlock(pointingOnBlock, selectedMaterial);
				
				if(inventory[selectedMaterial] <= 0){
					inventory[selectedMaterial] = 0;
					selectedMaterial = getNextAvailableMaterial(selectedMaterial);
				}
			}
			else
				c->map->setBlock(pointingOnBlock, selectedMaterial);
			if(enableFX)
				Mix_PlayChannel(-1, putBlock, 0);
		}
		catch(NotLoadedException e){
			if(movDebug)
				std::cout << "NotLoadedException buildBlock" << std::endl;
		}
	}
}

void movableNearCallback(btBroadphasePair& collisionPair,
						  btCollisionDispatcher& dp, const btDispatcherInfo& dispatchInfo) {
	
	dp.defaultNearCallback(collisionPair, dp, dispatchInfo);
}

void Movement::removeBlock()
{
	BlockPosition block = pointingOnBlock+pointingOnPlane;

	if(!sandboxMode){
		//Update inventory
		inventory[c->map->getBlock(block)]++;
		if(inventory[selectedMaterial] == 0)
			selectedMaterial = getNextAvailableMaterial(selectedMaterial);
		
		//Create small movable
		/*btConvexShape* s = new btBoxShape (btVector3(0.1,0.1,0.1));
		btVector3 localInertia(0,0,0);
		s->calculateLocalInertia(0.01,localInertia);
		btTransform t = btTransform::getIdentity();
		t.setOrigin(btVector3(block.x+0.375,block.y+0.375,block.z+0.375));
		t.setRotation(btQuaternion(btVector3(0,0,1),position.orientationHorizontal*(M_PI/180)));
		btRigidBody::btRigidBodyConstructionInfo rbInfo(0.01,new btDefaultMotionState(t),s,localInertia);
		MovingObject* o = new MovingObject(rbInfo);
		o->applyCentralImpulse(btVector3(sin((rand()%360-180)*(M_PI/180))*0.005,
										sin((rand()%360-180)*(M_PI/180))*0.005,
										sin(rand()%90*(M_PI/180))*0.005
		));
		o->tex = c->map->getBlock(block);
		
		dispatcher->setNearCallback(&movableNearCallback);
		c->map->objects.push_back(o);
		dynamicsWorld->addRigidBody(o);

		if(c->map->objects.size() > maxMovingObjects) {
			o = c->map->objects.front();
			dynamicsWorld->removeRigidBody(o);
			delete o;
			c->map->objects.pop_front();
		}*/
	}

	try{
		c->map->setBlock(block, 0);
	}
	catch(NotLoadedException e){
		if(movDebug)
			std::cout << "NotLoadedException removeBlock" << std::endl;
	}
}

void Movement::throwBlock(){
	btConvexShape* s = new btBoxShape (btVector3(0.1,0.1,0.1));
	btVector3 localInertia(0,0,0);
	s->calculateLocalInertia(0.01,localInertia);
	btTransform t = btTransform::getIdentity();
	t.setOrigin(btVector3(
						  position.x+cos(position.orientationHorizontal*(M_PI/180))*cos(position.orientationVertical*(M_PI/180))*1.3,
						  position.y+sin(position.orientationHorizontal*(M_PI/180))*cos(position.orientationVertical*(M_PI/180))*1.3,
						  position.z+sin(position.orientationVertical*(M_PI/180))*1.3
						 )
			   );
	t.setRotation(
		btQuaternion(btVector3(0,0,1),position.orientationHorizontal*(M_PI/180)) * 
		btQuaternion(btVector3(0,1,0),-position.orientationVertical*(M_PI/180))
		);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(0.01,new btDefaultMotionState(t),s,localInertia);
	MovingObject* o = new MovingObject(rbInfo);
	o->applyCentralImpulse(btVector3(cos(position.orientationHorizontal*(M_PI/180))*cos(position.orientationVertical*(M_PI/180))*0.3,
									 sin(position.orientationHorizontal*(M_PI/180))*cos(position.orientationVertical*(M_PI/180))*0.3,
									 sin(position.orientationVertical*(M_PI/180))*0.3
									));
	o->tex = selectedMaterial;
	
	o->setCcdMotionThreshold(0.1f);
	o->setCcdSweptSphereRadius(0.02f); 
	
	c->map->objects.push_back(o);
	dynamicsWorld->addRigidBody(o);
	
	if(c->map->objects.size() > maxMovingObjects) {
		o = c->map->objects.front();
		dynamicsWorld->removeRigidBody(o);
		delete o;
		c->map->objects.pop_front();
	}
}

void Movement::calcElevator()
{	
	int feetBlock = 1;
	try{
		feetBlock = c->map->getBlock(position.block());
	}
	catch(NotLoadedException e){
		std::cout << "not loaded" << std::endl;
	}
	//0 = air, 9 = water
	if(feetBlock != 0 && feetBlock != 9){
		btTransform t = ghost->getWorldTransform();
		btVector3 org = t.getOrigin();
		org.setZ(org.getZ()+1);
		t.setOrigin(org);
		ghost->setWorldTransform(t);
	}
}

void Movement::triggerNextFrame(int time){
	if(time)
		this->time = time;
	else
		this->time = 1;
	
	bool calc = 1;
	try {
		if(!c->map->getArea(position.block())->bullet_generated && c->map->getBlock(position.block())) {
			calc = 0;
			std::cout << "bullet not generated" << std::endl;
		} 
	} catch(NotLoadedException) {
		std::cout << "area not loaded" << std::endl;
		calc = 0;
	} catch(AreaEmptyException) {}
	
	if(calc) {
		calcNewSpeed();
		calcDuckingAndSteps();
		calcCharacter();
		calcPhysics();
		lastThrow++;
		if(throwPressed && (lastThrow >= 8 || fastPressed)){
			lastThrow = 0;
			throwBlock();
		}
		/*calcCollisionAndMove();*/
		calcPointingOn();
		calcBuilding();
		calcElevator();
	}
}

int Movement::getCountInInventory(int mat){
	return inventory[mat];
}

int Movement::getNextAvailableMaterial(int startMat)
{
	do {
		startMat++;
		if(startMat >= NUMBER_OF_MATERIALS)
			startMat = 1;
		if(startMat == selectedMaterial)
			break;
	} while(c->movement->getCountInInventory(startMat) == 0 && !sandboxMode);
	
	return startMat;
}

int Movement::getLastAvailableMaterial(int startMat)
{
	do {
		startMat--;
		if(startMat < 1)
			startMat = NUMBER_OF_MATERIALS - 1;
		if(startMat == selectedMaterial)
			break;
	} while(c->movement->getCountInInventory(startMat) == 0 && !sandboxMode);
	
	return startMat;
}
