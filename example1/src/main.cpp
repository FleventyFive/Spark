#include <iostream>
#include <algorithm>
#include <vector>
#include <limits>
#include <stack>
#include <ctime>
#include <any>

#include <assert.h>

#include "../../spark.hpp"
#include "components.hpp"
#include "events.hpp"

int main(void) {
	srand((unsigned int)time(NULL));

	std::cout << "Last compiled : " << __DATE__ << ", " << __TIME__ << '\n';
  std::cout << "Spark version - " << SPARK_VERSION_NUMBER << '\n';
	std::cout << "Developed by Mark Calhoun: https://github.com/FleventyFive\n\n";


	// create and initialize pool with 100 events
	Spark::Pool<Spark::Event> eventPool;
	for(int i = 0; i < 100; ++i)
		eventPool.add(std::unique_ptr<Spark::Event>{ new Spark::Event });

	Spark::World world;

	Spark::GameObject* sword = world.createGameObject();
	sword->addComponent<RenderComponent>('|', "Sword", "A dull sword, hot to the touch");
	sword->addComponent<DamageComponent>(Die(1, 6));
	sword->addComponent<FireDamageComponent>(Die(1, 4));
	sword->listenForEvent(EVENT_GET_RENDER_DATA);
	sword->listenForEvent(EVENT_DEAL_DAMAGE);

	assert(sword->getComponent<IceDamageComponent>() == nullptr);

	Spark::EventPtr e = eventPool.getResource();
	e->type = EVENT_GET_RENDER_DATA;
	e->data = RenderEvent();

	sword->fireEvent(e.get());

	std::cout << "Symbol - " << std::any_cast<RenderEvent>(e->data).symbol
		<< "\nName - " << std::any_cast<RenderEvent>(e->data).name
		<< "\nDescription - " << std::any_cast<RenderEvent>(e->data).description << '\n';

	e->type = EVENT_DEAL_DAMAGE;
	e->data = DealDamageEvent();

	sword->fireEvent(e.get());

	std::cout << "Swinging sword...\n";
	for(std::size_t i = 0; i < std::any_cast<DealDamageEvent>(e->data).damageVec.size(); ++i)
		std::cout << "Damage dealt - " << std::any_cast<DealDamageEvent>(e->data).damageVec[i].damageDealt << '\n';

	return 0;
}
