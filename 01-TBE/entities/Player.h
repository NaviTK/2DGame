#ifndef _PLAYER_INCLUDE
#define _PLAYER_INCLUDE


#include <vector>
#include <glm/glm.hpp>
#include "Sprite.h"
#include "Texture.h"
#include "ShaderProgram.h"


class TileMap;


// The Player class handles user input, platformer physics (gravity, jump,
// one-way grass platforms, vine climbing, horizontal velocity with tile collision),
// lives & invulnerability blinking, bomb placement requests, and power-up effects.
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
		CLIMB_IDLE, CLIMB_MOVE,
		NUM_ANIMS
	};

	// Loads the spritesheet and sets up the animations (call once, after init)
	void initSprite(ShaderProgram &shaderProgram);
	void render();

	void init(const glm::vec2 &initialPos, float tileSize = 16.0f);
	void update(int deltaTime, TileMap *map);

	glm::vec2 getPosition() const;
	void setPosition(const glm::vec2 &newPos);
	glm::vec2 getVelocity() const;
	void setVelocity(const glm::vec2 &newVel);
	glm::vec2 getSize() const;
	glm::vec2 getHitboxPos() const;
	glm::vec2 getHitboxSize() const;
	bool isAlive() const;
	int getLives() const;
	bool isInvulnerable() const;
	bool shouldRenderBlink() const;
	void takeDamage();
	void applyKnockback(const glm::vec2 &impulse);
	void kill();

	// Bomb management
	bool wantsToPlaceBomb() const;
	void bombPlaced();
	void bombExploded();
	int getBombRange() const;
	int getMaxBombs() const;
	int getActiveBombs() const;

	// Power-ups: 0 = EXTRA_BOMB, 1 = BOMB_RANGE, 2 = SPEED, 3 = EXTRA_LIFE, 4 = FULL_FIRE, 5 = VEST
	void addPowerUp(int type);
	int getSpeedUps() const;
	bool hasVest() const;
	float getVestTimer() const;

	// Upgrade caps (shared with the save system and HUD)
	static const int MAX_LIVES = 9;
	static const int MAX_BOMBS_CAP = 8;
	static const int MAX_RANGE_CAP = 8;
	static const int MAX_SPEED_UPS = 4;
	void applyStats(int newLives, int newMaxBombs, int newBombRange, int newSpeedUps);
	void syncInputLatch();
	void setBombPlatforms(const std::vector<glm::vec2> &platforms);

	glm::ivec2 getTilePosition(TileMap *map) const;

private:
	glm::vec2 pos;
	glm::vec2 vel;
	float speed;
	float jumpSpeed, gravity;
	float tileSize_;
	int maxBombs, activeBombs, bombRange, speedUps;
	int lives;
	float invulnTimer;      // ms remaining of invulnerability (2000 ms after losing a life)
	float vestTimer;        // ms remaining of the Vest power-up invincibility
	bool alive;
	bool onGround;
	bool climbing;
	float coyoteTimer;      // ms remaining where jump is still allowed after leaving ground
	float jumpBufferTimer;  // ms remaining for a buffered jump press before landing
	bool wantsPlaceBomb_;
	bool prevSpaceState;
	bool prevJumpState;
	std::vector<glm::vec2> bombPlatforms_;

	// Rendering / animation
	void updateAnimation(bool moving, int dirX, int dirY);
	Texture spritesheet;
	Sprite *sprite;
	float spriteTileSize_;
	PlayerAnim facing; // Last STAND_* direction, used when the player stops

};


#endif // _PLAYER_INCLUDE
