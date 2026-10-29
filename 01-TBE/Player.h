#ifndef _PLAYER_INCLUDE
#define _PLAYER_INCLUDE


#include <glm/glm.hpp>
#include "Sprite.h"
#include "Texture.h"
#include "ShaderProgram.h"


class TileMap;


// The Player class handles user input, platformer physics (gravity, jump,
// horizontal velocity with tile collision), bomb placement requests, and power-up effects.
// Dynamically scales to any tile size (16, 32, etc.)


class Player
{

public:
	Player();
	~Player();

	// Animation states of the player sprite
	enum PlayerAnim
	{
		STAND_DOWN, STAND_UP, STAND_LEFT, STAND_RIGHT,
		MOVE_DOWN, MOVE_UP, MOVE_LEFT, MOVE_RIGHT,
		JUMP_LEFT, JUMP_RIGHT, FALL_LEFT, FALL_RIGHT,
		NUM_ANIMS
	};

	// Loads the spritesheet and sets up the animations (call once, after init)
	void initSprite(ShaderProgram &shaderProgram);
	void render();

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
	glm::vec2 vel;
	float speed;
	float jumpSpeed, gravity;
	float tileSize_;
	int maxBombs, activeBombs, bombRange;
	bool alive;
	bool onGround;
	bool wantsPlaceBomb_;
	bool prevSpaceState;
	bool prevJumpState;

	// Rendering / animation
	void updateAnimation(bool moving, int dirX);
	Texture spritesheet;
	Sprite *sprite;
	PlayerAnim facing; // Last STAND_* direction, used when the player stops

};


#endif // _PLAYER_INCLUDE
