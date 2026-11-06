#include <cmath>
#include <cstdlib>
#include "BossEnemy.h"
#include "TileMap.h"


// Row 25 of images/caracters.png (253x632): y = 597..628 (32x32 boss frames).
static const float SHEET_W = 253.0f;
static const float SHEET_H = 632.0f;
static const float BOSS_CELL_W = 32.0f;
static const float BOSS_CELL_H = 32.0f;

static glm::vec2 bossCell(int x, int y)
{
	return glm::vec2(float(x) / BOSS_CELL_W, float(y) / BOSS_CELL_H);
}

BossEnemy::BossEnemy() : Enemy()
{
	hitboxScale_ = glm::vec2(1.55f, 1.75f);
	spriteFrameSize_ = glm::vec2(BOSS_CELL_W, BOSS_CELL_H);
	hp_ = BOSS_MAX_HP;
	maxHp_ = BOSS_MAX_HP;
	hitInvulnTimer_ = 0.0f;
	state_ = STATE_PATROL;
	stateTimer_ = 1600.0f;
	nextAttackType_ = 0;
}

BossEnemy::~BossEnemy()
{
}

void BossEnemy::init(const glm::vec2 &initialPos, float tileSize)
{
	Enemy::init(initialPos, tileSize);
	hitboxScale_ = glm::vec2(1.55f, 1.75f);
	spriteFrameSize_ = glm::vec2(BOSS_CELL_W, BOSS_CELL_H);
	hp_ = BOSS_MAX_HP;
	maxHp_ = BOSS_MAX_HP;
	hitInvulnTimer_ = 0.0f;
	state_ = STATE_PATROL;
	stateTimer_ = 1500.0f;
	nextAttackType_ = 0;

	if(sprite != NULL)
		sprite->changeAnimation(WALK_ANIM);
}

void BossEnemy::initSprite(ShaderProgram &shaderProgram)
{
	if(sprite != NULL && spriteTileSize_ == tileSize_)
	{
		sprite->changeAnimation(WALK_ANIM);
		return;
	}

	Texture &sheet = getSharedSpritesheet();

	if(sprite != NULL)
		delete sprite;
	spriteTileSize_ = tileSize_;

	float quadW = tileSize_ * (BOSS_CELL_W / 16.0f);
	float quadH = tileSize_ * (BOSS_CELL_H / 16.0f);
	sprite = Sprite::createSprite(glm::vec2(quadW, quadH),
	                              glm::vec2(BOSS_CELL_W / SHEET_W, BOSS_CELL_H / SHEET_H),
	                              &sheet, &shaderProgram);

	// Row 25 frames (y = 597 in images/caracters.png, 32x32 each):
	//   [2, 597]:   Stance / walk step 1
	//   [35, 597]:  Stance / walk step 2
	//   [68, 597]:  Roar / arms raised attack pose (Jump Slam & Dash Charge)
	//   [102, 597]: Damaged / dazed pose
	const glm::vec2 F_WALK_1 = bossCell(2, 597);
	const glm::vec2 F_WALK_2 = bossCell(35, 597);
	const glm::vec2 F_ATTACK = bossCell(68, 597);
	const glm::vec2 F_HURT   = bossCell(102, 597);

	sprite->setNumberAnimations(NUM_ANIMS);
	sprite->setAnimationSpeed(WALK_ANIM, 5);
	sprite->setAnimationSpeed(ROAR_ATTACK_ANIM, 8);
	sprite->setAnimationSpeed(HURT_DAZED_ANIM, 10);
	sprite->setAnimationSpeed(DYING_ANIM, 12);

	sprite->addKeyframe(WALK_ANIM, F_WALK_1);
	sprite->addKeyframe(WALK_ANIM, F_WALK_2);

	sprite->addKeyframe(ROAR_ATTACK_ANIM, F_ATTACK);
	sprite->addKeyframe(ROAR_ATTACK_ANIM, F_WALK_2);

	sprite->addKeyframe(HURT_DAZED_ANIM, F_HURT);
	sprite->addKeyframe(HURT_DAZED_ANIM, F_ATTACK);

	sprite->addKeyframe(DYING_ANIM, F_HURT);
	sprite->addKeyframe(DYING_ANIM, F_ATTACK);

	sprite->changeAnimation(WALK_ANIM);
}

