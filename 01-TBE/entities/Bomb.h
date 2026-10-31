#ifndef _BOMB_INCLUDE
#define _BOMB_INCLUDE


#include <glm/glm.hpp>


class ShaderProgram;


// A Bomb is placed at a tile position and counts down for 3 seconds (3000 ms).
// Every 1 second (1000 ms), it advances to the next fuse sprite from row 3 of
// images/caracters.png (x = 57, 75, 92; y = 51; 16x16 px).
// When the timer reaches zero it is marked as exploded.
class Bomb
{

public:
	Bomb();

	void init(const glm::ivec2 &tilePos, const glm::vec2 &worldPos, int range);
	void update(int deltaTime);
	void render(ShaderProgram &texProgram, float tileSize) const;

	glm::vec2 getPosition() const;
	glm::ivec2 getTilePosition() const;
	glm::vec2 getSize() const;
	int getRange() const;
	int getAnimStage() const; // 0 (3s..2s), 1 (2s..1s), 2 (1s..0s)

	bool isActive() const;
	void deactivate();
	bool hasExploded() const;
	bool isProcessed() const;
	void explode();         // force immediate explosion (chain reaction)
	void markProcessed();

	// Shared helper to draw any sub-rectangle of images/caracters.png without extra VAO/VBO allocations
	static void drawSheetRect(ShaderProgram &texProgram, const glm::vec2 &worldPos, const glm::vec2 &worldSize,
	                          int srcX, int srcY, int srcW, int srcH);

private:
	glm::vec2 pos;
	glm::ivec2 tilePos_;
	float timer;
	int range;
	bool active;
	bool exploded;
	bool processed;

};


#endif // _BOMB_INCLUDE
