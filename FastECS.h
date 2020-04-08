/*
MIT License

Copyright (c) 2020 César Godinho

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

/* Part of the credit goes to Sam Bloomberg and his repo about ECS, available at: https://github.com/redxdev/ECS */
// This is my attempt to create a very simplistic unity like engine
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

// Shloud define some aspect to draw before the other updates in case this project gets extended
/* 
   TODO 

 - Add Rendering
 - Add Scripting (?)
 - Add Physics (prob 3rd party)
 - Add Timing
 - Add Math commons

   DONE
 - ECS
 - Events
 - BMP loading
 */

#define FCS_COMPONENT(name) private: virtual std::shared_ptr<Component> clone() override { return std::make_shared<name>(*this); }

#define SYSTEM_NOHANDLE // Makes it so the createSystem does not return a Handle<SystemType>

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

	template<typename... U>
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
		template<typename... U>
		friend class System;
		friend class Scene;
		virtual ~EventSubscriber() { }

	protected:
		virtual void onEvent(Scene* scene, const Evt& event) = 0;

	private:
		inline void subscribe(Scene* scene);
		inline void unsubscribe(Scene* scene);
	};

	class BaseSystem
	{
	public:
		virtual ~BaseSystem() { }

		virtual void initialize(Scene* scene) = 0;
		virtual void deinitialize(Scene* scene) = 0;
		virtual void update(Scene* scene, float deltaTime) = 0;

	protected:
		std::unordered_map<std::type_index, std::vector<std::weak_ptr<Component>>> specificComponents; //TODO: This will be targeted later
		bool isActive = true;
	};

	template<typename... Events>
	class System : public BaseSystem, public EventSubscriber<Events>...
	{
	public:
		friend class Scene;
		virtual ~System() { }
		
	private:
		inline void internal_initialize(Scene* scene);
		inline void internal_deinitialize(Scene* scene);

	public:
		System() { }

	private:
		template <typename U>
		void call_subscribe(Scene* scene)
		{
			FCS::EventSubscriber<U>::subscribe(scene);
		}

		template <typename U>
		void call_unsubscribe(Scene* scene)
		{
			FCS::EventSubscriber<U>::unsubscribe(scene);
		}

		template <typename U, typename... Us>
		inline typename std::enable_if< (sizeof...(Us) > 0) >::type call_subscribe(Scene* scene)
		{
			FCS::EventSubscriber<U>::subscribe(scene);
			call_subscribe<Us...>(scene);
		}

		template <typename U, typename... Us>
		inline typename std::enable_if< (sizeof...(Us) > 0) >::type call_unsubscribe(Scene* scene)
		{
			FCS::EventSubscriber<U>::unsubscribe(scene);
			call_unsubscribe<Us...>(scene);
		}
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

		// Checks if the rvalue inside the handle is valid
		inline bool expired()
		{
			return data.expired();
		}

	private:
		std::weak_ptr<T> data;
	};

	class Entity
	{
	public: //TODO: This should be removed from public access
		Entity() { }
		Entity(const Entity& other)
		{
			for (auto oc : other.components)
			{
				components.insert({ oc.first, oc.second->clone() });
			}
		}

	public:
		// Add a component of type to the entity
		template<typename T>
		inline Handle<T> addComponent();

		// Remove a comnponent of type from the entity
		template<typename T>
		inline void removeComponent();

		// Get a component of type in the entity
		template<typename T>
		inline Handle<T> getComponent();

		// Check entity has a type of component
		template<typename T>
		inline bool has();

		// Check entity has all types of components
		template<typename T, typename U, typename... Args>
		inline bool has();

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

	class SceneManager;

	class Scene
	{
	public:
		template<typename U>
		friend class EventSubscriber;
		friend class SceneManager;

	protected:
		// User code to initialize the scene
		inline virtual void initialize() = 0;

		// User code to clean the scene
		inline virtual void deinitialize() = 0;

	public:
		// Instantiates a entity on the scene
		inline Handle<Entity> instantiate(Handle<Entity> copy = Handle<Entity>());

		// Destroys an entity on the scene
		inline void destroy(Handle<Entity> value);

		// Get all entities on the scene
		inline std::vector<Handle<Entity>> getAll();

		// Get all the entities that have all selected types in the scene
		template<typename... Types>
		inline std::vector<Handle<Entity>> getAllWith();

#ifndef SYSTEM_NOHANDLE
		// Create a system in the scene
		template<typename System>
		inline Handle<System> createSystem();
#else
		// Create a system in the scene
		template<typename System>
		inline void createSystem();
#endif
		// Remove a system from the scene
		template<typename System>
		inline void deleteSystem();

		// Emit an event to all active subscribers
		template<typename T>
		inline void emit(const T& event);

	private:
		inline void internal_update(float deltaTime);

	public:
		Scene() { }

	private:
		std::vector<std::shared_ptr<Entity>> entities;
		std::unordered_map<std::type_index, std::shared_ptr<BaseSystem>> systems;
		std::unordered_map<std::type_index, std::vector<Subscriber*>> subscribers;
	};

	//TODO: Extend states system (?)

	class SceneManager
	{
	private:
		inline void update();

	public:
		// Loads the selected scene on top of the stack
		template<typename Scene>
		static void LoadScene(bool unloadLast = false);

		// Unloads last scene (top of stack)
		static void UnloadScene();

		// Get all scenes in the stack
		static std::size_t GetSceneCount();

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

	inline void Scene::internal_update(float deltaTime)
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

	inline std::vector<Handle<Entity>> Scene::getAll()
	{
		std::vector<Handle<Entity>> ret;
		ret.reserve(entities.size());
		for (auto ent : entities)
		{
			ret.push_back(Handle<Entity>(ent));
		}
		return ret;
	}

	template<typename ...Types>
	inline std::vector<Handle<Entity>> Scene::getAllWith()
	{
		std::vector<Handle<Entity>> ret;
		for (auto ent : entities)
		{
			if (ent->has<Types...>())
			{
				ret.push_back(Handle<Entity>(ent));
			}
		}
		return ret;
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

	template<typename T>
	inline bool Entity::has()
	{
		return components.find(getType<T>()) != components.end();
	}

	template<typename T, typename U, typename... Args>
	inline bool Entity::has()
	{
		return has<T>() && has<U, Args...>();
	}

#ifndef SYSTEM_NOHANDLE
	template<typename System>
	inline Handle<System> Scene::createSystem()
	{
		auto shared = std::make_shared<System>();
		systems.emplace(getType<System>(), shared);
		shared->internal_initialize(this);
		return Handle<System>(shared);
	}
#else
	template<typename System>
	inline void Scene::createSystem()
	{
		auto shared = std::make_shared<System>();
		systems.emplace(getType<System>(), shared);
		shared->internal_initialize(this);
	}
#endif

	template<typename System>
	inline void Scene::deleteSystem()
	{
		auto found = systems.find(getType<System>());
		if (found != systems.end())
		{
			found->second->internal_deinitialize(this);
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

	template<typename... Events>
	inline void System<Events...>::internal_initialize(Scene* scene)
	{
		// Subscribe to templated events
		call_subscribe<Events...>(scene);

		// User defined initialize
		initialize(scene);
	}

	template<typename... Events>
	inline void System<Events...>::internal_deinitialize(Scene* scene)
	{
		// User defined deinitialize
		deinitialize(scene);

		// Unsubscribe to templated events
		call_unsubscribe<Events...>(scene);
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
		//FIX: Use clock instead of the constant 0 placeholder in delta time
		scenes.top()->internal_update(0);
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

		static_assert(!std::is_abstract<Scene>(), "Can't initialize an abstract scene!");
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
	inline std::size_t SceneManager::GetSceneCount()
	{
		return Instance().scenes.size();
	}
}

// Image loading Code Start Chunk
// The only supported formats for loading are BMP's
// There should be a PNG loader later, but as this requires deflation, stick with BMP for now
namespace resource_loader
{
	typedef std::uint8_t uint8;
	typedef std::uint16_t uint16;
	typedef std::uint32_t uint32;
	typedef std::uint64_t uint64;

	typedef std::int32_t int32;

	typedef unsigned char byte;
	//typedef char byte;

	namespace image_bmp
	{
#pragma pack(push, 1)
		struct FileHeader
		{
			uint16 file_type;
			uint32 file_size;
			uint16 reserved0;
			uint16 reserved1;
			uint32 offset_data;
		};

		struct InfoHeader
		{
			uint32 size;
			uint32 width;
			uint32 height;

			uint16 planes;
			uint16 depth;
			uint32 compression;
			uint32 size_img;
			int32 x_pmeter;
			int32 y_pmeter;
			uint32 colors_used;
			uint32 colors_important;
		};

		struct ColorHeader
		{
			uint32 red_mask;
			uint32 green_mask;
			uint32 blue_mask;
			uint32 alpha_mask;

			uint32 colorspace;
			uint32 unused[16];
		};
#pragma pack(pop)

		struct Image
		{
			byte* data;
			size_t size;
			uint32 width;
			uint32 height;
			uint32 channels;
		};

		// This is an approach like std::bitset
		template<std::size_t N>
		using byte_size =
			typename std::conditional<N == 8, uint8,
			typename std::conditional<N == 16, uint16,
			typename std::conditional<N == 32, uint32,
			uint64
			>::type
			>::type
			>::type;

		template<std::size_t N>
		inline static byte_size<N> readNbytes(byte** buffer)
		{
			static_assert(N == 8 || N == 16 || N == 32 || N == 64, "Error: not a valid size for readNbytes().");
			byte_size<N> value;
			std::size_t s = sizeof(byte_size<N>);
			std::memcpy((byte*)&value, *buffer, sizeof(byte_size<N>));
			*buffer += sizeof(byte_size<N>); // Be careful with the buffer overflow
			return value;
		}

		template<typename T>
		inline static T readPackedStruct(byte** buffer)
		{
			T value;
			std::memcpy((byte*)&value, *buffer, sizeof(T));
			*buffer += sizeof(T);
			return value;
		}

		inline static uint32 make_stride_aligned(uint32 align_stride, uint32 row_stride)
		{
			uint32_t new_stride = row_stride;
			while (new_stride % align_stride != 0) 
			{
				new_stride++;
			}
			return new_stride;
		}

		inline static Image read32_24BMP(const char* file)
		{
			FileHeader fileH;
			InfoHeader infoH;
			ColorHeader colorH;
			byte* data;
			byte* data_start;

			// Read binary data
			FILE* f = std::fopen(file, "rb");
			std::fseek(f, 0, SEEK_END);
			long fsize = std::ftell(f);
			std::fseek(f, 0, SEEK_SET);

			data = (byte*)malloc(fsize * sizeof(byte));
			data_start = data;
			if (data == nullptr)
			{
				return Image(); // TODO: Handle error
			}
			else
			{
				std::fread(data, 1, fsize, f); // FIX: Error C6386 here...
			}
			std::fclose(f);

			fileH = readPackedStruct<FileHeader>(&data);

			// BMP file type specifier
			if (fileH.file_type != 0x4D42)
			{
				if (data_start)
					free(data_start);

				return Image(); // TODO: Handle error
			}

			infoH = readPackedStruct<InfoHeader>(&data);

			if (infoH.depth == 32 && infoH.size >= sizeof(InfoHeader) + sizeof(ColorHeader))
			{
				colorH = readPackedStruct<ColorHeader>(&data);

				// Check for the color specification - BGRA
				if (colorH.red_mask != 0x00ff0000 ||
					colorH.green_mask != 0x0000ff00 ||
					colorH.blue_mask != 0x000000ff ||
					colorH.alpha_mask != 0xff000000)
				{
					if (data_start)
						free(data_start);

					return Image(); // TODO: Handle error
				}
				// Check for colorspace specification - sRGB
				if (colorH.colorspace != 0x73524742)
				{
					if (data_start)
						free(data_start);

					return Image(); // TODO: Handle error
				}
			}

			// Jump to pixel data (FIX: Memory)
			data += fileH.offset_data - (sizeof(FileHeader) + sizeof(InfoHeader) + infoH.depth == 32 ? sizeof(ColorHeader) : 0);

			if (infoH.depth == 32)
			{
				infoH.size = sizeof(InfoHeader) + sizeof(ColorHeader);
				fileH.offset_data = sizeof(FileHeader) + sizeof(InfoHeader) + sizeof(ColorHeader);
			}
			else
			{
				infoH.size = sizeof(InfoHeader);
				fileH.offset_data = sizeof(FileHeader) + sizeof(InfoHeader);
			}

			fileH.file_size = fileH.offset_data;

			// Check if BMP is horizontally aligned
			if (infoH.height < 0)
			{
				if (data_start)
					free(data_start);

				return Image(); // TODO: Handle error
			}

			byte* image_data = (byte*)malloc(infoH.width * infoH.height * infoH.depth / 8 * sizeof(byte));

			// Check if is required row padding
			if (infoH.width % 4 == 0)
			{
				if (image_data != nullptr)
				{
					std::memcpy(image_data, data, infoH.width * infoH.height * infoH.depth / 8 * sizeof(byte));
				}
				else
				{
					if (data_start)
						free(data_start);
					if (image_data)
						free(image_data);

					return Image(); // TODO: Handle error
				}
			}
			else // Needs row padding
			{
				uint32 row_stride = infoH.width * infoH.depth / 8;
				uint32 new_stride = make_stride_aligned(4, row_stride);
				byte* padding_row = (byte*)malloc((new_stride - row_stride) * sizeof(byte));

				if (image_data != nullptr && padding_row != nullptr)
				{
					for (unsigned int y = 0; y < infoH.height; y++)
					{
						// TODO: Create a func def for memcpy and advance
						std::memcpy(image_data + row_stride * y, data, row_stride * sizeof(byte));
						data += row_stride * sizeof(byte);
						std::memcpy(padding_row, data, (new_stride - row_stride) * sizeof(byte));
						data += (new_stride - row_stride) * sizeof(byte);
					}
					free(padding_row);
					fileH.file_size += infoH.width * infoH.height * infoH.depth / 8 * sizeof(byte) + infoH.height * (new_stride - row_stride) * sizeof(byte);
				}
				else
				{
					if (data_start)
						free(data_start);
					if (image_data)
						free(image_data);

					return Image(); // TODO: Handle error
				}
			}

			if (data_start)
				free(data_start);
			if (image_data)
			{
				Image i;
				i.data = image_data;
				i.size = infoH.width * infoH.height * infoH.depth / 8 * sizeof(byte);
				i.height = infoH.height;
				i.width = infoH.width;
				i.channels = infoH.depth / 8;
				return i;
			}

			return Image(); // TODO: Handle error
		}
	}
}