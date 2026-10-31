#ifndef _POWERUP_INCLUDE
#define _POWERUP_INCLUDE


#include <glm/glm.hpp>


enum PowerUpType { EXTRA_BOMB = 0, BOMB_RANGE = 1, SPEED_UP = 2 };


// A PowerUp appears when a destructible block is destroyed (with some
// probability). The player collects it by stepping on it.


class PowerUp
{

public:
	PowerUp();

	void init(const glm::ivec2 &tilePos, const glm::vec2 &worldPos, PowerUpType type);

	glm::vec2 getPosition() const;
	glm::ivec2 getTilePosition() const;
	glm::vec2 getSize() const;
	PowerUpType getType() const;
	bool isActive() const;
	void deactivate();
	bool isCollected() const;
	void collect();

private:
	glm::vec2 pos;
	glm::ivec2 tilePos_;
	PowerUpType type_;
	bool active;
	bool collected;

};


#endif // _POWERUP_INCLUDE
