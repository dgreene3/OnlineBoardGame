#pragma once

class btVector3;
class Entity;

class Asset {
private:
	Asset();
protected:
	int assetPackIndex;
	int group;
	float mass;
	btVector3 centerOfMass;
public:
	inline float getMass() {return mass;}
	inline int getGroup() {return group;}
	Asset(int assetPackIndex, int group) :
		assetPackIndex(assetPackIndex),
		group(group)
	{}

	virtual Entity *createEntity(const btVector3 &position, int id);
	
	virtual ~Asset();
};
