#ifndef SPARK_HPP
#define SPARK_HPP

#include <iostream>
#include <algorithm>
#include <limits>
#include <memory>
#include <vector>
#include <deque>
#include <any>
#include <map>

#include <assert.h>

#define SPARK_VERSION_NUMBER "1.1.0"

namespace Spark {
	class Listener;
	class World;
	class GameObject;

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

		GameObject* owner;
	public:
		virtual void fireEvent(Event* e) = 0;

		ComponentID getID() const noexcept { return id; }

		constexpr GameObject* getOwner() noexcept { return owner; }

		constexpr void setOwner(GameObject* g) noexcept { owner = g; }

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
		void fireEvent(Pool<Event>::ptrType& e) { for(const auto& component : components) component->fireEvent(e.get()); }
		
		template<typename T, typename... TArgs>
		void addComponent(TArgs&&... mArgs) noexcept {
			static_assert(std::is_base_of<Component, T>::value, "T not derived from Component");
			assert(!hasComponent<T>() && "Component already exists");
			components.push_back(std::unique_ptr<Component> { new T(std::forward<TArgs>(mArgs)...) });
			components.back()->setOwner(this);
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
		inline void stopListeningForEvent(unsigned int type) noexcept;

		unsigned int getID() const noexcept { return ID; }

		GameObject(World& _world, unsigned int _ID): world(_world), ID(_ID) { }
	};

	class Listener {
	private:
		GameObject& owner;

		const unsigned int listensForType;
	public:
		void onNotify(Event* e) noexcept { owner.fireEvent(e); }

		unsigned int getListensForType() const noexcept { return listensForType; }
		unsigned int getOwnerID() const noexcept { return owner.getID(); }

		Listener(GameObject& _owner, unsigned int _listensForType): owner(_owner), listensForType(_listensForType) { }
	};

	class World {
	private:
		std::vector<std::unique_ptr<GameObject>> gameObjects;
		// GameObjects have a map of listeners, mapped by 
		std::map<unsigned int, std::map<unsigned int, Listener*>> listeners;

		unsigned int lastID;

		std::vector<unsigned int> freeIDs;
	public:
		void fireEvent(Event* e) {
			if(e->gameObjectID == ALL_GAMEOBJECTS) {
				for(auto& [key, listenerMap] : listeners) {
					if(listenerMap.find(e->type) != listenerMap.end())
						listenerMap.find(e->type)->second->onNotify(e);
				}
			} else if(listeners.find(e->gameObjectID) != listeners.end()) {
				if(listeners[e->gameObjectID].find(e->type) != listeners[e->gameObjectID].end()) {
					listeners[e->gameObjectID][e->type]->onNotify(e);
				}
			}
		}

		void fireEvent(Pool<Event>::ptrType& e) {
			if(e->gameObjectID == ALL_GAMEOBJECTS) {
				for(auto& [key, listenerMap] : listeners) {
					if(listenerMap.find(e->type) != listenerMap.end())
						listenerMap.find(e->type)->second->onNotify(e.get());
				}
			} else if(listeners.find(e->gameObjectID) != listeners.end()) {
				if(listeners[e->gameObjectID].find(e->type) != listeners[e->gameObjectID].end()) {
					listeners[e->gameObjectID][e->type]->onNotify(e.get());
				}
			}
		}

		void addListener(unsigned int gameObjectID, Listener* l) noexcept {
			if(listeners[gameObjectID].find(l->getListensForType()) == listeners[gameObjectID].end())
				listeners[gameObjectID][l->getListensForType()] = l;
		}

		void removeListener(unsigned int gameObjectID, unsigned int type) noexcept {
			if(listeners[gameObjectID].find(type) != listeners[gameObjectID].end())
				listeners[gameObjectID].erase(type);
		}

		void removeAllListeners(unsigned int gameObjectID) noexcept {
			listeners.erase(gameObjectID);
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
				removeAllListeners(id);
			}
		}

		World(): lastID(0) { }
	};

	inline void GameObject::listenForEvent(unsigned int type) noexcept {
		listeners.push_back(std::unique_ptr<Listener> { new Listener(*this, type) });
		world.addListener(ID, listeners.back().get());
	}

	inline void GameObject::stopListeningForEvent(unsigned int type) noexcept {
		int index = 0;
		auto iter = std::find_if(listeners.begin(), listeners.end(),
			[&](const std::unique_ptr<Listener>& lPtr) {
				++index;
				return (lPtr->getListensForType() == type);
			});
		if(iter != listeners.end()) {
			world.removeListener(ID, type);
			std::swap(listeners[index - 1], listeners.back());
			listeners.pop_back();
		}
	}
};

#endif