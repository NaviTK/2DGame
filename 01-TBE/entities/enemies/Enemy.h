#ifndef _ENEMY_INCLUDE
#define _ENEMY_INCLUDE


#include <glm/glm.hpp>
#include "Sprite.h"
#include "Texture.h"
#include "ShaderProgram.h"


class TileMap;


// Base class for all enemies. Supports per-enemy-type custom hitboxes,
// spritesheet animations (sharing a single texture in VRAM), target player
// awareness, and virtual AI/pathfinding in update(). Compatible with ObjectPool<T, Capacity>.
class Enemy
{

public:
	enum Direction { DIR_LEFT = 2, DIR_RIGHT = 3 };

	Enemy();
	virtual ~Enemy();

	virtual void init(const glm::vec2 &initialPos, float tileSize = 16.0f);
	virtual void initSprite(ShaderProgram &shaderProgram);
	virtual void update(int deltaTime, TileMap *map);
	virtual void render() const;

	// Updates awareness of the player's current hitbox so AI subclasses (e.g. KirbyEnemy)
	// can detect when the player is on the same platform.
	void setTargetPlayer(const glm::vec2 &playerHitboxPos, const glm::vec2 &playerHitboxSize, bool playerAlive);

	glm::vec2 getPosition() const;
	void setPosition(const glm::vec2 &newPos);
	glm::vec2 getSize() const;

	// Custom per-enemy hitbox (horizontally centered, bottom-aligned with the tile floor)
	glm::vec2 getHitboxPos() const;
	glm::vec2 getHitboxSize() const;
	void setHitboxScale(const glm::vec2 &scale);

	void reverseDirection();
	int getDirection() const;

	// ObjectPool & lifecycle interface
	bool isActive() const;
	void deactivate();
	bool isAlive() const;
	virtual void kill();

	glm::ivec2 getTilePosition(TileMap *map) const;

protected:
	static Texture &getSharedSpritesheet();
	bool hasFloorAtTile(TileMap *map, int tx, int ty) const;

protected:
	glm::vec2 pos;
	float velY;
	float speed;
	float tileSize_;
	glm::vec2 hitboxScale_; // Hitbox size as a fraction of tileSize_ (e.g. 0.68 x 0.82)
	int direction;          // DIR_LEFT (2) or DIR_RIGHT (3)
	bool onGround;
	bool alive;
	bool dying;
	float deathTimer;

	// Target player info for AI pathfinding
	glm::vec2 targetPlayerPos_;
	glm::vec2 targetPlayerSize_;
	bool targetPlayerAlive_;

	Sprite *sprite;
	float spriteTileSize_;
	glm::vec2 spriteFrameSize_; // Pixel dimensions of one cell for this enemy type

};


#endif // _ENEMY_INCLUDE
