#pragma once

#include <iostream>
#include <string>

#include "../../spark.hpp"

struct Die {
	unsigned int rolls, sides;
	Die(unsigned int _rolls, unsigned int _sides): rolls(_rolls), sides(_sides) { }
};

int rollDie(const Die& die) noexcept {
	int roll = 0;
	for(unsigned int i = 0; i < die.rolls; ++i)
		roll += rand() % die.sides + 1;
	return roll;
}

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

struct DealDamageEvent {
	std::vector<Damage> damageVec;
};

struct HealEvent { int health; };

struct PositionIncrementEvent { int incAmount; };

struct RenderEvent {
	std::string name, description;
	char symbol;
};

// union Spark::EventPayload {
// 	DamageEvent damageEvent;
// 	GetDamageEvent getDamageEvent;
// 	HealEvent healEvent;
// 	PositionIncrementEvent positionIncrementEvent;
// };

// Attack entities by posting a message with the id of the object its attacking.
// struct PositionIncrementEvent: public Spark::Event {
// 	int incAmount;

// 	PositionIncrementEvent(): Event(EVENT_INCREMENT_POSITION) { }
// };

// struct DamageGetEvent: public Spark::Event {
// 	int damage;

// 	DamageGetEvent(): Event(EVENT_GET_DAMAGE) { }
// };