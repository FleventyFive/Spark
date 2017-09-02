#pragma once

#include <iostream>
#include <random>

#include "spark.hpp"
#include "events.hpp"

class RenderComponent: public Spark::Component {
private:
	char symbol;
	std::string name, description;
public:
	void fireEvent(Spark::Event* e) {
		switch(e->type) {
			case EVENT_GET_RENDER_DATA:
				{
					RenderEvent& re = std::experimental::any_cast<RenderEvent&>(e->data);
					re.symbol = symbol;
					re.name = name;
					re.description = description;
				}
				break;
			default:
				break;
		}
	}

	RenderComponent(char _symbol, std::string _name, std::string _description): Component(Spark::getComponentID<RenderComponent>()),
		symbol(_symbol), name(_name), description(_description) { }
};

class DamageComponent: public Spark::Component {
private:
	Die die;
public:
	void fireEvent(Spark::Event* e) {
		switch(e->type) {
			case EVENT_DEAL_DAMAGE:
				{
					DealDamageEvent& dde = std::experimental::any_cast<DealDamageEvent&>(e->data);
					dde.damageVec.push_back(Damage(rollDie(die), DAMAGE_SLASH));
				}
				break;
			default:
				break;
		}
	}

	DamageComponent(Die _die): Component(Spark::getComponentID<DamageComponent>()), die(_die) { }
};

class FireDamageComponent: public Spark::Component {
private:
	Die die;
public:
	void fireEvent(Spark::Event* e) {
		switch(e->type) {
			case EVENT_DEAL_DAMAGE:
			{
				DealDamageEvent& dde = std::experimental::any_cast<DealDamageEvent&>(e->data);
				dde.damageVec.push_back(Damage(rollDie(die), DAMAGE_FIRE));
			}
				break;
			default:
				break;
		}
	}

	FireDamageComponent(Die _die): Component(Spark::getComponentID<FireDamageComponent>()), die(_die) { }
};

class IceDamageComponent: public Spark::Component {
private:
	Die die;
public:
	void fireEvent(Spark::Event* e) {
		switch(e->type) {
			case EVENT_DEAL_DAMAGE:
			{
				DealDamageEvent& dde = std::experimental::any_cast<DealDamageEvent&>(e->data);
				dde.damageVec.push_back(Damage(rollDie(die), DAMAGE_ICE));
			}
				break;
			default:
				break;
		}
	}

	IceDamageComponent(Die _die): Component(Spark::getComponentID<IceDamageComponent>()), die(_die) { }
};
