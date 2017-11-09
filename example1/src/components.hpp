#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include <iostream>
#include <random>

#include "../../spark.hpp"
#include "events.hpp"

class RenderComponent: public Spark::Component {
private:
	char symbol;
	std::string name, description;
public:
	void fireEvent(Spark::Event* e) override {
		switch(e->type) {
			case EVENT_GET_RENDER_DATA:
				{
					// Give the event the render data
					auto& re = std::any_cast<RenderEvent&>(e->data);
					re.symbol = symbol;
					re.name = name;
					re.description = description;
				}
				break;
			default:
				break;
		}
	}

	RenderComponent(char _symbol, const std::string& _name, const std::string& _description): Component(Spark::getComponentID<RenderComponent>()),
		symbol(_symbol), name(_name), description(_description) { }
};

class DamageComponent: public Spark::Component {
private:
	Die die;
public:
	void fireEvent(Spark::Event* e) override {
		switch(e->type) {
			case EVENT_DEAL_DAMAGE:
				{
					// Generate some damage, and give it to the event
					auto& dde = std::any_cast<DealDamageEvent&>(e->data);
					dde.damageVec.emplace_back(Damage(die.roll(), DAMAGE_SLASH));
				}
				break;
			default:
				break;
		}
	}

	DamageComponent(unsigned int rolls, unsigned int sides): Component(Spark::getComponentID<DamageComponent>()), die(Die({rolls, sides})) { }
};

class FireDamageComponent: public Spark::Component {
private:
	Die die;
public:
	void fireEvent(Spark::Event* e) override {
		switch(e->type) {
			case EVENT_DEAL_DAMAGE:
			{
				// Generate some damage, and give it to the event
				auto& dde = std::any_cast<DealDamageEvent&>(e->data);
				dde.damageVec.emplace_back(Damage(die.roll(), DAMAGE_FIRE));
			}
				break;
			default:
				break;
		}
	}

	FireDamageComponent(unsigned int rolls, unsigned int sides): Component(Spark::getComponentID<FireDamageComponent>()), die(Die({rolls, sides})) { }
};

class IceDamageComponent: public Spark::Component {
private:
	Die die;
public:
	void fireEvent(Spark::Event* e) override {
		switch(e->type) {
			case EVENT_DEAL_DAMAGE:
			{
				// Generate some damage, and give it to the event
				auto& dde = std::any_cast<DealDamageEvent&>(e->data);
				dde.damageVec.emplace_back(Damage(die.roll(), DAMAGE_ICE));
			}
				break;
			default:
				break;
		}
	}

	IceDamageComponent(unsigned int rolls, unsigned int sides): Component(Spark::getComponentID<IceDamageComponent>()), die(Die({rolls, sides})) { }
};

#endif