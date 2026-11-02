#ifndef _POWERUP_INCLUDE
#define _POWERUP_INCLUDE


#include <glm/glm.hpp>


class ShaderProgram;


// Pocket Bomberman item panels (row 7 of images/caracters.png, 16x16 each)
enum PowerUpType
{
	EXTRA_BOMB = 0, // Bomb Up:   +1 bomb on screen at the same time
	BOMB_RANGE = 1, // Fire Up:   +1 flame length
	SPEED_UP   = 2, // Skate:     faster movement
	EXTRA_LIFE = 3, // Heart:     +1 life
	FULL_FIRE  = 4, // Full Fire: maximum flame length
	VEST       = 5, // Vest:      10 s invincibility
	NUM_POWERUP_TYPES
};


// A PowerUp appears when a destructible block is destroyed (with some
// probability). The player collects it by touching it. Freshly spawned
// power-ups have a short grace period so the same explosion that revealed
// them does not burn them instantly.
class PowerUp
{

public:
	PowerUp();

	void init(const glm::ivec2 &tilePos, const glm::vec2 &worldPos, PowerUpType type);
	void update(int deltaTime);
	void render(ShaderProgram &texProgram, float tileSize) const;

	glm::vec2 getPosition() const;
	glm::ivec2 getTilePosition() const;
	glm::vec2 getSize() const;
	PowerUpType getType() const;
	bool isActive() const;
	void deactivate();
	bool isCollected() const;
	void collect();
	bool canBeBurned() const;

	// Source rectangle of the item panel inside images/caracters.png (shared with the HUD)
	static glm::ivec2 getSheetCoords(PowerUpType type);
	static PowerUpType rollRandomType();

private:
	glm::vec2 pos;
	glm::ivec2 tilePos_;
	PowerUpType type_;
	float animTimer_;
	float graceTimer_;
	bool active;
	bool collected;

};


#endif // _POWERUP_INCLUDE
