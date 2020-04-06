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
	class System;

	// TODO: Threadable and Use Observer
	class Subscriber
	{
	public:
		virtual ~Subscriber() { }
	};

	template<typename Evt>
	class EventSubscriber : public Subscriber
	{
	public:
		friend class Scene;
		virtual ~EventSubscriber() { }

	protected:
		virtual void onEvent(Scene* scene, const Evt& event) = 0;
		inline void subscribe(Scene* scene, System* system);
		inline void unsubscribe(Scene* scene, System* system);
	};

	class System : public std::enable_shared_from_this<System>
	{
	public:
		virtual ~System() { }
		virtual void initialize(Scene* scene) = 0;
		virtual void deinitialize(Scene* scene) = 0;
		virtual void update(Scene* scene, float deltaTime) = 0;

	public:
		System() { }

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
		template<typename U>
		friend class EventSubscriber;
		inline void start();
		inline void update(float deltaTime);

		inline Handle<Entity> instantiate(Handle<Entity> copy = Handle<Entity>());
		inline void destroy(Handle<Entity> value);

		template<typename System>
		inline Handle<System> createSystem();
		template<typename System>
		inline void deleteSystem();

		template<typename T>
		inline void emit(const T& event);

		inline static Scene Create()
		{
			return Scene();
		}

	private:
		Scene() { }

	private:
		std::vector<std::shared_ptr<Entity>> entities;
		std::unordered_map<std::type_index, std::shared_ptr<System>> systems;
		std::unordered_map<std::type_index, std::vector<std::weak_ptr<Subscriber>>> subscribers;
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

	template<typename System>
	inline Handle<System> Scene::createSystem()
	{
		auto shared = std::make_shared<System>();
		systems.emplace(getType<System>(), shared);
		shared->initialize(this);
		return Handle<System>(shared);
	}

	template<typename System>
	inline void Scene::deleteSystem()
	{
		auto found = systems.find(getType<System>());
		if (found != systems.end())
		{
			found->second->deinitialize(this);
			systems.erase(getType<System>());
		}
	}

	template<typename T>
	inline void Scene::emit(const T& event)
	{
		auto found = subscribers.find(getType<T>());
		if (found != subscribers.end())
		{
			for (auto base : found->second)
			{
				auto* sub = reinterpret_cast<EventSubscriber<T>*>(base.lock().get());
				sub->onEvent(this, event);
			}
		}
	}

	template<typename Evt>
	inline void EventSubscriber<Evt>::subscribe(Scene* scene, System* system)
	{
		auto found = scene->subscribers.find(getType<Evt>());
		if (found != scene->subscribers.end())
		{
			found->second.push_back(std::dynamic_pointer_cast<EventSubscriber>(system->shared_from_this()));
		}
		else
		{
			auto test = system->shared_from_this();
			scene->subscribers.insert({ getType<Evt>(), { std::dynamic_pointer_cast<EventSubscriber>(test) } });
		}
	}

	template<typename Evt>
	inline void EventSubscriber<Evt>::unsubscribe(Scene* scene, System* system)
	{
		auto found = scene->subscribers.find(getType<Evt>());
		if (found != scene->subscribers.end())
		{
			for (unsigned int i = 0; i < found->second.size(); i++)
			{
				if (std::dynamic_pointer_cast<EventSubscriber>(system->shared_from_this()) == found->second[i].lock())
				{
					found->second.erase(found->second.begin() + i);
				}
			}
			if (found->second.size() == 0)
			{
				scene->subscribers.erase(found);
			}
		}
	}
}