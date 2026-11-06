#include <cmath>
#include <cstdlib>
#include "KirbyEnemy.h"
#include "TileMap.h"


// Row 11 of images/caracters.png (253x632): y = 171..186 (height 16 px).
// Using a 16x16 cell fits all 7 sprites in row 11 with zero bleed into neighbors.
static const float SHEET_W = 253.0f;
static const float SHEET_H = 632.0f;
static const float KIRBY_CELL_W = 16.0f;
static const float KIRBY_CELL_H = 16.0f;

static glm::vec2 kirbyCell(int x, int y)
{
	return glm::vec2(float(x) / KIRBY_CELL_W, float(y) / KIRBY_CELL_H);
}

KirbyEnemy::KirbyEnemy() : Enemy()
{
	// Rounder, wider hitbox matching the 15x16 Kirby-like creature
	hitboxScale_ = glm::vec2(0.80f, 0.80f);
	spriteFrameSize_ = glm::vec2(KIRBY_CELL_W, KIRBY_CELL_H);
	speed = tileSize_ * 0.0022f;
	pauseTimer = 0.0f;
	decisionTimer = 900.0f;
	chasingPlayer = false;
}

KirbyEnemy::~KirbyEnemy()
{
}

void KirbyEnemy::init(const glm::vec2 &initialPos, float tileSize)
{
	Enemy::init(initialPos, tileSize);
	hitboxScale_ = glm::vec2(0.80f, 0.80f);
	spriteFrameSize_ = glm::vec2(KIRBY_CELL_W, KIRBY_CELL_H);
	speed = tileSize_ * 0.0022f;
	pauseTimer = 0.0f;
	decisionTimer = float(700 + (rand() % 700));
	chasingPlayer = false;

	if(sprite != NULL)
	{
		sprite->changeAnimation((direction == DIR_LEFT) ? MOVE_LEFT : MOVE_RIGHT);
	}
}

void KirbyEnemy::initSprite(ShaderProgram &shaderProgram)
{
	if(sprite != NULL && spriteTileSize_ == tileSize_)
	{
		sprite->changeAnimation((direction == DIR_LEFT) ? MOVE_LEFT : MOVE_RIGHT);
		return;
	}

	Texture &sheet = getSharedSpritesheet();

	if(sprite != NULL)
		delete sprite;
	spriteTileSize_ = tileSize_;

	float quadW = tileSize_ * (KIRBY_CELL_W / 16.0f);
	float quadH = tileSize_ * (KIRBY_CELL_H / 16.0f);
	sprite = Sprite::createSprite(glm::vec2(quadW, quadH),
	                              glm::vec2(KIRBY_CELL_W / SHEET_W, KIRBY_CELL_H / SHEET_H),
	                              &sheet, &shaderProgram);

	// Row 11 frames (y = 171..186 in images/caracters.png):
	//   Frame 0 (x=2):   walk left step 1
	//   Frame 1 (x=19):  stand / mid-step left
	//   Frame 2 (x=36):  walk left step 2
	//   Frame 3 (x=52):  front open-mouth alert pose (when spotting player)
	//   Frame 4 (x=69):  front smile pose (when turning)
	//   Frame 5 (x=86):  electrocuted / hit frame 1
	//   Frame 6 (x=104): electrocuted / hit frame 2
	const glm::vec2 F_WALK_1      = kirbyCell(2, 171);
	const glm::vec2 F_STAND_MID   = kirbyCell(19, 171);
	const glm::vec2 F_WALK_2      = kirbyCell(36, 171);
	const glm::vec2 F_ALERT_FRONT = kirbyCell(52, 171);
	const glm::vec2 F_SMILE_FRONT = kirbyCell(69, 171);
	const glm::vec2 F_DEATH_1     = kirbyCell(86, 171);
	const glm::vec2 F_DEATH_2     = kirbyCell(104, 171);

	sprite->setNumberAnimations(NUM_ANIMS);
	sprite->setAnimationSpeed(STAND_LEFT, 6);
	sprite->setAnimationSpeed(STAND_RIGHT, 6);
	sprite->setAnimationSpeed(MOVE_LEFT, 8);
	sprite->setAnimationSpeed(MOVE_RIGHT, 8);
	sprite->setAnimationSpeed(SMILE_FRONT, 6);
	sprite->setAnimationSpeed(ALERT_FRONT, 8);
	sprite->setAnimationSpeed(DYING_ANIM, 10);

	sprite->addKeyframe(STAND_LEFT, F_STAND_MID);
	sprite->addKeyframe(STAND_RIGHT, F_STAND_MID, true);

	// Walk left cycle
	sprite->addKeyframe(MOVE_LEFT, F_WALK_1);
	sprite->addKeyframe(MOVE_LEFT, F_STAND_MID);
	sprite->addKeyframe(MOVE_LEFT, F_WALK_2);
	sprite->addKeyframe(MOVE_LEFT, F_STAND_MID);

	// Walk right cycle (mirrored)
	sprite->addKeyframe(MOVE_RIGHT, F_WALK_1, true);
	sprite->addKeyframe(MOVE_RIGHT, F_STAND_MID, true);
	sprite->addKeyframe(MOVE_RIGHT, F_WALK_2, true);
	sprite->addKeyframe(MOVE_RIGHT, F_STAND_MID, true);

	// Front-facing turn & alert poses
	sprite->addKeyframe(SMILE_FRONT, F_SMILE_FRONT);
	sprite->addKeyframe(ALERT_FRONT, F_ALERT_FRONT);
	sprite->addKeyframe(ALERT_FRONT, F_SMILE_FRONT);

	// Death animation (row 11 frames 5 & 6)
	sprite->addKeyframe(DYING_ANIM, F_DEATH_1);
	sprite->addKeyframe(DYING_ANIM, F_DEATH_2);

	sprite->changeAnimation((direction == DIR_LEFT) ? MOVE_LEFT : MOVE_RIGHT);
}

