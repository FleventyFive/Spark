#ifndef EVENTS_HPP
#define EVENTS_HPP

#include <iostream>
#include <random>
#include <string>

#include "../../spark.hpp"

class Die {
private:
	std::mt19937 eng{std::random_device{}()};

	unsigned int rolls, sides;
public:
	unsigned int roll() {
		unsigned int roll = 0;
		for(unsigned int i = 0; i < rolls; ++i) {
			roll += std::uniform_int_distribution<unsigned int>{1, sides}(eng);
		}
		return roll;
	}

	Die(unsigned int _rolls, unsigned int _sides): rolls(_rolls), sides(_sides) { }
};

enum EventType {
	EVENT_DAMAGE = 0,
	EVENT_HEAL,
	EVENT_DEAL_DAMAGE,
	EVENT_INCREMENT_POSITION,
	EVENT_UPDATE,
	EVENT_GET_RENDER_DATA
};

enum DamageType {
	DAMAGE_FIRE = 0,
	DAMAGE_ICE,
	DAMAGE_SLASH
};

struct Damage {
	int damageDealt;
	DamageType type;

	Damage(int _damageDealt, DamageType _type): damageDealt(_damageDealt), type(_type) { }
};

struct DealDamageEvent { std::vector<Damage> damageVec; };

struct HealEvent { int health; };

struct PositionIncrementEvent { int incAmount; };

struct RenderEvent {
	std::string name, description;
	char symbol;
};

#endif