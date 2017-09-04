#ifndef SPARK_HPP
#define SPARK_HPP

#include <iostream>
#include <type_traits>
#include <algorithm>
#include <limits>
#include <memory>
#include <vector>
#include <deque>
#include <any>
#include <map>

#define SPARK_VERSION_NUMBER "1.0.0"

namespace Spark {
	class Listener;
	class World;

	constexpr static unsigned int ALL_GAMEOBJECTS = std::numeric_limits<unsigned int>::max();

	typedef std::size_t ComponentID;

	inline ComponentID getComponentID() noexcept {
		static std::size_t lastID = 0;
		return ++lastID;
	}

	template<typename T>
	inline ComponentID getComponentID() noexcept {
		static std::size_t id = getComponentID();
		return id;
	}

	template<typename T>
	class Pool {
	private:
		struct ReturnToPoolDeleter {
			Pool* pool;

			void operator()(T* ptr) {
				try {
					pool->add(std::unique_ptr<T>{ptr});
					return;
				} catch(...) {}
				std::default_delete<T>{}(ptr);
			}

			ReturnToPoolDeleter(Pool* p): pool(p) { }
		};

		std::deque<std::unique_ptr<T>> objects;
	public:
		using ptrType = std::unique_ptr<T, ReturnToPoolDeleter>;

		constexpr void add(std::unique_ptr<T> o) noexcept { objects.push_back(std::move(o)); }

		constexpr ptrType getResource() noexcept {
			assert(!objects.empty());
			ptrType tmp(objects.front().release(), ReturnToPoolDeleter{this});
			objects.pop_front();
			return std::move(tmp);
		}

		constexpr int size() const noexcept { return objects.size(); }
		constexpr bool empty() const noexcept { return objects.empty(); }

		constexpr Pool() noexcept { }
	};

	struct Event {
		unsigned int type, gameObjectID;
		std::any data;
	};

	typedef Pool<Event>::ptrType EventPtr;

	class Component {
	private:
		const ComponentID id;
	public:
		virtual void fireEvent(Event* e) = 0;

		ComponentID getID() const noexcept { return id; }

		Component(ComponentID _id): id(_id) { }
	};

	class GameObject {
	private:

		std::vector<std::unique_ptr<Listener>> listeners;
		std::vector<std::unique_ptr<Component>> components;

		World& world;

		const unsigned int ID;
	public:
		void fireEvent(Event* e) { for(const auto& component : components) component->fireEvent(e); }
		
		template<typename T, typename... TArgs>
		void addComponent(TArgs&&... mArgs) noexcept {
			static_assert(std::is_base_of<Component, T>::value, "T not derived from Component");
			assert(!hasComponent<T>() && "Component already exists");
			components.push_back(std::unique_ptr<Component> { new T(std::forward<TArgs>(mArgs)...) });
		}

		template<typename T>
		void removeComponent() noexcept {
			int index = 0;
			auto iter = std::find_if(components.begin(), components.end(),
			[&](const std::unique_ptr<Component>& cPtr) {
				++index;
				return (cPtr->getID() == getComponentID<T>());
			});
			if(iter != components.end()) {
				std::swap(components[index - 1], components.back());
				components.pop_back();
			}
		}

		template<typename T>
		Component* getComponent() noexcept {
			int index = 0;
			auto iter = std::find_if(components.begin(), components.end(),
			[&](const std::unique_ptr<Component>& cPtr) {
				++index;
				return (cPtr->getID() == getComponentID<T>());
			});

			if(iter != components.end())
				return components[index - 1].get();
			else
				return nullptr;
		}

		template<typename T>
		bool hasComponent() {
			auto iter = std::find_if(components.begin(), components.end(),
			[&](const std::unique_ptr<Component>& cPtr) {
				return (cPtr->getID() == getComponentID<T>());
			});
			return iter != components.end();
		}

		inline void listenForEvent(unsigned int type) noexcept;

		unsigned int getID() const noexcept { return ID; }

		GameObject(World& _world, unsigned int _ID): world(_world), ID(_ID) { }
	};

	class Listener {
	private:
		GameObject& owner;

		const unsigned int listensForType;
	public:
		void onNotify(Event* e) noexcept {
			if(e->gameObjectID == getOwnerID() || e->gameObjectID == ALL_GAMEOBJECTS)
				owner.fireEvent(e);
		}

		unsigned int getListensForType() const noexcept { return listensForType; }
		unsigned int getOwnerID() const noexcept { return owner.getID(); }

		Listener(GameObject& _owner, unsigned int _listensForType): owner(_owner), listensForType(_listensForType) { }
	};

	class World {
	private:
		std::vector<std::unique_ptr<GameObject>> gameObjects;
		std::map<unsigned int, std::vector<Listener*>> listeners;

		unsigned int lastID;

		std::vector<unsigned int> freeIDs;
	public:
		void fireEvent(Event* e) {
			if(listeners.find(e->type) != listeners.end()) {
				const std::vector<Listener*>& listenerVec = listeners[e->type];
				for(const auto& listener : listenerVec)
					listener->onNotify(e);
			}
		}

		void addListener(Listener* l) noexcept {
			listeners[l->getListensForType()].push_back(l);
		}

		GameObject* createGameObject() noexcept {
			if(freeIDs.size() == 0) {
				gameObjects.push_back(std::unique_ptr<GameObject> { new GameObject(*this, ++lastID) });
			} else {
				gameObjects.push_back(std::unique_ptr<GameObject> { new GameObject(*this, freeIDs[0]) });
				freeIDs.erase(freeIDs.begin());
			}
			return gameObjects.back().get();
		}

		void destroyGameObject(GameObject* g) noexcept {
			int index = 0;
			unsigned int id = g->getID();
			auto iter = std::find_if(gameObjects.begin(), gameObjects.end(),
			[&](const std::unique_ptr<GameObject>& gPtr) {
				++index;
				return (gPtr->getID() == id);
			});
			if(iter != gameObjects.end()) {
				std::swap(gameObjects[index - 1], gameObjects.back());
				gameObjects.pop_back();
				freeIDs.push_back(id);
			}
		}

		World(): lastID(0) { }
	};

	inline void GameObject::listenForEvent(unsigned int type) noexcept {
		listeners.push_back(std::unique_ptr<Listener> { new Listener(*this, type) });
		world.addListener(listeners.back().get());
	}
};

#endif