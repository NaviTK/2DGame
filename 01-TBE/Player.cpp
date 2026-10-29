#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include "Player.h"
#include "TileMap.h"
#include "Game.h"


Player::Player() : sprite(NULL), facing(STAND_RIGHT)
{
	pos = glm::vec2(0.0f);
	vel = glm::vec2(0.0f);
	tileSize_ = 16.0f;
	speed = tileSize_ * 0.0055f;       // ~88 px/s for 16px tile
	jumpSpeed = tileSize_ * 0.0155f;   // Initial upward velocity (~2.8 tiles high jump)
	gravity = tileSize_ * 0.000042f;   // Downward acceleration per ms^2
	maxBombs = 1;
	activeBombs = 0;
	bombRange = 1;
	alive = true;
	onGround = false;
	wantsPlaceBomb_ = false;
	prevSpaceState = false;
	prevJumpState = false;
}

Player::~Player()
{
	if(sprite != NULL)
		delete sprite;
}

// Sprite cells inside images/caracters.png (253x632). Frames are not on a regular
// grid, so each one is a fixed 17x17 cell (bottom-aligned, centered on the sprite).
static const float SHEET_W = 253.0f, SHEET_H = 632.0f;
static const float FRAME_SIZE = 17.0f;

static glm::vec2 cell(int x, int y)
{
	// Sprite::addKeyframe expects the displacement in frame units
	return glm::vec2(float(x) / FRAME_SIZE, float(y) / FRAME_SIZE);
}

void Player::initSprite(ShaderProgram &shaderProgram)
{
	spritesheet.loadFromFile("images/caracters.png", TEXTURE_PIXEL_FORMAT_RGBA);
	spritesheet.setWrapS(GL_CLAMP_TO_EDGE);
	spritesheet.setWrapT(GL_CLAMP_TO_EDGE);
	spritesheet.setMinFilter(GL_NEAREST);
	spritesheet.setMagFilter(GL_NEAREST);

	if(sprite != NULL)
		delete sprite;
	// The 17px frame is drawn slightly bigger than the 16px tile (scaled to tile size)
	float quad = tileSize_ * FRAME_SIZE / 16.0f;
	sprite = Sprite::createSprite(glm::vec2(quad, quad),
	                              glm::vec2(FRAME_SIZE / SHEET_W, FRAME_SIZE / SHEET_H),
	                              &spritesheet, &shaderProgram);

	// First row frames (Pocket Bomberman platformer sprites, top-left of each 17x17 cell)
	const glm::vec2 SIDE_STAND = cell(3, 1);    // A0: standing facing left
	const glm::vec2 SIDE_WALK1 = cell(19, 1);   // A1: walk step 1
	const glm::vec2 SIDE_WALK2 = cell(36, 1);   // A2: walk step 2
	const glm::vec2 CROUCH     = cell(52, 1);   // A3: crouch / prepare jump
	const glm::vec2 JUMP_UP    = cell(68, 1);   // A4: jumping up
	const glm::vec2 JUMP_FALL  = cell(86, 1);   // A5: airborne / falling
	const glm::vec2 BACK       = cell(140, 1);  // A8: back-facing frame

	sprite->setNumberAnimations(NUM_ANIMS);
	for(int i = 0; i < NUM_ANIMS; i++)
		sprite->setAnimationSpeed(i, 8);

	sprite->addKeyframe(STAND_DOWN, CROUCH);
	sprite->addKeyframe(STAND_UP, BACK);
	sprite->addKeyframe(STAND_LEFT, SIDE_STAND);
	sprite->addKeyframe(STAND_RIGHT, SIDE_STAND, true);

	sprite->addKeyframe(MOVE_DOWN, CROUCH);
	sprite->addKeyframe(MOVE_UP, BACK);

	sprite->addKeyframe(MOVE_LEFT, SIDE_STAND);
	sprite->addKeyframe(MOVE_LEFT, SIDE_WALK1);
	sprite->addKeyframe(MOVE_LEFT, SIDE_STAND);
	sprite->addKeyframe(MOVE_LEFT, SIDE_WALK2);

	// Right = left frames mirrored
	sprite->addKeyframe(MOVE_RIGHT, SIDE_STAND, true);
	sprite->addKeyframe(MOVE_RIGHT, SIDE_WALK1, true);
	sprite->addKeyframe(MOVE_RIGHT, SIDE_STAND, true);
	sprite->addKeyframe(MOVE_RIGHT, SIDE_WALK2, true);

	// Airborne animations from row 1
	sprite->addKeyframe(JUMP_LEFT, JUMP_UP);
	sprite->addKeyframe(JUMP_RIGHT, JUMP_UP, true);
	sprite->addKeyframe(FALL_LEFT, JUMP_FALL);
	sprite->addKeyframe(FALL_RIGHT, JUMP_FALL, true);

	facing = STAND_RIGHT;
	sprite->changeAnimation(STAND_RIGHT);
}

