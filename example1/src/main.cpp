#include <any>
#include <cassert>
#include <iostream>
#include <vector>

#include "../../spark.hpp"
#include "components.hpp"
#include "events.hpp"

Spark::GameObject* createFromBlueprint(Spark::World &world, Spark::Blueprint blueprint);

int main() {
	std::cout << "Last compiled : " << __DATE__ << ", " << __TIME__ << '\n';
	std::cout << "Spark version - " << SPARK_VERSION_NUMBER << '\n';
	std::cout << "Developed by Mark Calhoun: https://github.com/FleventyFive\n\n";


	// Create and initialize pool with 100 events
	Spark::Pool<Spark::Event> eventPool;
	for(int i = 0; i < 100; ++i) {
		eventPool.add(std::make_unique<Spark::Event>());
	}

	Spark::World world;

	// Load in the blueprints
	world.loadBlueprints("objects.blpt");

	// Create the sword from the blueprint
	Spark::GameObject* sword = createFromBlueprint(world, world.getBlueprintByName("Sword"));

	// Make sure the sword doesnt have the IceDamageComponent
	assert(sword->getComponent<IceDamageComponent>() == nullptr);

	// Create an event to get the render data from the sword
	Spark::EventPtr e = eventPool.getResource();
	e->type = EVENT_GET_RENDER_DATA;
	e->data = RenderEvent();

	sword->fireEvent(e);

	// Display the render data
	std::cout << "Symbol - " << std::any_cast<RenderEvent>(e->data).symbol
		<< "\nName - " << std::any_cast<RenderEvent>(e->data).name
		<< "\nDescription - " << std::any_cast<RenderEvent>(e->data).description << '\n';

	// Create an event to get the damage from the sword
	e->type = EVENT_DEAL_DAMAGE;
	e->data = DealDamageEvent();

	sword->fireEvent(e);

	// Display the damage
	std::cout << "Swinging sword...\n";
	for(std::size_t i = 0; i < std::any_cast<DealDamageEvent>(e->data).damageVec.size(); ++i) {
		std::cout << "Damage dealt - " << std::any_cast<DealDamageEvent>(e->data).damageVec[i].damageDealt << '\n';
	}

	return 0;
}

// Here we use the data collected from the blueprints to create our gameobjects
Spark::GameObject* createFromBlueprint(Spark::World &world, Spark::Blueprint blueprint) {
	Spark::GameObject* g = world.createGameObject();

	std::cout << blueprint.name << '\n';
	for(auto& component : blueprint.components) {
		std::cout << '\t' << component.name << '\n';
		for(const auto& [key, value] : component.arguments) {
			std::cout << "\t\t" << key << " - " << value << '\n';
		}
	}
	std::cout << "Listen for -\n";
	for(const auto& name : blueprint.listenForEvents) {
		std::cout << "\t\t" << name << '\n';
	}

	for(auto& component : blueprint.components) {
		if(component.name == "RenderComponent") {
			g->addComponent<RenderComponent>(component.arguments["symbol"][0], component.arguments["name"], component.arguments["description"]);
		} else if(component.name == "DamageComponent") {
			g->addComponent<DamageComponent>(
				static_cast<unsigned int>(std::stol(component.arguments["rolls"])), 
				static_cast<unsigned int>(std::stol(component.arguments["sides"]))
			);
		} else if(component.name == "FireDamageComponent") {
			g->addComponent<FireDamageComponent>(
				static_cast<unsigned int>(std::stol(component.arguments["rolls"])), 
				static_cast<unsigned int>(std::stol(component.arguments["sides"]))
			);
		}
	}

	for(auto& event : blueprint.listenForEvents) {
		if(event == "EVENT_GET_RENDER_DATA") {
			g->listenForEvent(EVENT_GET_RENDER_DATA);
		} else if(event == "EVENT_DEAL_DAMAGE") {
			g->listenForEvent(EVENT_DEAL_DAMAGE);
		}
	}

	return g;
}
