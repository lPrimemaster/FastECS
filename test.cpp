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

class TransformSystem : public FCS::System, FCS::EventSubscriber<float>, FCS::EventSubscriber<int>
{
public:
	virtual void initialize(FCS::Scene* scene) override
	{
		FCS::EventSubscriber<float>::subscribe(scene);
		FCS::EventSubscriber<int>::subscribe(scene);
	}

	virtual void deinitialize(FCS::Scene* scene) override
	{
		FCS::EventSubscriber<float>::unsubscribe(scene);
		FCS::EventSubscriber<int>::unsubscribe(scene);
	}

	virtual void update(FCS::Scene* scene, float deltaTime) override
	{

	}

	virtual void onEvent(FCS::Scene* scene, const float& event) override
	{
		std::cout << "Recieved float evt" << std::endl;
	}

	virtual void onEvent(FCS::Scene* scene, const int& event) override
	{
		std::cout << "Recieved int evt" << std::endl;
	}
};

int main(int argc, char* argv[])
{
	FCS::Scene scene = FCS::Scene::Create();

	scene.createSystem<TransformSystem>();
	scene.emit<float>(9);
	scene.emit<int>(9);

	scene.deleteSystem<TransformSystem>();

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