void Player::updateAnimation(bool moving, int dirX)
{
	if(sprite == NULL)
		return;

	if(dirX < 0)
		facing = STAND_LEFT;
	else if(dirX > 0)
		facing = STAND_RIGHT;

	bool lookRight = (facing == STAND_RIGHT);

	if(!onGround)
	{
		if(vel.y < 0.0f)
			sprite->changeAnimation(lookRight ? JUMP_RIGHT : JUMP_LEFT);
		else
			sprite->changeAnimation(lookRight ? FALL_RIGHT : FALL_LEFT);
	}
	else if(moving)
	{
		sprite->changeAnimation(lookRight ? MOVE_RIGHT : MOVE_LEFT);
	}
	else
	{
		sprite->changeAnimation(facing);
	}
}

void Player::render()
{
	if(sprite == NULL)
		return;
	// Bottom-aligned with the hitbox bottom and horizontally centered on the player's tile
	float quad = tileSize_ * FRAME_SIZE / 16.0f;
	glm::vec2 hPos = getHitboxPos();
	glm::vec2 hSize = getHitboxSize();
	float drawX = pos.x - (quad - tileSize_) * 0.5f;
	float drawY = (hPos.y + hSize.y) - quad;
	sprite->setPosition(glm::vec2(drawX, drawY));
	sprite->render();
}

void Player::init(const glm::vec2 &initialPos, float tileSize)
{
	pos = initialPos;
	vel = glm::vec2(0.0f);
	tileSize_ = tileSize;
	speed = tileSize_ * 0.0055f;
	jumpSpeed = tileSize_ * 0.0155f;
	gravity = tileSize_ * 0.000042f;
	maxBombs = 1;
	activeBombs = 0;
	bombRange = 1;
	alive = true;
	onGround = false;
	wantsPlaceBomb_ = false;
	prevSpaceState = false;
	prevJumpState = false;
}

