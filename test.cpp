// Test file
#include <iostream>
#include "FastECS.h"

class Transform : public FCS::Component
{
public:
	float x, y, z;

	FCS_COMPONENT(Transform);
};

class TransformSystem : public FCS::System, FCS::EventSubscriber<FCS::Event::EntityCreated>, FCS::EventSubscriber<FCS::Event::EntityDestroyed>
{
public:
	virtual void initialize(FCS::Scene* scene) override
	{
		FCS::EventSubscriber<FCS::Event::EntityCreated>::subscribe(scene);
		FCS::EventSubscriber<FCS::Event::EntityDestroyed>::subscribe(scene);
	}

	virtual void deinitialize(FCS::Scene* scene) override
	{
		FCS::EventSubscriber<FCS::Event::EntityCreated>::unsubscribe(scene);
		FCS::EventSubscriber<FCS::Event::EntityDestroyed>::unsubscribe(scene);
	}

	virtual void update(FCS::Scene* scene, float deltaTime) override
	{
		
	}

	virtual void onEvent(FCS::Scene* scene, const FCS::Event::EntityCreated& event)
	{
		std::cout << "Entity " << typeid(event.entity).name() << " was created!" << std::endl;
	}

	virtual void onEvent(FCS::Scene* scene, const FCS::Event::EntityDestroyed& event)
	{
		std::cout << "Entity " << typeid(event.entity).name() << " was destroyed!" << std::endl;
	}
};

class MyScene : public FCS::Scene
{
public:
	virtual void initialize() override
	{
		createSystem<TransformSystem>();
		auto ent = instantiate();
	}
};

int main(int argc, char* argv[])
{
	// Extended test
	FCS::SceneManager::LoadScene<MyScene>();

	FCS::SceneManager::UnloadScene();

	// Single tests
	/*FCS::Scene scene = FCS::Scene::Create();

	scene.createSystem<TransformSystem>();


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
	
	scene.deleteSystem<TransformSystem>();*/

	return 0;
}