#include <assert.h>
#include <iostream>
#include "Constants.h"
#include "EntityManager.h"
#include "Entity.h"
#include "Asset.h"
#include "Interaction.h"
#include "PhysicsUpdate.h"
#include "ShakeStrategy.h"

	
#define MAX_PICKUP_VELOCITY 5*BOARD_SIZE
#define PICKUP_POWER 10
#define ROTATION_SPEED 1.5

using namespace std;

class ClosestNotInListCallback :
	public btCollisionWorld::RayResultCallback
{
public:
	btScalar closest;
	vector<int> heldList;
		
	ClosestNotInListCallback(vector<int> exclude)
		:heldList(exclude), closest(5)
	{}

	virtual	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) {
		btRigidBody *collisionObject = (btRigidBody *)rayResult.m_collisionObject;
		Entity *entity = (Entity *)collisionObject->getMotionState();
		if(rayResult.m_hitFraction < closest && find(heldList.begin(), heldList.end(), entity->getId()) == heldList.end()) {
			m_collisionObject = rayResult.m_collisionObject;
			closest = rayResult.m_hitFraction;
		}

		return rayResult.m_hitFraction;
	}
};

class AllNotInListCallback :
	public btCollisionWorld::RayResultCallback
{
public:
	vector<int> heldList;
	vector<Entity *> intersected;
		
	AllNotInListCallback(vector<int> exclude)
		:heldList(exclude)
	{}

	virtual	btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult, bool normalInWorldSpace) {
		btRigidBody *collisionObject = (btRigidBody *)rayResult.m_collisionObject;
		Entity *entity = (Entity *)collisionObject->getMotionState();
		if(find(heldList.begin(), heldList.end(), entity->getId()) == heldList.end()) {
			m_collisionObject = rayResult.m_collisionObject;
			intersected.push_back(entity);
		}
		return rayResult.m_hitFraction;
	}
};

EntityManager::EntityManager() {

	groundEnt = new Entity(NULL, -1, btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)), ShakeStrategy::defaultShakeStrategy);
	//Set up the physics world
	broadphase = new btDbvtBroadphase();
	collisionConfiguration = new btDefaultCollisionConfiguration;
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver;
	btCompoundShape *walls = new btCompoundShape();
	//floor
	walls->addChildShape(btTransform(btQuaternion(0,0,0,1), btVector3(0,-BOARD_SIZE,0)),
					 new btBoxShape(btVector3(2*BOARD_SIZE, BOARD_SIZE, 2*BOARD_SIZE)));
	//left wall
	walls->addChildShape(btTransform(btQuaternion(0,0,0,1), btVector3(-2*BOARD_SIZE,0,0)),
					 new btBoxShape(btVector3(BOARD_SIZE, 2*BOARD_SIZE, 2*BOARD_SIZE)));
	//right wall
	walls->addChildShape(btTransform(btQuaternion(0,0,0,1), btVector3(2*BOARD_SIZE,0,0)),
					 new btBoxShape(btVector3(BOARD_SIZE, 2*BOARD_SIZE, 2*BOARD_SIZE)));
	//far wall
	walls->addChildShape(btTransform(btQuaternion(0,0,0,1), btVector3(0,0,-2*BOARD_SIZE)),
					 new btBoxShape(btVector3(2*BOARD_SIZE, 2*BOARD_SIZE, BOARD_SIZE)));
	//near wall
	walls->addChildShape(btTransform(btQuaternion(0,0,0,1), btVector3(0,0,2*BOARD_SIZE)),
					 new btBoxShape(btVector3(2*BOARD_SIZE, 2*BOARD_SIZE, BOARD_SIZE)));
	//prepare for entry into world
    btRigidBody::btRigidBodyConstructionInfo
            wallsRBCI(0,groundEnt,walls,btVector3(0,0,0));

	//take lock to modify world
	FunctionLock lock(worldLock);
	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
	world->getDispatchInfo().m_useContinuous=true;
	world->setGravity(btVector3(0.0f, -50.0f, 0.0f));
	world->addRigidBody(new btRigidBody(wallsRBCI));
}

EntityManager::~EntityManager() {
	FunctionLock lock(worldLock);
	//Remove the objects from the physics world
	for(pair<int, Entity*> e : entities) {
		world->removeRigidBody(e.second->getPhysicsBody());
	}

	//Delete the physics world and all associated things in it.
	delete world;
    delete solver;
    delete collisionConfiguration;
    delete dispatcher;
    delete broadphase;
}

void EntityManager::addEntity(Entity *e) {
	FunctionLock lock(worldLock);
	e->getPhysicsBody()->setCcdMotionThreshold(0.05f);
	e->getPhysicsBody()->setCcdSweptSphereRadius(0.2f);
	entities[e->getId()] = e;
	world->addRigidBody(e->getPhysicsBody());
}