void Player::update(int deltaTime, TileMap *map)
{
	if(!alive)
		return;

	// Cap deltaTime to avoid huge physics steps on window drag / lag spikes
	int dt = (deltaTime > 40) ? 40 : deltaTime;

	float tSize = float(map->getBlockSize());
	tileSize_ = tSize;

	// Input direction & horizontal velocity with smooth inertia
	int dirX = 0;
	if(Game::instance().getKey(GLFW_KEY_LEFT) || Game::instance().getKey(GLFW_KEY_A)) dirX = -1;
	else if(Game::instance().getKey(GLFW_KEY_RIGHT) || Game::instance().getKey(GLFW_KEY_D)) dirX = 1;

	float targetVx = float(dirX) * speed;
	float accel = (onGround ? 0.018f : 0.010f) * float(dt);
	if(accel > 1.0f) accel = 1.0f;
	vel.x += (targetVx - vel.x) * accel;
	if(std::fabs(vel.x) < 0.001f && dirX == 0)
		vel.x = 0.0f;

	// Jump input (Up / W)
	bool currJump = Game::instance().getKey(GLFW_KEY_UP) || Game::instance().getKey(GLFW_KEY_W);
	if(currJump && !prevJumpState && onGround)
	{
		vel.y = -jumpSpeed;
		onGround = false;
	}
	// Variable jump height: releasing jump early cuts upward velocity
	if(!currJump && vel.y < -jumpSpeed * 0.45f)
	{
		vel.y = -jumpSpeed * 0.45f;
	}
	prevJumpState = currJump;

	// Apply gravity
	vel.y += gravity * float(dt);
	float maxFall = tSize * 0.018f;
	if(vel.y > maxFall)
		vel.y = maxFall;

	// Collision box
	glm::vec2 colSize = getHitboxSize();
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, (tSize - colSize.y) * 0.5f);

	// Horizontal movement & collision
	float dx = vel.x * float(dt);
	pos.x += dx;
	if(dx < 0.0f && map->collisionMoveLeft(pos + colOffset, colSize))
	{
		int tileX = int(std::floor((pos.x + colOffset.x - map->getMinCoords().x) / tSize));
		pos.x = map->getMinCoords().x + (tileX + 1) * tSize - colOffset.x;
		vel.x = 0.0f;
	}
	else if(dx > 0.0f && map->collisionMoveRight(pos + colOffset, colSize))
	{
		int tileX = int(std::floor((pos.x + colOffset.x + colSize.x - 0.001f - map->getMinCoords().x) / tSize));
		pos.x = map->getMinCoords().x + tileX * tSize - colSize.x - colOffset.x;
		vel.x = 0.0f;
	}

	// Vertical movement & collision (gravity + jump)
	float dy = vel.y * float(dt);
	pos.y += dy;
	if(dy < 0.0f)
	{
		if(map->collisionMoveUp(pos + colOffset, colSize))
		{
			int tileY = int(std::floor((pos.y + colOffset.y - map->getMinCoords().y) / tSize));
			pos.y = map->getMinCoords().y + (tileY + 1) * tSize - colOffset.y;
			vel.y = 0.0f;
		}
		onGround = false;
	}
	else
	{
		if(map->collisionMoveDown(pos + colOffset, colSize))
		{
			int tileY = int(std::floor((pos.y + colOffset.y + colSize.y - 0.001f - map->getMinCoords().y) / tSize));
			pos.y = map->getMinCoords().y + tileY * tSize - colSize.y - colOffset.y;
			vel.y = 0.0f;
			onGround = true;
		}
		else
		{
			onGround = false;
		}
	}

	updateAnimation(dirX != 0, dirX);
	if(sprite != NULL)
		sprite->update(dt);

	// Bomb placement input
	bool currSpace = Game::instance().getKey(GLFW_KEY_SPACE) || Game::instance().getKey(GLFW_KEY_ENTER);
	if(currSpace && !prevSpaceState)
	{
		if(activeBombs < maxBombs)
			wantsPlaceBomb_ = true;
	}
	prevSpaceState = currSpace;
}

glm::vec2 Player::getPosition() const
{
	return pos;
}

glm::vec2 Player::getSize() const
{
	return glm::vec2(tileSize_, tileSize_);
}

glm::vec2 Player::getHitboxPos() const
{
	glm::vec2 hSize = getHitboxSize();
	return pos + glm::vec2((tileSize_ - hSize.x) * 0.5f, (tileSize_ - hSize.y) * 0.5f);
}

glm::vec2 Player::getHitboxSize() const
{
	return glm::vec2(tileSize_ * 0.75f, tileSize_ * 0.75f);
}

bool Player::isAlive() const
{
	return alive;
}

void Player::kill()
{
	alive = false;
}

bool Player::wantsToPlaceBomb() const
{
	return wantsPlaceBomb_;
}

void Player::bombPlaced()
{
	wantsPlaceBomb_ = false;
	activeBombs++;
}

void Player::bombExploded()
{
	if(activeBombs > 0)
		activeBombs--;
}

int Player::getBombRange() const
{
	return bombRange;
}

int Player::getMaxBombs() const
{
	return maxBombs;
}

int Player::getActiveBombs() const
{
	return activeBombs;
}

void Player::addPowerUp(int type)
{
	if(type == 0) // EXTRA_BOMB
		maxBombs++;
	else if(type == 1) // BOMB_RANGE
		bombRange++;
	else if(type == 2) // SPEED
		speed += tileSize_ * 0.0015f;
}

glm::ivec2 Player::getTilePosition(TileMap *map) const
{
	float tSize = float(map->getBlockSize());
	glm::vec2 center = pos + glm::vec2(tSize * 0.5f, tSize * 0.5f);
	return map->worldToTile(center);
}
