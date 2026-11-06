#ifndef _POWERUP_INCLUDE
#define _POWERUP_INCLUDE


#include <glm/glm.hpp>


class ShaderProgram;


// Pocket Bomberman item panels (row 8 of images/caracters.png, 16x16 each)
// Implements the 4 core power-ups:
//   1. Bomb Up (EXTRA_BOMB)
//   2. Fire Up (BOMB_RANGE)
//   3. Speed Up / Roller Skate (SPEED_UP)
//   4. Cor / Heart impact shield (HEART_SHIELD / EXTRA_LIFE)
enum PowerUpType
{
	EXTRA_BOMB   = 0, // Bomb Up:   +1 simultaneous bomb limit
	BOMB_RANGE   = 1, // Fire Up:   +1 explosion tile radius
	SPEED_UP     = 2, // Skate:     faster horizontal movement
	HEART_SHIELD = 3, // Cor/Heart: 1-hit impact shield (absorbs next enemy/explosion hit)
	EXTRA_LIFE   = 3, // Alias for compatibility
	FULL_FIRE    = 4,
	VEST         = 5,
	NUM_POWERUP_TYPES = 4
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
	static PowerUpType rollRandomType(bool allowSpeedUp = true);

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
