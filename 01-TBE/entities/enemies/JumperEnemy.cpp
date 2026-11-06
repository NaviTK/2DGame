#include <cmath>
#include <cstdlib>
#include "JumperEnemy.h"
#include "TileMap.h"


// Row 15 of images/caracters.png (253x632): y = 243..258 (16x16 cells).
static const float SHEET_W = 253.0f;
static const float SHEET_H = 632.0f;
static const float JUMPER_CELL_W = 16.0f;
static const float JUMPER_CELL_H = 16.0f;

static glm::vec2 jumperCell(int x, int y)
{
	return glm::vec2(float(x) / JUMPER_CELL_W, float(y) / JUMPER_CELL_H);
}

JumperEnemy::JumperEnemy() : Enemy()
{
	hitboxScale_ = glm::vec2(0.76f, 0.78f);
	spriteFrameSize_ = glm::vec2(JUMPER_CELL_W, JUMPER_CELL_H);
	speed = tileSize_ * 0.0028f;
	jumpTimer_ = 650.0f;
	landTimer_ = 0.0f;
	jumpCycle_ = 0;
	wasOnGround_ = false;
}

JumperEnemy::~JumperEnemy()
{
}

void JumperEnemy::init(const glm::vec2 &initialPos, float tileSize)
{
	Enemy::init(initialPos, tileSize);
	hitboxScale_ = glm::vec2(0.76f, 0.78f);
	spriteFrameSize_ = glm::vec2(JUMPER_CELL_W, JUMPER_CELL_H);
	speed = tileSize_ * 0.0028f;
	jumpTimer_ = float(450 + (rand() % 450));
	landTimer_ = 0.0f;
	jumpCycle_ = rand() % 3;
	wasOnGround_ = false;

	if(sprite != NULL)
		sprite->changeAnimation((direction == DIR_LEFT) ? WALK_LEFT : WALK_RIGHT);
}

void JumperEnemy::initSprite(ShaderProgram &shaderProgram)
{
	if(sprite != NULL && spriteTileSize_ == tileSize_)
	{
		sprite->changeAnimation((direction == DIR_LEFT) ? WALK_LEFT : WALK_RIGHT);
		return;
	}

	Texture &sheet = getSharedSpritesheet();

	if(sprite != NULL)
		delete sprite;
	spriteTileSize_ = tileSize_;

	float quadW = tileSize_ * (JUMPER_CELL_W / 16.0f);
	float quadH = tileSize_ * (JUMPER_CELL_H / 16.0f);
	sprite = Sprite::createSprite(glm::vec2(quadW, quadH),
	                              glm::vec2(JUMPER_CELL_W / SHEET_W, JUMPER_CELL_H / SHEET_H),
	                              &sheet, &shaderProgram);

	// Row 15 frames (y = 243 in images/caracters.png):
	//   [2, 243]:   Walk / glide step 1
	//   [20, 243]:  Walk / glide step 2
	//   [38, 243]:  Vertical spring jump frame 1
	//   [56, 243]:  Vertical spring jump frame 2
	//   [74, 243]:  Landing squash pose
	//   [92, 243]:  Defeated / hit frame 1
	//   [110, 243]: Defeated / hit frame 2
	const glm::vec2 F_WALK_1  = jumperCell(2, 243);
	const glm::vec2 F_WALK_2  = jumperCell(20, 243);
	const glm::vec2 F_JUMP_1  = jumperCell(38, 243);
	const glm::vec2 F_JUMP_2  = jumperCell(56, 243);
	const glm::vec2 F_LAND    = jumperCell(74, 243);
	const glm::vec2 F_DEATH_1 = jumperCell(92, 243);
	const glm::vec2 F_DEATH_2 = jumperCell(110, 243);

	sprite->setNumberAnimations(NUM_ANIMS);
	sprite->setAnimationSpeed(WALK_LEFT, 8);
	sprite->setAnimationSpeed(WALK_RIGHT, 8);
	sprite->setAnimationSpeed(JUMP_LEFT, 10);
	sprite->setAnimationSpeed(JUMP_RIGHT, 10);
	sprite->setAnimationSpeed(LAND_ANIM, 8);
	sprite->setAnimationSpeed(DYING_ANIM, 10);

	sprite->addKeyframe(WALK_LEFT, F_WALK_1);
	sprite->addKeyframe(WALK_LEFT, F_WALK_2);

	sprite->addKeyframe(WALK_RIGHT, F_WALK_1, true);
	sprite->addKeyframe(WALK_RIGHT, F_WALK_2, true);

	sprite->addKeyframe(JUMP_LEFT, F_JUMP_1);
	sprite->addKeyframe(JUMP_LEFT, F_JUMP_2);

	sprite->addKeyframe(JUMP_RIGHT, F_JUMP_1, true);
	sprite->addKeyframe(JUMP_RIGHT, F_JUMP_2, true);

	sprite->addKeyframe(LAND_ANIM, F_LAND);

	sprite->addKeyframe(DYING_ANIM, F_DEATH_1);
	sprite->addKeyframe(DYING_ANIM, F_DEATH_2);

	sprite->changeAnimation((direction == DIR_LEFT) ? WALK_LEFT : WALK_RIGHT);
}

