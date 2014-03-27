#include <iostream>
#include "EntityManager.h"
#include "Entity.h"
#include "Asset.h"
#include "Interaction.h"
#include "PhysicsUpdate.h"
	
EntityManager::EntityManager() {
	//Set up the physics world
	broadphase = new btDbvtBroadphase();
	collisionConfiguration = new btDefaultCollisionConfiguration;
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver;
    world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);

	world->setGravity(btVector3(0.0f, -10.0f, 0.0f));

	btCollisionShape *groundShape = new btStaticPlaneShape(btVector3(-1.1, 1, 0.9), -2);
	btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,0,0)));
    btRigidBody::btRigidBodyConstructionInfo
            groundRigidBodyCI(0,groundMotionState,groundShape,btVector3(0,0,0));
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
    world->addRigidBody(groundRigidBody);

	btCollisionShape *groundShape2 = new btStaticPlaneShape(btVector3(0.9, 1, -1.1), -2);
	btDefaultMotionState* groundMotionState2 = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
    btRigidBody::btRigidBodyConstructionInfo
            groundRigidBodyCI2(0,groundMotionState2,groundShape2,btVector3(0,0,0));
    btRigidBody* groundRigidBody2 = new btRigidBody(groundRigidBodyCI2);
    world->addRigidBody(groundRigidBody2);


 //   btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0,1,0),1);

 //   btCollisionShape* fallShape = new btSphereShape(1);

	//btDefaultMotionState* groundMotionState = new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,-1,0)));
 //   btRigidBody::btRigidBodyConstructionInfo
 //           groundRigidBodyCI(0,groundMotionState,groundShape,btVector3(0,0,0));
 //   btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);
 //   world->addRigidBody(groundRigidBody);


 //   btDefaultMotionState* fallMotionState =
 //           new btDefaultMotionState(btTransform(btQuaternion(0,0,0,1),btVector3(0,50,0)));
 //   btScalar mass = 1;
 //   btVector3 fallInertia(0,0,0);
 //   fallShape->calculateLocalInertia(mass,fallInertia);
 //   btRigidBody::btRigidBodyConstructionInfo fallRigidBodyCI(mass,fallMotionState,fallShape,fallInertia);
 //   btRigidBody* fallRigidBody = new btRigidBody(fallRigidBodyCI);
 //   world->addRigidBody(fallRigidBody);


 //   for (int i=0 ; i<300 ; i++) {
 //           world->stepSimulation(1/60.f,10);

 //           btTransform trans;
 //           fallRigidBody->getMotionState()->getWorldTransform(trans);

 //           std::cout << "sphere height: " << trans.getOrigin().getY() << std::endl;
 //   }

 //   world->removeRigidBody(fallRigidBody);
 //   delete fallRigidBody->getMotionState();
 //   delete fallRigidBody;

 //   world->removeRigidBody(groundRigidBody);
 //   delete groundRigidBody->getMotionState();
 //   delete groundRigidBody;


 //   delete fallShape;

 //   delete groundShape;
}

EntityManager::~EntityManager() {

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
	entities[e->getId()] = e;
	world->addRigidBody(e->getPhysicsBody());
}

void EntityManager::handleInteraction(Interaction *action) {
	for(int &i : action->ids) {
		if(i < 0) {
			int id = -i;
			btRigidBody& physBody = *entities[id]->getPhysicsBody();
			physBody.setGravity(btVector3(0, -1, 0));
			physBody.setAngularFactor(btVector3(1, 1, 1));
		} else {
			btRigidBody& physBody = *entities[i]->getPhysicsBody();
			physBody.setGravity(action->mousePos);
			physBody.setAngularFactor(btVector3(0, 0, 0));
		}
	}
}

void EntityManager::handlePhysicsUpdate(PhysicsUpdate *update) {
	Entity* ent = entities[update->entityId];
	
	btRigidBody& physBody = *ent->getPhysicsBody();
	
	physBody.getMotionState()->setWorldTransform(update->transform);
	physBody.setLinearVelocity(update->linearVel);
	physBody.setAngularVelocity(update->angularVel);
}

void EntityManager::start() {
	lastTime = clock();
}

void EntityManager::update() {
	//Step physics simulation

	clock_t currTime = clock();
	world->stepSimulation(float(currTime - lastTime)/float(CLOCKS_PER_SEC), 10);
	lastTime = currTime;
	
	//Combine stackable entities
	for(unsigned int i = 0; i < entities.size(); i ++) {
		for(unsigned int j = i + 1; j < entities.size(); j ++) {
			Entity *ent1 = entities.at(i);
			Entity *ent2 = entities.at(j);

			if(ent1->getType()->getGroup() == ent2->getType()->getGroup()) { //&& similar positions && stackable) {
				//stack them
			}
		}
	}
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

Entity* EntityManager::getIntersectingEntity(const btVector3& from, const btVector3& to) {
	btCollisionWorld::ClosestRayResultCallback callback(from, to);
	world->rayTest(from, to, callback);

	if(callback.hasHit()) {
		const btCollisionObject *collided = callback.m_collisionObject;
		for(pair<int, Entity *> e : entities) {
			if(e.second->getPhysicsBody() == collided) {
				return e.second;
			}
		}
	}
	return NULL;
}

void EntityManager::createPhysicsUpdates() {
	for(pair<int, Entity*> p : entities) {
		Entity * e = p.second;
		btRigidBody& physBody = * e->getPhysicsBody();
		btTransform transform;
		physBody.getMotionState()->getWorldTransform(transform);
		btVector3 linear = physBody.getLinearVelocity();
		btVector3 angular = physBody.getAngularVelocity();
		PhysicsUpdate *update = PhysicsUpdate::create(e->getId(), transform, linear, angular);

		firePhysicsUpdate(update);

		update->emancipate();
	}
}