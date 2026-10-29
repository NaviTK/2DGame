#ifndef _ENEMY_INCLUDE
#define _ENEMY_INCLUDE


#include <glm/glm.hpp>


class TileMap;


// Simple enemy with platformer gravity and patrolling AI.
// Dynamically scales to any tile size (16, 32, etc.)


class Enemy
{

public:
	Enemy();

	void init(const glm::vec2 &initialPos, float tileSize = 16.0f);
	void update(int deltaTime, TileMap *map);

	glm::vec2 getPosition() const;
	glm::vec2 getSize() const;
	bool isAlive() const;
	void kill();

	glm::ivec2 getTilePosition(TileMap *map) const;

private:
	glm::vec2 pos;
	float velY;
	float speed;
	float tileSize_;
	int direction;   // 2=left, 3=right
	float dirTimer;  // ms until next random direction change
	bool alive;

};


#endif // _ENEMY_INCLUDE
