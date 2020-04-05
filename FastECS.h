#pragma once
// This is my attempt to create a very simplistic unity like ECS
// Still a work in progress
#include <typeindex>
#include <algorithm>
#include <stdint.h>
#include <unordered_map> 
#include <functional>
#include <type_traits>
#include <chrono>
#include <memory>

#define FCS_COMPONENT(name) private: virtual std::shared_ptr<Component> clone() override { return std::make_shared<name>(*this); }

namespace FCS
{
	template<typename T>
	constexpr std::type_index getType()
	{
		return std::type_index(typeid(T));
	}

	class Component
	{
	public:
		friend class Entity;
		virtual ~Component() { }

	protected:
		Component() { }

	private:
		virtual std::shared_ptr<Component> clone() = 0;

	private:
		bool isActive = true;
	};

	class Scene;

	// TODO: Threadable and Use Observer
	class System
	{
	public:
		virtual ~System() { }
		virtual void update(Scene* scene, float deltaTime) = 0;

	private:
		std::vector<std::weak_ptr<Component>> specificComponents;
		bool isActive = true;
	};

	template<typename T>
	class Handle
	{
	public:
		friend class Scene;
		Handle() { data = std::weak_ptr<T>(); }
		Handle(const Handle<T>& other) { data = other.data; }
		Handle(std::weak_ptr<T> data) { this->data = data; }
		~Handle() = default;

	public:
		T* operator->() const
		{
			return data.lock().get();
		}

		inline bool expired()
		{
			return data.expired();
		}

	private:
		std::weak_ptr<T> data;
	};

	class Entity
	{
	public:
		Entity() { }
		Entity(const Entity& other)
		{
			for (auto oc : other.components)
			{
				components.insert({ oc.first, oc.second->clone() /*std::make_shared<Component>(*oc.second.get())*/ });
			}
		}

	public:
		template<typename T>
		inline Handle<T> addComponent();

		template<typename T>
		inline void removeComponent();

		template<typename T>
		inline Handle<T> getComponent();

	private:
		std::unordered_map<std::type_index, std::shared_ptr<Component>> components;
		bool isActive = true;
	};

	class Scene
	{
	public:
		inline void start();
		inline void update(float deltaTime);

		inline Handle<Entity> instantiate(Handle<Entity> copy = Handle<Entity>());
		inline void destroy(Handle<Entity> value);

		inline static Scene Create()
		{
			return Scene();
		}

	private:
		Scene() { }

	private:
		std::vector<std::shared_ptr<Entity>> entities;
		std::unordered_map<std::type_index, std::shared_ptr<System>> systems;
	};

	inline void Scene::start()
	{

	}

	inline void Scene::update(float deltaTime)
	{
		for (auto tsp : systems)
		{
			tsp.second->update(this, deltaTime);
		}
	}

	inline Handle<Entity> Scene::instantiate(Handle<Entity> copy)
	{
		if (copy.expired())
		{
			entities.push_back(std::move(std::make_shared<Entity>()));
			return Handle<Entity>(*(entities.end() - 1));
		}
		else
		{
			entities.push_back((std::move(std::make_shared<Entity>(*copy.data.lock().get()))));
			return Handle<Entity>(*(entities.end() - 1));
		}
	}

	inline void Scene::destroy(Handle<Entity> value)
	{
		using It = std::vector<std::shared_ptr<Entity>>::iterator;
		std::shared_ptr<Entity> newValue = value.data.lock();;
		

		for (It it = entities.begin(); it != entities.end(); it++)
		{
			if (*it == newValue)
			{
				// This calls the destructor
				entities.erase(it);
			}
		}
	}

	template<typename T>
	inline Handle<T> Entity::addComponent()
	{
		auto shared = std::make_shared<T>();
		components.emplace(getType<T>(), shared);
		return Handle<T>(shared);
	}

	template<typename T>
	inline void Entity::removeComponent()
	{
		if(components.find(getType<T>()) != components.end())
			components.erase(getType<T>());
	}
	template<typename T>
	inline Handle<T> Entity::getComponent()
	{
		auto it = components.find(getType<T>());
		if (it != components.end())
		{
			// Need a dynamic cast to derived here
			auto derived = std::dynamic_pointer_cast<T>(it->second);
			return Handle<T>(derived);
		}
		return Handle<T>();
	}
}