void BossEnemy::update(int deltaTime, TileMap *map)
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

	if(hitInvulnTimer_ > 0.0f)
	{
		hitInvulnTimer_ -= float(dt);
		if(hitInvulnTimer_ < 0.0f)
			hitInvulnTimer_ = 0.0f;
	}

	float tSize = float(map->getBlockSize());
	tileSize_ = tSize;

	glm::vec2 colSize = getHitboxSize();
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, tSize - colSize.y);

	// Enrage factor: boss attacks faster and moves faster when at or below 2 HP
	bool enraged = (hp_ <= 2);
	float rageMult = enraged ? 1.28f : 1.0f;

	// 1. Boss Attack State Machine
	stateTimer_ -= float(dt);
	float myCenterX = pos.x + tSize * 0.5f;
	float pCenterX = targetPlayerPos_.x + targetPlayerSize_.x * 0.5f;

	if(state_ == STATE_PATROL)
	{
		speed = tSize * 0.0024f * rageMult;
		if(stateTimer_ <= 0.0f && onGround)
		{
			// Lock onto Bomberman and telegraph the next special attack
			if(targetPlayerAlive_)
				direction = (pCenterX < myCenterX) ? DIR_LEFT : DIR_RIGHT;
			state_ = STATE_TELEGRAPH;
			stateTimer_ = enraged ? 280.0f : 400.0f;
		}
	}
	else if(state_ == STATE_TELEGRAPH)
	{
		speed = 0.0f;
		if(stateTimer_ <= 0.0f)
		{
			if(targetPlayerAlive_)
				direction = (pCenterX < myCenterX) ? DIR_LEFT : DIR_RIGHT;

			if(nextAttackType_ == 0)
			{
				// Pattern A: High Jump Slam toward Bomberman
				state_ = STATE_JUMP_SLAM;
				velY = -tSize * 0.0172f;
				onGround = false;
				stateTimer_ = 1100.0f;
			}
			else
			{
				// Pattern B: High-speed horizontal Dash Charge
				state_ = STATE_DASH_CHARGE;
				stateTimer_ = enraged ? 1100.0f : 900.0f;
			}
			nextAttackType_ = (nextAttackType_ + 1) % 2;
		}
	}
	else if(state_ == STATE_JUMP_SLAM)
	{
		speed = tSize * 0.0044f * rageMult;
		if(onGround && stateTimer_ < 850.0f)
		{
			state_ = STATE_PATROL;
			stateTimer_ = enraged ? 1000.0f : 1500.0f;
		}
	}
	else if(state_ == STATE_DASH_CHARGE)
	{
		speed = tSize * 0.0056f * rageMult;
		if(stateTimer_ <= 0.0f)
		{
			state_ = STATE_PATROL;
			stateTimer_ = enraged ? 1000.0f : 1500.0f;
		}
	}

	// 2. Vertical physics (gravity, ceiling, floor)
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
			onGround = true;
		}
		else
		{
			onGround = false;
		}
	}

	// 3. Horizontal movement & wall bounce
	float moveDist = speed * float(dt);
	glm::vec2 minCoords = map->getMinCoords();

	if(moveDist > 0.0f)
	{
		if(direction == DIR_LEFT)
		{
			pos.x -= moveDist;
			if(map->collisionMoveLeft(pos + colOffset, colSize))
			{
				int tileX = int(std::floor((pos.x + colOffset.x - minCoords.x) / tSize));
				pos.x = minCoords.x + (tileX + 1) * tSize - colOffset.x;
				direction = DIR_RIGHT;
			}
		}
		else
		{
			pos.x += moveDist;
			if(map->collisionMoveRight(pos + colOffset, colSize))
			{
				int tileX = int(std::floor((pos.x + colOffset.x + colSize.x - 0.001f - minCoords.x) / tSize));
				pos.x = minCoords.x + tileX * tSize - colSize.x - colOffset.x;
				direction = DIR_LEFT;
			}
		}
	}

	// 4. Update animation
	if(sprite != NULL)
	{
		if(hitInvulnTimer_ > 600.0f)
			sprite->changeAnimation(HURT_DAZED_ANIM);
		else if(state_ == STATE_TELEGRAPH || state_ == STATE_JUMP_SLAM || state_ == STATE_DASH_CHARGE)
			sprite->changeAnimation(ROAR_ATTACK_ANIM);
		else
			sprite->changeAnimation(WALK_ANIM);
		sprite->update(dt);
	}
}

void BossEnemy::render() const
{
	if(!isActive() || sprite == NULL)
		return;

	// Blink rapidly while invulnerable after taking a hit
	if(hitInvulnTimer_ > 0.0f && (int(hitInvulnTimer_ / 65.0f) % 2) == 1)
		return;

	Enemy::render();
}

void BossEnemy::kill()
{
	if(!alive || dying || hitInvulnTimer_ > 0.0f)
		return;

	hp_--;
	if(hp_ <= 0)
	{
		hp_ = 0;
		alive = false;
		dying = true;
		deathTimer = 850.0f;
		if(sprite != NULL)
			sprite->changeAnimation(DYING_ANIM);
	}
	else
	{
		// Survives the blast, flashes invulnerable for 1.1s, and prepares an immediate counter-attack
		hitInvulnTimer_ = 1100.0f;
		state_ = STATE_TELEGRAPH;
		stateTimer_ = 260.0f;
		if(sprite != NULL)
			sprite->changeAnimation(HURT_DAZED_ANIM);
	}
}

void BossEnemy::forceKill()
{
	if(!alive && !dying)
		return;
	hp_ = 0;
	alive = false;
	dying = false;
	deathTimer = 0.0f;
}

