#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include "Player.h"
#include "TileMap.h"
#include "Game.h"


Player::Player() : sprite(NULL), spriteTileSize_(0.0f), facing(STAND_RIGHT)
{
	pos = glm::vec2(0.0f);
	vel = glm::vec2(0.0f);
	tileSize_ = 16.0f;
	speed = tileSize_ * 0.0055f;       // ~88 px/s for 16px tile
	jumpSpeed = tileSize_ * 0.0158f;   // Initial upward velocity (clears 3 full blocks, < 4 blocks)
	gravity = tileSize_ * 0.000042f;   // Downward acceleration per ms^2
	maxBombs = 1;
	activeBombs = 0;
	bombRange = 3;
	speedUps = 0;
	lives = 3;
	invulnTimer = 0.0f;
	vestTimer = 0.0f;
	alive = true;
	onGround = false;
	climbing = false;
	coyoteTimer = 0.0f;
	jumpBufferTimer = 0.0f;
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
	// Reuse existing spritesheet texture and Sprite VAO/VBO across level reloads
	// if already initialized for the current tile size (zero disk/VRAM/heap churn).
	if(sprite != NULL && spriteTileSize_ == tileSize_)
	{
		facing = STAND_RIGHT;
		sprite->changeAnimation(STAND_RIGHT);
		return;
	}

	if(spritesheet.width() == 0)
	{
		spritesheet.loadFromFile("images/caracters.png", TEXTURE_PIXEL_FORMAT_RGBA);
		spritesheet.setWrapS(GL_CLAMP_TO_EDGE);
		spritesheet.setWrapT(GL_CLAMP_TO_EDGE);
		spritesheet.setMinFilter(GL_NEAREST);
		spritesheet.setMagFilter(GL_NEAREST);
	}

	if(sprite != NULL)
		delete sprite;
	spriteTileSize_ = tileSize_;
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
	const glm::vec2 CLIMB_BACK = cell(140, 1);  // A8: back-facing climbing frame

	sprite->setNumberAnimations(NUM_ANIMS);
	for(int i = 0; i < NUM_ANIMS; i++)
		sprite->setAnimationSpeed(i, 8);

	sprite->addKeyframe(STAND_DOWN, CROUCH);
	sprite->addKeyframe(STAND_UP, CLIMB_BACK);
	sprite->addKeyframe(STAND_LEFT, SIDE_STAND);
	sprite->addKeyframe(STAND_RIGHT, SIDE_STAND, true);

	sprite->addKeyframe(MOVE_DOWN, CROUCH);
	sprite->addKeyframe(MOVE_UP, CLIMB_BACK);

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

	// Vine climbing animations (holding onto vine vs actively climbing up/down)
	sprite->addKeyframe(CLIMB_IDLE, CLIMB_BACK);
	sprite->addKeyframe(CLIMB_MOVE, CLIMB_BACK);
	sprite->addKeyframe(CLIMB_MOVE, CLIMB_BACK, true);

	facing = STAND_RIGHT;
	sprite->changeAnimation(STAND_RIGHT);
}

