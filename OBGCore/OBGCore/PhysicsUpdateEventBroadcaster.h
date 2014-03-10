#pragma once
#include <vector>

class PhysicsUpdateListener;
class PhysicsUpdate;

class PhysicsUpdateEventBroadcaster {
private:
	std::vector<PhysicsUpdateListener *> listeners;

protected:
	void fire(PhysicsUpdate *update);

public:
	PhysicsUpdateEventBroadcaster() {};
	virtual void registerPhysicsUpdateListener(PhysicsUpdateListener *listener);
	virtual ~PhysicsUpdateEventBroadcaster() {};
};
