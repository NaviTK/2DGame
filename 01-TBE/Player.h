#ifndef _PLAYER_INCLUDE
#define _PLAYER_INCLUDE


#include <glm/glm.hpp>


class TileMap;


// The Player class handles user input, movement with tile collision,
// bomb placement requests, and power-up effects.
// Dynamically scales to any tile size (16, 32, etc.)


class Player
{

public:
	Player();

	void init(const glm::vec2 &initialPos, float tileSize = 16.0f);
	void update(int deltaTime, TileMap *map);

	glm::vec2 getPosition() const;
	glm::vec2 getSize() const;
	glm::vec2 getHitboxPos() const;
	glm::vec2 getHitboxSize() const;
	bool isAlive() const;
	void kill();

	// Bomb management
	bool wantsToPlaceBomb() const;
	void bombPlaced();
	void bombExploded();
	int getBombRange() const;
	int getMaxBombs() const;
	int getActiveBombs() const;

	// Power-ups: 0 = EXTRA_BOMB, 1 = BOMB_RANGE, 2 = SPEED
	void addPowerUp(int type);

	glm::ivec2 getTilePosition(TileMap *map) const;

private:
	glm::vec2 pos;
	float speed;
	float tileSize_;
	int maxBombs, activeBombs, bombRange;
	bool alive;
	bool wantsPlaceBomb_;
	bool prevSpaceState;

};


#endif // _PLAYER_INCLUDE