void JumperEnemy::update(int deltaTime, TileMap *map)
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

	// 1. Periodic jump decision when grounded
	if(onGround)
	{
		if(landTimer_ > 0.0f)
			landTimer_ -= float(dt);

		jumpTimer_ -= float(dt);
		if(jumpTimer_ <= 0.0f)
		{
			jumpCycle_ = (jumpCycle_ + 1) % 3;
			// Alternating jump height: short hop vs high leap over obstacles
			float jumpImpulse = (jumpCycle_ == 1) ? (tSize * 0.0155f) : (tSize * 0.0118f);
			velY = -jumpImpulse;
			onGround = false;

			// On every 3rd jump, unpredictably flip direction or hop toward player
			if(jumpCycle_ == 2)
			{
				if(targetPlayerAlive_ && (rand() % 100) < 60)
				{
					float myCenterX = pos.x + tSize * 0.5f;
					float pCenterX = targetPlayerPos_.x + targetPlayerSize_.x * 0.5f;
					direction = (pCenterX < myCenterX) ? DIR_LEFT : DIR_RIGHT;
				}
				else
				{
					reverseDirection();
				}
			}

			jumpTimer_ = float(550 + (rand() % 500));
		}
	}

	// 2. Vertical physics (gravity, ceiling collision, landing)
	float gravity = tSize * 0.000040f;
	velY += gravity * float(dt);
	float maxFall = tSize * 0.018f;
	if(velY > maxFall)
		velY = maxFall;

	float prevBottomY = pos.y + colOffset.y + colSize.y;
	pos.y += velY * float(dt);

	if(velY < 0.0f)
	{
		if(map->collisionMoveUp(pos + colOffset, colSize))
		{
			int tileY = int(std::floor((pos.y + colOffset.y - map->getMinCoords().y) / tSize));
			pos.y = map->getMinCoords().y + (tileY + 1) * tSize - colOffset.y;
			velY = 0.0f;
		}
		onGround = false;
	}
	else
	{
		float snappedColY = pos.y + colOffset.y;
		if(map->collisionMoveDown(pos + colOffset, colSize, &snappedColY, prevBottomY))
		{
			pos.y = snappedColY - colOffset.y;
			velY = 0.0f;
			if(!wasOnGround_)
				landTimer_ = 110.0f;
			onGround = true;
		}
		else
		{
			onGround = false;
		}
	}
	wasOnGround_ = onGround;

	// 3. Horizontal movement (faster while airborne to arc over obstacles)
	speed = onGround ? (tSize * 0.0026f) : (tSize * 0.0034f);
	float moveDist = speed * float(dt);
	glm::vec2 minCoords = map->getMinCoords();

	if(direction == DIR_LEFT)
	{
		pos.x -= moveDist;
		if(map->collisionMoveLeft(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x - minCoords.x) / tSize));
			pos.x = minCoords.x + (tileX + 1) * tSize - colOffset.x;
			// If grounded against a wall/block, try jumping over it before reversing
			if(onGround && jumpTimer_ > 150.0f)
			{
				velY = -tSize * 0.0148f;
				onGround = false;
				jumpTimer_ = 650.0f;
			}
			else
			{
				direction = DIR_RIGHT;
			}
		}
	}
	else
	{
		pos.x += moveDist;
		if(map->collisionMoveRight(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x + colSize.x - 0.001f - minCoords.x) / tSize));
			pos.x = minCoords.x + tileX * tSize - colSize.x - colOffset.x;
			if(onGround && jumpTimer_ > 150.0f)
			{
				velY = -tSize * 0.0148f;
				onGround = false;
				jumpTimer_ = 650.0f;
			}
			else
			{
				direction = DIR_LEFT;
			}
		}
	}

	// 4. Update animation state
	if(sprite != NULL)
	{
		if(!onGround)
			sprite->changeAnimation((direction == DIR_LEFT) ? JUMP_LEFT : JUMP_RIGHT);
		else if(landTimer_ > 0.0f)
			sprite->changeAnimation(LAND_ANIM);
		else
			sprite->changeAnimation((direction == DIR_LEFT) ? WALK_LEFT : WALK_RIGHT);
		sprite->update(dt);
	}
}

void JumperEnemy::kill()
{
	if(!alive || dying)
		return;
	alive = false;
	dying = true;
	deathTimer = 450.0f;
	if(sprite != NULL)
		sprite->changeAnimation(DYING_ANIM);
}

