// Test file
#include <iostream>
#include "FastECS.h"

class Transform : public FCS::Component
{
public:
	//Transform() { }
	float x, y, z;

	FCS_COMPONENT(Transform);
};


int main(int argc, char* argv[])
{
	FCS::Scene scene = FCS::Scene::Create();

	FCS::Handle<FCS::Entity> eh1 = scene.instantiate();

	FCS::Handle<Transform> t = eh1->addComponent<Transform>();
	t->x = 0;
	t->y = 0;
	t->z = 0;

	FCS::Handle<FCS::Entity> eh2 = scene.instantiate(eh1);
	auto t1 = eh2->getComponent<Transform>();
	t1->x = 2;
	t1->y = 2;
	t1->z = 2;

	return 0;
}