void KirbyEnemy::update(int deltaTime, TileMap *map)
{
	if(!isActive())
		return;

	int dt = (deltaTime > 40) ? 40 : deltaTime;

	if(dying)
	{
		deathTimer -= float(dt);
		if(sprite != NULL)
		{
			sprite->changeAnimation(DYING_ANIM);
			sprite->update(dt);
		}
		if(deathTimer <= 0.0f)
			dying = false;
		return;
	}

	float tSize = float(map->getBlockSize());
	tileSize_ = tSize;

	glm::vec2 colSize = getHitboxSize();
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, tSize - colSize.y);

	// 1. Vertical physics (gravity to stay grounded on its platform)
	float gravity = tSize * 0.000042f;
	velY += gravity * float(dt);
	float maxFall = tSize * 0.018f;
	if(velY > maxFall)
		velY = maxFall;

	float prevBottomY = pos.y + colOffset.y + colSize.y;
	pos.y += velY * float(dt);
	float snappedColY = pos.y + colOffset.y;
	if(map->collisionMoveDown(pos + colOffset, colSize, &snappedColY, prevBottomY))
	{
		pos.y = snappedColY - colOffset.y;
		velY = 0.0f;
		onGround = true;
	}
	else
	{
		onGround = false;
	}

	// 2. Check if the player is on the same platform height
	float myBottomY = pos.y + colOffset.y + colSize.y;
	float playerBottomY = targetPlayerPos_.y + targetPlayerSize_.y;
	bool samePlatformHeight = targetPlayerAlive_ && onGround &&
	                          (std::fabs(playerBottomY - myBottomY) <= tSize * 0.55f);

	// Periodically decide whether to randomly move towards the player (if on the same platform)
	// or wander/change direction along the platform
	decisionTimer -= float(dt);
	if(decisionTimer <= 0.0f)
	{
		decisionTimer = float(800 + (rand() % 900));
		if(samePlatformHeight)
		{
			// 65% probability to lock onto the player when on the same platform
			if((rand() % 100) < 65)
			{
				float myCenterX = pos.x + tSize * 0.5f;
				float playerCenterX = targetPlayerPos_.x + targetPlayerSize_.x * 0.5f;
				int desiredDir = (playerCenterX < myCenterX) ? DIR_LEFT : DIR_RIGHT;
				if(!chasingPlayer || direction != desiredDir)
				{
					direction = desiredDir;
					pauseTimer = 160.0f;
					if(sprite != NULL)
						sprite->changeAnimation(ALERT_FRONT);
				}
				chasingPlayer = true;
			}
			else
			{
				chasingPlayer = false;
			}
		}
		else
		{
			chasingPlayer = false;
			// Occasional random direction change while wandering
			if((rand() % 100) < 25)
			{
				reverseDirection();
				pauseTimer = 130.0f;
				if(sprite != NULL)
					sprite->changeAnimation(SMILE_FRONT);
			}
		}
	}

	if(!samePlatformHeight)
		chasingPlayer = false;

	// 3. Handle brief turn/alert pause
	if(pauseTimer > 0.0f)
	{
		pauseTimer -= float(dt);
		if(sprite != NULL)
			sprite->update(dt);
		return;
	}

	// Move slightly faster when actively chasing the player on the same platform
	speed = chasingPlayer ? (tSize * 0.0030f) : (tSize * 0.0020f);
	float moveDist = speed * float(dt);
	glm::vec2 minCoords = map->getMinCoords();
	int supportTileY = int(std::floor((pos.y + colOffset.y + colSize.y + 1.0f - minCoords.y) / tSize));

	// 4. Platform-bound movement (never jumps or falls off its platform)
	if(direction == DIR_LEFT)
	{
		float nextLeftX = pos.x + colOffset.x - moveDist;
		int leadTileX = int(std::floor((nextLeftX - 0.5f - minCoords.x) / tSize));

		bool hitWall = map->collisionMoveLeft(glm::vec2(pos.x - moveDist, pos.y) + colOffset, colSize);
		bool atLedge = onGround && !hasFloorAtTile(map, leadTileX, supportTileY);

		if(hitWall || atLedge)
		{
			if(hitWall)
			{
				int tileX = int(std::floor((pos.x + colOffset.x - moveDist - minCoords.x) / tSize));
				pos.x = minCoords.x + (tileX + 1) * tSize - colOffset.x;
			}
			direction = DIR_RIGHT;
			chasingPlayer = false;
			pauseTimer = 140.0f;
			if(sprite != NULL)
				sprite->changeAnimation(SMILE_FRONT);
		}
		else
		{
			pos.x -= moveDist;
		}
	}
	else
	{
		float nextRightX = pos.x + colOffset.x + colSize.x + moveDist;
		int leadTileX = int(std::floor((nextRightX + 0.5f - minCoords.x) / tSize));

		bool hitWall = map->collisionMoveRight(glm::vec2(pos.x + moveDist, pos.y) + colOffset, colSize);
		bool atLedge = onGround && !hasFloorAtTile(map, leadTileX, supportTileY);

		if(hitWall || atLedge)
		{
			if(hitWall)
			{
				int tileX = int(std::floor((pos.x + colOffset.x + colSize.x + moveDist - 0.001f - minCoords.x) / tSize));
				pos.x = minCoords.x + tileX * tSize - colSize.x - colOffset.x;
			}
			direction = DIR_LEFT;
			chasingPlayer = false;
			pauseTimer = 140.0f;
			if(sprite != NULL)
				sprite->changeAnimation(SMILE_FRONT);
		}
		else
		{
			pos.x += moveDist;
		}
	}

	if(sprite != NULL)
	{
		if(pauseTimer <= 0.0f)
			sprite->changeAnimation((direction == DIR_LEFT) ? MOVE_LEFT : MOVE_RIGHT);
		sprite->update(dt);
	}
}

void KirbyEnemy::kill()
{
	if(!alive || dying)
		return;
	alive = false;
	dying = true;
	deathTimer = 450.0f;
	if(sprite != NULL)
		sprite->changeAnimation(DYING_ANIM);
}

