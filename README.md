# Spark

Spark is an open source header-only C++ entity component library designed to be easy to use.

## Dependencies
To compile a project using Spark, a C++17 compliant compiler is required.

### Using spark with your project
Because Spark is a header only library, all you need to do is drag the header into your project and include it!
```c++
#include "spark.hpp"
```

## Quick Start

### World
The World holds all GameObjects, and pointers to listeners. You must have at least one to use Spark.
```c++
World world;
```

### Pool
The pool is multipurpose, and can be used for more than just events. To get an object from the pool call `Pool::getResource()`. Objects returned from the pool are unique pointers with a custom deleter that automatically returns them to the pool when out of scope. To add objects to the pool, pass unique pointers into `Pool::add(...)`. When retrieving objects, the type can be `auto` or `Spark::Pool<T>::ptrType`. Using a pool for events is recommended, and you can use the predefined type `Spark::EventPtr` for ease of typing and increased readability.

```c++
Spark::Pool<Spark::Event> eventPool;
for(int i = 0; i < DESIRED_NUMBER_OF_EVENTS; ++i)
	eventPool.add(std::unique_ptr<Spark::Event> { new Spark::Event });
EventPtr event = eventPool.getResource();
```

### Events
Events are used to give and receive data. To create an event, create structure that holds the data you wish to get or modify, and a list of event types. Get an event from the pool if using one, or create an event if not. set `Event::type` to the desired type, and `Event::data` to the desired data structure. If firing from the world, also set `Event::gameObjectID` to the ID of the GameObject you wish to modify. If firing directly in a GameObject, this is not necessary. 

```c++
// List of event types
enum EventTypes {
	EVENT_GET_NAME = 0
};

// Data struct
struct GetNameEvent {
	std::string name;
};

Spark::EventPtr e = eventPool.getResource();
e->type = EVENT_GET_NAME;
e->data = GetNameEvent();
e->gameObjectID = targetGameObject.getID(); // Only required if using world.fireEvent();

world.fireEvent(e.get());
// or
targetGameObject->fireEvent(e.get());
```
Note that the using .get() is only necessary if the event is from a pool. To modify Event data, create a reference to the desired data type, and cast the `Event::data`. You can then modify the data through the reference. For example:
```c++
GetNameEvent& getNameEvent = std::any_cast<GetNameEvent&>(yourEvent->data);
getNameEvent.name = "New Name";
```

### Components
A Components holds data, and event processing logic. To create one simply inherit from `Spark::Component`, add the constructor, create the fireEvent function, and initialize the Component class with an ID using Spark's getComponentID function. For exmaple:

```c++
class NameComponent: public Spark::Component {
private:
	std::string name;
public:
	void fireEvent(Spark::Event* e) {
		switch(e->type) {
			case EVENT_GET_NAME:
            // The GetNameEvent reference is in its own private scope to
            // prevent cross-initialization.
			{
				GetNameEvent& gne = std::any_cast<GetNameEvent&>(e->data);
				gne.name = name;
			}
				break;
			default:
				break;
		}
	}

	NameComponent(std::string _name): Component(Spark::getComponentID<NameComponent>()), name(_name) { }
};
```

### GameObjects
GameObjects hold components, their ID, and listen for events. To create a game object you must must have a world, and call `World::createGameObject()`.
```c++
GameObject* g = world.createGameObject();
```
To interact with components in a GameObject, call
* `GameObject::addComponent<YourComponent>(...)`
* `GameObject::getComponent<YourComponent>()`
* `GameObject::hasComponent<YourComponent>()`
* `GameObject::removeComponent<YourComponent>()`

Adding a component
```c++
g->addComponent<NameComponent>("Sword");
```
Getting a component
```c++
NameComponent* nameComponent = g->getComponent<NameComponent>();
```
Checking to see if a GameObject has a component
```c++
assert(g->hasComponent<NameComponent>());
```
Removing a component 
```c++
g->removeComponent<NameComponent>();
```
If you want a GameObject to receive an event when fired from `World::fireEvent()`, you must make the GameObject listen for that event with `GameObject::listenForEvent(EVENT_ID)`. This will register a listener that is automatically given to the world. For example:
```c++
g->listenForEvent(EVENT_ID);
```
the object g will now be notified by the listener whenever a event of type EVENT_GET_NAME is fired through world. To fire an event that effects all GameObjects listening for that event type, set `Event::gameObjectID` to `Spark::ALL_GAMEOBJECTS`.

You can also remove listeners with `GameObject::stopListeningForEvent(EVENT_ID)`
```c++
g->stopListeningForEvent(EVENT_ID);
```
