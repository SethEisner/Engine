#pragma once
#include <vector>
#include "RenderItem.h"

class Engine;

class Scene {
public:
	Scene();
	~Scene();
	bool init();
	void update();
	void shutdown();
private:
	std::vector<RenderItem>* m_items;
	// eventually have a scene graph
	// eventually have a BVH
};