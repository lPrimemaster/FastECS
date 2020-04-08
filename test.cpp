// Test file
#include "FastECS.h"
#include <iostream>

class Transform : public FCS::Component
{
public:
	float x, y, z;

	FCS_COMPONENT(Transform);
};

class TransformSystem : public FCS::System<FCS::Event::EntityCreated>
{
public:
	void initialize(FCS::Scene* scene) override
	{
		auto ent_wout = scene->instantiate();
		auto ent_with = scene->getAllWith<Transform>();
	}

	void deinitialize(FCS::Scene* scene) override
	{

	}

	void update(FCS::Scene* scene, float deltaTime) override
	{
		
	}

	void onEvent(FCS::Scene* scene, const FCS::Event::EntityCreated& event)
	{
		std::cout << "Entity " << typeid(event.entity).name() << " was created!" << std::endl;
	}
};

class MyScene : public FCS::Scene
{
public:
	void initialize() override
	{
		createSystem<TransformSystem>();
	}

	void deinitialize() override
	{
		//Empty
	}
};

int main(int argc, char* argv[])
{
	// Extended test
	FCS::SceneManager::LoadScene<MyScene>();
	FCS::SceneManager::UnloadScene();

	resource_loader::image_bmp::Image img = resource_loader::image_bmp::read32_24BMP("Shapes.bmp");

	return 0;
}