void EntityManager::handleInteraction(Interaction *action) {
	FunctionLock lock(worldLock);
	for(int &i : action->ids) {
		assert(entities.find(abs(i)) != entities.end());
		if(i < 0) {
			btRigidBody& physBody = *entities[-i]->getPhysicsBody();
			physBody.setGravity(world->getGravity());
			physBody.setAngularFactor(btVector3(1, 1, 1));
			physBody.activate(true);
		} else {
			btRigidBody& physBody = *entities[i]->getPhysicsBody();
			btVector3 v = action->mousePos - physBody.getWorldTransform().getOrigin();
			v *= PICKUP_POWER;
			if (v.length() > MAX_PICKUP_VELOCITY) {
				v = MAX_PICKUP_VELOCITY * v.normalized();
			}
			physBody.setLinearVelocity(v);
			physBody.setGravity(btVector3(0,0,0));
			btVector3 rotations = btVector3(0.0, 0.0, 0.0);
			if(action->flags & ROT_POS_X) {
				rotations += btVector3(ROTATION_SPEED, 0.0, 0.0);
			}
			if(action->flags & ROT_NEG_X) {
				rotations += btVector3(-ROTATION_SPEED, 0.0, 0.0);
			}
			if(action->flags & ROT_POS_Y) {
				rotations += btVector3(0.0, ROTATION_SPEED, 0.0);
			}
			if(action->flags & ROT_NEG_Y) {
				rotations += btVector3(0.0, -ROTATION_SPEED, 0.0);
			}
			if(action->flags & ROT_POS_Z) {
				rotations += btVector3(0.0, 0.0, ROTATION_SPEED);
			}
			if(action->flags & ROT_NEG_Z) {
				rotations += btVector3(0.0, 0.0, -ROTATION_SPEED);
			}
			if(action->flags & SHAKING) {
				entities[i]->shake();
			}
			physBody.setAngularVelocity(rotations);
			physBody.activate(true);
		}
	}
}

void EntityManager::createPhysicsUpdates() {
	PhysicsUpdate update;
	FunctionLock lock(worldLock);
	for(pair<int, Entity*> p : entities) {
		Entity * e = p.second;
		btRigidBody& physBody = * e->getPhysicsBody();
		btTransform transform = physBody.getWorldTransform();
		btVector3 linear = physBody.getLinearVelocity();
		btVector3 angular = physBody.getAngularVelocity();
		bool moreRoom = update.addEntity(e->getId(), physBody.isActive(), transform, linear, angular);
		if (!moreRoom) {
			firePhysicsUpdate(&update);
			update.clear();
		}
	}
	if (update.size() > 0)
	firePhysicsUpdate(&update);
}

void EntityManager::handlePhysicsUpdate(PhysicsUpdate *physupdate) {
	FunctionLock lock(worldLock);
	for (EntityUpdate &update : *physupdate) {
		Entity* ent = entities[update.getEntityId()];
	
		btRigidBody& physBody = *ent->getPhysicsBody();
	
		physBody.setWorldTransform(update.getWorldTransform());
		physBody.setLinearVelocity(update.getLinearVelocity());
		physBody.setAngularVelocity(update.getAngularVelocity());
		if (update.isActive())
			physBody.activate(true);
	}
}

void EntityManager::start() {
	lastTime = clock();
}

void EntityManager::update(int time) {
	//Step physics simulation

	FunctionLock lock(worldLock);
	world->stepSimulation(float(time - lastTime)/float(CLOCKS_PER_SEC), 10, 0.005f);
	lock.unlock();
	lastTime = time;
	
	//Combine stackable entities
	//TODO:[JK] I commented this out the first time for a reason.
	//This is the WRONG WAY to iterate physics objects.
	//Use AABBs and the collision broadphase (or maybe even narrowphase) instead.
	//for(auto iter1 = entities.begin(), end = entities.end(); iter1 != end;) {
	//	Entity *ent1 = iter1->second;
	//	for(auto iter2 = ++iter1; iter2 != end; ++iter2) {
	//		Entity *ent2 = iter2->second;
	//
	//	}
	//}
}

void EntityManager::clear() {
	for(pair<int, Entity*> e : entities) {
		world->removeRigidBody(e.second->getPhysicsBody());	
	}
	entities.clear();
}

Entity* EntityManager::getEntityById(int id) {
	return entities[id];
}

Entity* EntityManager::getIntersectingEntity(const btVector3& from, const btVector3& to, vector<int> heldList) {
	ClosestNotInListCallback callback(heldList);
	FunctionLock lock(worldLock);
		world->rayTest(from, to, callback);
	lock.unlock();

	if(callback.hasHit()) {
		const btCollisionObject *collided = callback.m_collisionObject;
		if(((btRigidBody *)collided)->getMotionState() != groundEnt) {
			btQuaternion quat = btQuaternion();
			return (Entity *) ((btRigidBody *)collided)->getMotionState();
		}

	}
	return NULL;
}

vector<Entity *> EntityManager::getAllIntersectingEntities(const btVector3 &from, const btVector3 &to, vector<int> excludingIds) {
	AllNotInListCallback callback(excludingIds);
	FunctionLock lock(worldLock);
		world->rayTest(from, to, callback);
	lock.unlock();

	return callback.intersected;
}
