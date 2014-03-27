#pragma once
#include <InteractionEventBroadcaster.h>
#include "MessageEventBroadcaster.h"

class ChatBox;

class UserInputHandler :
	public MessageEventBroadcaster,
	public InteractionEventBroadcaster
{
private:
	ChatBox *chat;
	std::vector<int> heldList;
	std::vector<int> lastHeldList;
	double mouseX, mouseZ;
	int screenX, screenY;
public:
	UserInputHandler();
	virtual void update();
	virtual void keyPressed(unsigned char c, int x, int y);
	virtual void specialKeyPressed(int k, int x, int y);
	virtual void mousePressed(int button, int state, int x, int y);
	virtual void mouseDragged(int x, int y);
	virtual void mouseMoved(int x, int y);
	~UserInputHandler();

	inline ChatBox *getChatBox() { return chat; }
};
