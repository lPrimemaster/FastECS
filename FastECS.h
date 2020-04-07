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
#include <stack>

#define FCS_COMPONENT(name) private: virtual std::shared_ptr<Component> clone() override { return std::make_shared<name>(*this); }

#define SYSTEM_NOHANDLE

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

	// TODO: Threadable
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
		inline void subscribe(Scene* scene);
		inline void unsubscribe(Scene* scene);
	};

	class System
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
				components.insert({ oc.first, oc.second->clone() });
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

	namespace Event
	{
		struct EntityCreated
		{
			Handle<Entity> entity;
		};

		struct EntityDestroyed
		{
			Handle<Entity> entity;
		};
	}

	class Scene
	{
	public:
		template<typename U>
		friend class EventSubscriber;
		inline virtual void initialize();
		inline void update(float deltaTime); //FIX: Decide how to handle this later! Should this be overridable

		inline Handle<Entity> instantiate(Handle<Entity> copy = Handle<Entity>());
		inline void destroy(Handle<Entity> value);

#ifndef SYSTEM_NOHANDLE
		template<typename System>
		inline Handle<System> createSystem();
#else
		template<typename System>
		inline void createSystem();
#endif
		template<typename System>
		inline void deleteSystem();

		template<typename T>
		inline void emit(const T& event);

		inline static Scene Create()
		{
			return Scene();
		}

	public:
		Scene() { }

	private:
		std::vector<std::shared_ptr<Entity>> entities;
		std::unordered_map<std::type_index, std::shared_ptr<System>> systems;
		std::unordered_map<std::type_index, std::vector<Subscriber*>> subscribers;
	};

	//TODO: Extend states system

	class SceneManager
	{
	private:
		inline void update();

	public:
		template<typename Scene>
		static void LoadScene(bool unloadLast = false);

		static void UnloadScene();

	private:
		static inline SceneManager& Instance()
		{
			static SceneManager instance;
			return instance;
		}

	private:
		SceneManager() = default;

	private:
		std::stack<std::unique_ptr<Scene>> scenes;
	};

	inline void Scene::initialize()
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
			auto ent = std::make_shared<Entity>();
			entities.push_back(ent);
			emit<Event::EntityCreated>({ Handle<Entity>(ent) });
			return Handle<Entity>(*(entities.end() - 1));
		}
		else
		{
			auto ent = std::make_shared<Entity>(*copy.data.lock().get());
			entities.push_back(ent);
			emit<Event::EntityCreated>({ Handle<Entity>(ent) });
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
				emit<Event::EntityDestroyed>({ Handle<Entity>(*it) });
				// This calls the dtor
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

#ifndef SYSTEM_NOHANDLE
	template<typename System>
	inline Handle<System> Scene::createSystem()
	{
		auto shared = std::make_shared<System>();
		systems.emplace(getType<System>(), shared);
		shared->initialize(this);
		return Handle<System>(shared);
	}
#else
	template<typename System>
	inline void Scene::createSystem()
	{
		auto shared = std::make_shared<System>();
		systems.emplace(getType<System>(), shared);
		shared->initialize(this);
	}
#endif

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
				auto* sub = reinterpret_cast<EventSubscriber<T>*>(base);
				sub->onEvent(this, event);
			}
		}
	}

	template<typename Evt>
	inline void EventSubscriber<Evt>::subscribe(Scene* scene)
	{
		auto found = scene->subscribers.find(getType<Evt>());
		if (found != scene->subscribers.end())
		{
			found->second.push_back(this);
		}
		else
		{
			scene->subscribers.insert({ getType<Evt>(), { this } });
		}
	}

	template<typename Evt>
	inline void EventSubscriber<Evt>::unsubscribe(Scene* scene)
	{
		auto found = scene->subscribers.find(getType<Evt>());
		if (found != scene->subscribers.end())
		{
			for (unsigned int i = 0; i < found->second.size(); i++)
			{
				if (this == found->second[i])
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

	inline void SceneManager::update()
	{
		//FIX: 0 placeholder for now in delta time
		scenes.top()->update(0);
	}

	template<typename Scene>
	inline void SceneManager::LoadScene(bool unloadLast)
	{
		SceneManager& sm = Instance();
		if (unloadLast && !sm.scenes.empty())
		{
			// This should call last loaded scene's dtor
			sm.scenes.pop();
		}

		auto uscene = std::make_unique<Scene>();
		uscene->initialize();
		sm.scenes.push(std::move(uscene));
	}

	inline void SceneManager::UnloadScene()
	{
		SceneManager& sm = Instance();
		if (!sm.scenes.empty())
		{
			// This should call last loaded scene's dtor
			sm.scenes.pop();
		}
	}
}