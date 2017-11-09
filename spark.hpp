#ifndef SPARK_HPP
#define SPARK_HPP

#include <algorithm>
#include <any>
#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#define SPARK_VERSION_NUMBER "1.5.0"

namespace Spark {
	class Listener;
	class World;
	class GameObject;

	struct BlueprintComponent {
		std::string name;
		std::map<std::string, std::string> arguments;
	};

	struct Blueprint {
		std::string name;
		std::vector<BlueprintComponent> components;
		std::vector<std::string> listenForEvents;
	};

	constexpr static unsigned int ALL_GAMEOBJECTS = std::numeric_limits<unsigned int>::max();

	using ComponentID = std::size_t;

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

			explicit ReturnToPoolDeleter(Pool* p): pool(p) { }
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

		Pool() = default;
	};

	struct Event {
		unsigned int type, gameObjectID;
		std::any data;
	};

	using EventPtr = Pool<Event>::ptrType;

	class Component {
	private:
		const ComponentID id;

		GameObject* owner;
	public:
		virtual void fireEvent(Event* e) = 0;

		ComponentID getID() const noexcept { return id; }

		constexpr GameObject* getOwner() noexcept { return owner; }

		constexpr void setOwner(GameObject* g) noexcept { owner = g; }

		explicit Component(ComponentID _id): id(_id) { }
	};

	class GameObject {
	private:
		std::vector<std::shared_ptr<Listener>> listeners;
		std::vector<std::unique_ptr<Component>> components;

		World& world;

		const unsigned int ID;
	public:
		void fireEvent(Event* e) const { for(const auto& component : components) { component->fireEvent(e); } }
		void fireEvent(const Pool<Event>::ptrType& e) const { for(const auto& component : components) { component->fireEvent(e.get()); } }
		
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

			if(iter != components.end()) {
				return components[index - 1].get();
			}
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

		inline void destroy();

		GameObject(World& _world, unsigned int _ID): world(_world), ID(_ID) { }
	};

	class Listener {
	private:
		GameObject& owner;

		const unsigned int listensForType;
	public:
		void onNotify(Event* e) const noexcept { owner.fireEvent(e); }

		unsigned int getListensForType() const noexcept { return listensForType; }
		unsigned int getOwnerID() const noexcept { return owner.getID(); }

		Listener(GameObject& _owner, unsigned int _listensForType): owner(_owner), listensForType(_listensForType) { }
	};

	class World {
	private:
		std::vector<std::unique_ptr<GameObject>> gameObjects;
		// GameObjects have a map of listeners, mapped by the event they listen for
		std::map<unsigned int, std::map<unsigned int, std::shared_ptr<Listener>>> listeners;

		unsigned int lastID{0};
		std::vector<unsigned int> freeIDs;

		std::map<std::string, Blueprint> blueprintMap;

		BlueprintComponent parseComponent(const std::string& componentData) {
			BlueprintComponent component;

			std::size_t start, end;

			start = componentData.find("ComponentName");
			start = componentData.find('\"', start) + 1;
			end = componentData.find('\"', start);

			component.name = componentData.substr(start,end-(start));

			start = componentData.find(' ', end);

			do {
				end = componentData.find('=', start);
				if(end != std::string::npos) {
					std::string name = componentData.substr(start + 1, end - start - 1);
					start = componentData.find('\"', start) + 1;
					end = componentData.find('\"', start + 1);
					std::string data = componentData.substr(start, end - start);

					component.arguments[name] = data;

					start = end + 1;
				}
			} while(end < componentData.size() - 1);

			return component;
		}

		void parseBlueprint(const std::string& blueprint) {
			Blueprint _blueprint;
			std::size_t start, end, blueprintEnd;

			blueprintEnd = blueprint.find("</object>");

			start = blueprint.find("Name");

			start = blueprint.find('\"', start) + 1;
			end = blueprint.find('\"', start);

			_blueprint.name = blueprint.substr(start,end-(start));

			start = blueprint.find('<', start) + 1;
			end = blueprint.find('>', start);

			while(end < blueprintEnd) {
				int identifier = blueprint.find(' ', start);


				if(blueprint.substr(start, identifier - start) == "component") {
					_blueprint.components.push_back(parseComponent(blueprint.substr(start, end - start)));
				} else {
					start = blueprint.find("Name", start);
					start = blueprint.find('\"', start) + 1;
					end = blueprint.find('\"', start);
					_blueprint.listenForEvents.push_back(blueprint.substr(start, end - start));
				}

				start = blueprint.find('<', start) + 1;
				end = blueprint.find('>', start);
			}

			blueprintMap[_blueprint.name] = _blueprint;
		}

	public:
		void fireEvent(Event* e) {
			if(e->gameObjectID == ALL_GAMEOBJECTS) {
				for(auto& [_, listenerMap] : listeners) {
					if(listenerMap.find(e->type) != listenerMap.end()) {
						listenerMap.find(e->type)->second->onNotify(e);
					}
				}
			} else if(listeners.find(e->gameObjectID) != listeners.end()) {
				if(listeners[e->gameObjectID].find(e->type) != listeners[e->gameObjectID].end()) {
					listeners[e->gameObjectID][e->type]->onNotify(e);
				}
			}
		}

		void fireEvent(const Pool<Event>::ptrType& e) {
			if(e->gameObjectID == ALL_GAMEOBJECTS) {
				for(auto& [_, listenerMap] : listeners) {
					if(listenerMap.find(e->type) != listenerMap.end()) {
						listenerMap.find(e->type)->second->onNotify(e.get());
					}
				}
			} else if(listeners.find(e->gameObjectID) != listeners.end()) {
				if(listeners[e->gameObjectID].find(e->type) != listeners[e->gameObjectID].end()) {
					listeners[e->gameObjectID][e->type]->onNotify(e.get());
				}
			}
		}

		void addListener(unsigned int gameObjectID, const std::shared_ptr<Listener>& l) noexcept {
			if(listeners[gameObjectID].find(l->getListensForType()) == listeners[gameObjectID].end()) {
				listeners[gameObjectID][l->getListensForType()] = l;
			}
		}

		void removeListener(unsigned int gameObjectID, unsigned int type) noexcept {
			if(listeners[gameObjectID].find(type) != listeners[gameObjectID].end()) {
				listeners[gameObjectID].erase(type);
			}
		}

		void removeAllListeners(unsigned int gameObjectID) noexcept { listeners.erase(gameObjectID); }

		GameObject* createGameObject() noexcept {
			if(freeIDs.empty()) {
				gameObjects.push_back(std::make_unique<GameObject>(*this, ++lastID));
			} else {
				gameObjects.push_back(std::make_unique<GameObject>(*this, freeIDs[0]));
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

		void loadBlueprints(const std::string& filePath) {
			std::ifstream file(filePath);

			std::string fileText, line, blueprint;

			while(getline(file, line)) {
				if(line[0] != '#' && line[0] != '\n') {
					fileText += line;
				}
			}

			fileText.erase(std::remove(fileText.begin(), fileText.end(), '\t'), fileText.end());
			fileText.erase(std::remove(fileText.begin(), fileText.end(), '\n'), fileText.end());

			std::size_t start = 0;
			std::size_t pos = fileText.find("</object>");

		 	while(pos != std::string::npos) {
				pos += 9;

				blueprint = fileText.substr(start,pos-(start));

				parseBlueprint(blueprint);

				start = pos;
				pos = fileText.find("</object>",start);
			}
		}

		Blueprint getBlueprintByName(const std::string& blueprintName) { return blueprintMap[blueprintName]; }

		std::vector<Blueprint> getBlueprints() {
			std::vector<Blueprint> blueprints;
			for(const auto& [key, blueprint] : blueprintMap)
				blueprints.push_back(blueprint);

			return blueprints;
		}

		World() = default;
		~World() = default;
	};

	inline void GameObject::listenForEvent(unsigned int type) noexcept {
		listeners.push_back(std::make_shared<Listener>(*this, type));
		world.addListener(ID, listeners.back());
	}

	inline void GameObject::stopListeningForEvent(unsigned int type) noexcept {
		int index = 0;
		auto iter = std::find_if(listeners.begin(), listeners.end(),
			[&](const std::shared_ptr<Listener>& lPtr) {
				++index;
				return (lPtr->getListensForType() == type);
			});
		if(iter != listeners.end()) {
			world.removeListener(ID, type);
			std::swap(listeners[index - 1], listeners.back());
			listeners.pop_back();
		}
	}

	inline void GameObject::destroy() { world.destroyGameObject(this); }
}

#endif