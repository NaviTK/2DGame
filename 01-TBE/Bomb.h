#ifndef _BOMB_INCLUDE
#define _BOMB_INCLUDE


#include <glm/glm.hpp>


// A Bomb is placed at a tile position and counts down.
// When the timer reaches zero it is marked as exploded.
// The Scene then creates an Explosion and marks it as processed.


class Bomb
{

public:
	Bomb();

	void init(const glm::ivec2 &tilePos, const glm::vec2 &worldPos, int range);
	void update(int deltaTime);

	glm::vec2 getPosition() const;
	glm::ivec2 getTilePosition() const;
	glm::vec2 getSize() const;
	int getRange() const;

	bool hasExploded() const;
	bool isProcessed() const;
	void explode();         // force immediate explosion (chain reaction)
	void markProcessed();

private:
	glm::vec2 pos;
	glm::ivec2 tilePos_;
	float timer;
	int range;
	bool exploded;
	bool processed;

};


#endif // _BOMB_INCLUDE