void Player::updateAnimation(bool moving, int dirX, int dirY)
{
	if(sprite == NULL)
		return;

	if(climbing)
	{
		if(dirY != 0 || dirX != 0)
			sprite->changeAnimation(CLIMB_MOVE);
		else
			sprite->changeAnimation(CLIMB_IDLE);
		return;
	}

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
	// Blinking effect during the 2-second invulnerability window: skip drawing every other 80ms slice
	if(!shouldRenderBlink())
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
	jumpSpeed = tileSize_ * 0.0158f;
	gravity = tileSize_ * 0.000042f;
	maxBombs = 1;
	activeBombs = 0;
	bombRange = 3;
	speedUps = 0;
	lives = 3;
	invulnTimer = 0.0f;
	vestTimer = 0.0f;
	alive = true;
	onGround = false;
	climbing = false;
	coyoteTimer = 0.0f;
	jumpBufferTimer = 0.0f;
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

	// Update invulnerability timer (2000 ms after losing a life)
	if(invulnTimer > 0.0f)
	{
		invulnTimer -= float(dt);
		if(invulnTimer < 0.0f)
			invulnTimer = 0.0f;
	}
	if(vestTimer > 0.0f)
	{
		vestTimer -= float(dt);
		if(vestTimer < 0.0f)
			vestTimer = 0.0f;
	}

	float tSize = float(map->getBlockSize());
	tileSize_ = tSize;
	speed = tSize * (0.0055f + 0.0015f * float(speedUps));

	// Collision box
	glm::vec2 colSize = getHitboxSize();
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, (tSize - colSize.y) * 0.5f);

	// Input directions
	int dirX = 0, dirY = 0;
	if(Game::instance().getKey(GLFW_KEY_LEFT) || Game::instance().getKey(GLFW_KEY_A)) dirX = -1;
	else if(Game::instance().getKey(GLFW_KEY_RIGHT) || Game::instance().getKey(GLFW_KEY_D)) dirX = 1;
	if(Game::instance().getKey(GLFW_KEY_UP) || Game::instance().getKey(GLFW_KEY_W)) dirY = -1;
	else if(Game::instance().getKey(GLFW_KEY_DOWN) || Game::instance().getKey(GLFW_KEY_S)) dirY = 1;

	bool touchingVine = map->isClimbableAt(pos + colOffset, colSize);
	bool currJump = (dirY < 0);

	// Check if there is a climbable vine directly below the player's feet (so pressing Down
	// while standing on top of the vine cap 147 enters climbing mode downwards)
	glm::vec2 belowFeetPos = pos + colOffset + glm::vec2(0.0f, 2.0f);
	bool vineBelow = map->isClimbableAt(belowFeetPos, colSize);

	// Enter climbing state if touching a vine and pressing Up/Down, or pressing Down above a vine
	if((touchingVine && dirY != 0) || (vineBelow && dirY > 0))
	{
		climbing = true;
		onGround = false;
		coyoteTimer = 0.0f;
		jumpBufferTimer = 0.0f;
		vel = glm::vec2(0.0f);
		if(!touchingVine && vineBelow)
			pos.y += 2.0f; // step down onto the vine
	}
	else if(!touchingVine)
	{
		climbing = false;
	}

	if(climbing)
	{
		// Move freely along the vine without gravity
		float climbSpeed = speed * 0.85f;
		float dx = float(dirX) * climbSpeed * float(dt);
		float dy = float(dirY) * climbSpeed * float(dt);

		pos.x += dx;
		if(dx < 0.0f && map->collisionMoveLeft(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + (tileX + 1) * tSize - colOffset.x;
		}
		else if(dx > 0.0f && map->collisionMoveRight(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x + colSize.x - 0.001f - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + tileX * tSize - colSize.x - colOffset.x;
		}

		float prevBottomY = pos.y + colOffset.y + colSize.y;
		pos.y += dy;
		if(dy < 0.0f)
		{
			if(map->collisionMoveUp(pos + colOffset, colSize))
			{
				int tileY = int(std::floor((pos.y + colOffset.y - map->getMinCoords().y) / tSize));
				pos.y = map->getMinCoords().y + (tileY + 1) * tSize - colOffset.y;
			}
			// If the player's feet climbed above the top of the vine cap (147), snap onto its top surface
			int footTileY = int(std::floor((pos.y + colOffset.y + colSize.y - 0.001f - map->getMinCoords().y) / tSize));
			int prevFootTileY = int(std::floor((prevBottomY - 0.001f - map->getMinCoords().y) / tSize));
			int centerTileX = int(std::floor((pos.x + tSize * 0.5f - map->getMinCoords().x) / tSize));
			if(footTileY < prevFootTileY && map->isOneWayPlatform(centerTileX, prevFootTileY) && !map->isClimbable(centerTileX, footTileY))
			{
				pos.y = map->getMinCoords().y + prevFootTileY * tSize - colSize.y - colOffset.y;
				onGround = true;
				climbing = false;
			}
		}
		else if(dy > 0.0f)
		{
			float snappedColY = pos.y + colOffset.y;
			if(!map->isClimbableAt(pos + colOffset, colSize) &&
			   map->collisionMoveDown(pos + colOffset, colSize, &snappedColY, prevBottomY))
			{
				pos.y = snappedColY - colOffset.y;
				onGround = true;
				climbing = false;
			}
			else if(!map->isWalkable(
				int(std::floor((pos.x + tSize * 0.5f - map->getMinCoords().x) / tSize)),
				int(std::floor((pos.y + colOffset.y + colSize.y - 0.001f - map->getMinCoords().y) / tSize))))
			{
				int tileY = int(std::floor((pos.y + colOffset.y + colSize.y - 0.001f - map->getMinCoords().y) / tSize));
				pos.y = map->getMinCoords().y + tileY * tSize - colSize.y - colOffset.y;
				onGround = true;
				climbing = false;
			}
		}

		// If the player moved off the vine, exit climbing state
		if(!map->isClimbableAt(pos + colOffset, colSize))
			climbing = false;
	}
	else
	{
		// Game-feel constants:
		// 1) Coyote Time: allow jumping up to 110 ms after walking off a ledge
		// 2) Jump Buffering: remember a jump press up to 130 ms before landing
		// 3) Halved Gravity Jump Peak: 0.5x gravity near the apex while holding jump
		// 4) Jump Corner Correction: nudge up to ~40% of tile width around ceiling corners
		const float COYOTE_TIME_MS      = 110.0f;
		const float JUMP_BUFFER_MS      = 130.0f;
		const float PEAK_VEL_THRESHOLD  = jumpSpeed * 0.25f;
		const float CORNER_CORRECT_MAX  = tSize * 0.40f;

		// 1. Update Coyote Time timer
		if(onGround)
			coyoteTimer = COYOTE_TIME_MS;
		else if(coyoteTimer > 0.0f)
			coyoteTimer -= float(dt);

		// 2. Update Jump Buffer timer: triggered on fresh jump press, cleared if key is released
		if(currJump && !prevJumpState)
			jumpBufferTimer = JUMP_BUFFER_MS;
		else if(!currJump)
			jumpBufferTimer = 0.0f;
		else if(jumpBufferTimer > 0.0f)
			jumpBufferTimer -= float(dt);

		// Horizontal velocity with smooth inertia
		float targetVx = float(dirX) * speed;
		float accel = (onGround ? 0.018f : 0.010f) * float(dt);
		if(accel > 1.0f) accel = 1.0f;
		vel.x += (targetVx - vel.x) * accel;
		if(std::fabs(vel.x) < 0.001f && dirX == 0)
			vel.x = 0.0f;

		// Execute jump if buffered and within coyote time (or on ground)
		if(jumpBufferTimer > 0.0f && coyoteTimer > 0.0f)
		{
			vel.y = -jumpSpeed;
			onGround = false;
			coyoteTimer = 0.0f;
			jumpBufferTimer = 0.0f;
		}

		// Variable jump height: releasing jump early cuts upward velocity
		if(!currJump && vel.y < -jumpSpeed * 0.45f)
		{
			vel.y = -jumpSpeed * 0.45f;
		}

		// 3. Halved Gravity Jump Peak: apply 50% gravity near the apex while holding jump
		float gravMult = 1.0f;
		if(currJump && !onGround && std::fabs(vel.y) <= PEAK_VEL_THRESHOLD)
		{
			gravMult = 0.5f;
		}
		vel.y += gravity * gravMult * float(dt);
		float maxFall = tSize * 0.018f;
		if(vel.y > maxFall)
			vel.y = maxFall;

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

		// Vertical movement & collision (one-way platforms only collide when falling from above)
		float prevBottomY = pos.y + colOffset.y + colSize.y;
		float dy = vel.y * float(dt);
		pos.y += dy;
		if(dy < 0.0f)
		{
			if(map->collisionMoveUp(pos + colOffset, colSize))
			{
				// 4. Jump Corner Correction: if the player's head clipped the edge of a ceiling
				// tile, try wiggling horizontally up to CORNER_CORRECT_MAX pixels into free space
				// so the jump continues smoothly without losing upward momentum.
				bool corrected = false;
				for(float shift = 1.0f; shift <= CORNER_CORRECT_MAX; shift += 1.0f)
				{
					// Try nudging right (if not pressing left)
					if(dirX >= 0)
					{
						glm::vec2 testRight(pos.x + shift, pos.y);
						if(!map->collisionMoveRight(testRight + colOffset, colSize) &&
						   !map->collisionMoveUp(testRight + colOffset, colSize))
						{
							pos.x += shift;
							corrected = true;
							break;
						}
					}
					// Try nudging left (if not pressing right)
					if(dirX <= 0)
					{
						glm::vec2 testLeft(pos.x - shift, pos.y);
						if(!map->collisionMoveLeft(testLeft + colOffset, colSize) &&
						   !map->collisionMoveUp(testLeft + colOffset, colSize))
						{
							pos.x -= shift;
							corrected = true;
							break;
						}
					}
				}

				if(!corrected)
				{
					int tileY = int(std::floor((pos.y + colOffset.y - map->getMinCoords().y) / tSize));
					pos.y = map->getMinCoords().y + (tileY + 1) * tSize - colOffset.y;
					vel.y = 0.0f;
				}
			}
			onGround = false;
		}
		else
		{
			float snappedColY = pos.y + colOffset.y;
			bool landed = map->collisionMoveDown(pos + colOffset, colSize, &snappedColY, prevBottomY);

			// Pocket Bomberman mechanic: active bombs act as one-way platforms from above
			// so the player can jump onto a placed bomb and bomb-jump to higher ledges.
			float hx = pos.x + colOffset.x;
			float newBottomY = pos.y + colOffset.y + colSize.y;
			for(size_t b = 0; b < bombPlatforms_.size(); b++)
			{
				const glm::vec2 &bp = bombPlatforms_[b];
				if(hx + colSize.x > bp.x + 2.0f && hx < bp.x + tSize - 2.0f &&
				   prevBottomY <= bp.y + 2.5f && newBottomY >= bp.y)
				{
					float candidateY = bp.y - colSize.y;
					if(!landed || candidateY < snappedColY)
					{
						snappedColY = candidateY;
						landed = true;
					}
				}
			}

			if(landed)
			{
				pos.y = snappedColY - colOffset.y;
				vel.y = 0.0f;
				onGround = true;

				// 2. Jump Buffering on exact landing frame: if jump was pressed shortly before
				// touching the ground and is still held, jump immediately on this landing frame!
				if(jumpBufferTimer > 0.0f && currJump)
				{
					vel.y = -jumpSpeed;
					onGround = false;
					coyoteTimer = 0.0f;
					jumpBufferTimer = 0.0f;
				}
			}
			else
			{
				onGround = false;
			}
		}
	}

	prevJumpState = currJump;

	updateAnimation(dirX != 0, dirX, dirY);
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

void Player::setPosition(const glm::vec2 &newPos)
{
	pos = newPos;
}

glm::vec2 Player::getVelocity() const
{
	return vel;
}

void Player::setVelocity(const glm::vec2 &newVel)
{
	vel = newVel;
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

int Player::getLives() const
{
	return lives;
}

bool Player::isInvulnerable() const
{
	return invulnTimer > 0.0f || vestTimer > 0.0f;
}

bool Player::hasVest() const
{
	return vestTimer > 0.0f;
}

float Player::getVestTimer() const
{
	return vestTimer;
}

bool Player::shouldRenderBlink() const
{
	if(!alive)
		return true;
	if(invulnTimer <= 0.0f)
		return true;
	// Toggle visibility every 80 ms (~12.5 Hz blink) during the 2000 ms invulnerability window
	return (int(invulnTimer / 80.0f) % 2) == 0;
}

void Player::takeDamage()
{
	if(!alive || invulnTimer > 0.0f || vestTimer > 0.0f)
		return;

	lives--;
	if(lives <= 0)
	{
		lives = 0;
		alive = false;
	}
	else
	{
		invulnTimer = 2000.0f; // 2 seconds of invulnerability
	}
}

void Player::applyKnockback(const glm::vec2 &impulse)
{
	vel = impulse;
	onGround = false;
	climbing = false;
}

void Player::kill()
{
	lives = 0;
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
	switch(type)
	{
	case 0: // EXTRA_BOMB
		if(maxBombs < MAX_BOMBS_CAP) maxBombs++;
		break;
	case 1: // BOMB_RANGE
		if(bombRange < MAX_RANGE_CAP) bombRange++;
		break;
	case 2: // SPEED
		if(speedUps < MAX_SPEED_UPS) speedUps++;
		speed = tileSize_ * (0.0055f + 0.0015f * float(speedUps));
		break;
	case 3: // EXTRA_LIFE
		if(lives < MAX_LIVES) lives++;
		break;
	case 4: // FULL_FIRE
		bombRange = MAX_RANGE_CAP;
		break;
	case 5: // VEST (10 seconds of invincibility)
		vestTimer = 10000.0f;
		break;
	default:
		break;
	}
}

int Player::getSpeedUps() const
{
	return speedUps;
}

void Player::applyStats(int newLives, int newMaxBombs, int newBombRange, int newSpeedUps)
{
	lives = glm::clamp(newLives, 1, int(MAX_LIVES));
	alive = true;
	maxBombs = glm::clamp(newMaxBombs, 1, int(MAX_BOMBS_CAP));
	bombRange = glm::clamp(newBombRange, 1, int(MAX_RANGE_CAP));
	speedUps = glm::clamp(newSpeedUps, 0, int(MAX_SPEED_UPS));
	speed = tileSize_ * (0.0055f + 0.0015f * float(speedUps));
}

void Player::syncInputLatch()
{
	wantsPlaceBomb_ = false;
	prevSpaceState = Game::instance().getKey(GLFW_KEY_SPACE) || Game::instance().getKey(GLFW_KEY_ENTER);
	prevJumpState = Game::instance().getKey(GLFW_KEY_UP) || Game::instance().getKey(GLFW_KEY_W);
}

void Player::setBombPlatforms(const std::vector<glm::vec2> &platforms)
{
	bombPlatforms_ = platforms;
}

glm::ivec2 Player::getTilePosition(TileMap *map) const
{
	float tSize = float(map->getBlockSize());
	glm::vec2 center = pos + glm::vec2(tSize * 0.5f, tSize * 0.5f);
	return map->worldToTile(center);
}
