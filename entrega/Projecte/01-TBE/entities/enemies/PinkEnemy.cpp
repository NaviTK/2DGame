#include <cmath>
#include <cstdlib>
#include "PinkEnemy.h"
#include "TileMap.h"


// Row 10 of images/caracters.png (253x632): y = 153..168 (height 16 px).
// Using a 15x16 cell size fits all 7 sprites in row 10 with zero bleed into neighbors.
static const float SHEET_W = 253.0f;
static const float SHEET_H = 632.0f;
static const float PINK_CELL_W = 15.0f;
static const float PINK_CELL_H = 16.0f;

static glm::vec2 pinkCell(int x, int y)
{
	return glm::vec2(float(x) / PINK_CELL_W, float(y) / PINK_CELL_H);
}

PinkEnemy::PinkEnemy() : Enemy()
{
	// Custom slim hitbox matching the 13x16 pink creature sprite
	hitboxScale_ = glm::vec2(0.68f, 0.82f);
	spriteFrameSize_ = glm::vec2(PINK_CELL_W, PINK_CELL_H);
	speed = tileSize_ * 0.0018f; // Slow patrol speed
	turnTimer = 0.0f;
}

PinkEnemy::~PinkEnemy()
{
}

void PinkEnemy::init(const glm::vec2 &initialPos, float tileSize)
{
	Enemy::init(initialPos, tileSize);
	hitboxScale_ = glm::vec2(0.68f, 0.82f);
	spriteFrameSize_ = glm::vec2(PINK_CELL_W, PINK_CELL_H);
	speed = tileSize_ * 0.0018f; // Moves slowly along its platform
	turnTimer = 0.0f;

	if(sprite != NULL)
	{
		sprite->changeAnimation((direction == DIR_LEFT) ? MOVE_LEFT : MOVE_RIGHT);
	}
}

void PinkEnemy::initSprite(ShaderProgram &shaderProgram)
{
	// Reuse existing Sprite VAO/VBO across level reloads if tileSize_ is unchanged
	if(sprite != NULL && spriteTileSize_ == tileSize_)
	{
		sprite->changeAnimation((direction == DIR_LEFT) ? MOVE_LEFT : MOVE_RIGHT);
		return;
	}

	Texture &sheet = getSharedSpritesheet();

	if(sprite != NULL)
		delete sprite;
	spriteTileSize_ = tileSize_;

	float quadW = tileSize_ * (PINK_CELL_W / 16.0f);
	float quadH = tileSize_ * (PINK_CELL_H / 16.0f);
	sprite = Sprite::createSprite(glm::vec2(quadW, quadH),
	                              glm::vec2(PINK_CELL_W / SHEET_W, PINK_CELL_H / SHEET_H),
	                              &sheet, &shaderProgram);

	// Row 10 frames (y = 153..168 in images/caracters.png):
	//   Frame 0 (x=1):  standing/looking left
	//   Frame 1 (x=16): standing/looking right
	//   Frame 2 (x=30): walk left step 1
	//   Frame 3 (x=45): walk left step 2
	//   Frame 4 (x=59): front-facing turn pose
	//   Frame 5 (x=75): hit / electrocuted frame 1
	//   Frame 6 (x=92): hit / electrocuted frame 2
	const glm::vec2 F_LOOK_LEFT  = pinkCell(1, 153);
	const glm::vec2 F_LOOK_RIGHT = pinkCell(16, 153);
	const glm::vec2 F_WALK_1     = pinkCell(30, 153);
	const glm::vec2 F_WALK_2     = pinkCell(45, 153);
	const glm::vec2 F_TURN_FRONT = pinkCell(59, 153);
	const glm::vec2 F_DEATH_1    = pinkCell(75, 153);
	const glm::vec2 F_DEATH_2    = pinkCell(92, 153);

	sprite->setNumberAnimations(NUM_ANIMS);
	sprite->setAnimationSpeed(STAND_LEFT, 6);
	sprite->setAnimationSpeed(STAND_RIGHT, 6);
	sprite->setAnimationSpeed(MOVE_LEFT, 6);
	sprite->setAnimationSpeed(MOVE_RIGHT, 6);
	sprite->setAnimationSpeed(TURN_FRONT, 6);
	sprite->setAnimationSpeed(DYING_ANIM, 10);

	sprite->addKeyframe(STAND_LEFT, F_LOOK_LEFT);
	sprite->addKeyframe(STAND_RIGHT, F_LOOK_RIGHT);

	// Walk left cycle
	sprite->addKeyframe(MOVE_LEFT, F_WALK_1);
	sprite->addKeyframe(MOVE_LEFT, F_WALK_2);
	sprite->addKeyframe(MOVE_LEFT, F_WALK_1);
	sprite->addKeyframe(MOVE_LEFT, F_LOOK_LEFT);

	// Walk right cycle (mirrored walk frames + right-looking frame)
	sprite->addKeyframe(MOVE_RIGHT, F_WALK_1, true);
	sprite->addKeyframe(MOVE_RIGHT, F_WALK_2, true);
	sprite->addKeyframe(MOVE_RIGHT, F_WALK_1, true);
	sprite->addKeyframe(MOVE_RIGHT, F_LOOK_RIGHT, false);

	// Turn / front idle pose
	sprite->addKeyframe(TURN_FRONT, F_TURN_FRONT);

	// Death animation (row 10 frames 5 & 6)
	sprite->addKeyframe(DYING_ANIM, F_DEATH_1);
	sprite->addKeyframe(DYING_ANIM, F_DEATH_2);

	sprite->changeAnimation((direction == DIR_LEFT) ? MOVE_LEFT : MOVE_RIGHT);
}

void PinkEnemy::update(int deltaTime, TileMap *map)
{
	if(!isActive())
		return;

	int dt = (deltaTime > 40) ? 40 : deltaTime;

	// Play death animation before deactivating slot in the Object Pool
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
	speed = tSize * 0.0018f; // Slow steady patrol speed

	glm::vec2 colSize = getHitboxSize();
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, tSize - colSize.y);

	// 1. Vertical physics (gravity to rest on solid ground or one-way platform)
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

	// 2. Brief front-facing pause when turning at a platform edge or wall
	if(turnTimer > 0.0f)
	{
		turnTimer -= float(dt);
		if(sprite != NULL)
		{
			sprite->changeAnimation(TURN_FRONT);
			sprite->update(dt);
		}
		return;
	}

	// 3. Platform-bound pathfinding: move slowly and never walk off a platform ledge
	float moveDist = speed * float(dt);
	glm::vec2 minCoords = map->getMinCoords();
	int supportTileY = int(std::floor((pos.y + colOffset.y + colSize.y + 1.0f - minCoords.y) / tSize));

	if(direction == DIR_LEFT)
	{
		float nextLeftX = pos.x + colOffset.x - moveDist;
		int leadTileX = int(std::floor((nextLeftX - 0.5f - minCoords.x) / tSize));

		// Check if there is a wall ahead OR if the platform ends (no floor beneath leading foot)
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
			turnTimer = 140.0f;
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

		// Check if there is a wall ahead OR if the platform ends (no floor beneath leading foot)
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
			turnTimer = 140.0f;
		}
		else
		{
			pos.x += moveDist;
		}
	}

	if(sprite != NULL)
	{
		if(turnTimer > 0.0f)
			sprite->changeAnimation(TURN_FRONT);
		else
			sprite->changeAnimation((direction == DIR_LEFT) ? MOVE_LEFT : MOVE_RIGHT);
		sprite->update(dt);
	}
}

void PinkEnemy::kill()
{
	if(!alive || dying)
		return;
	alive = false;
	dying = true;
	deathTimer = 450.0f; // Play row 10 electrocuted/hit frames for 0.45s
	if(sprite != NULL)
		sprite->changeAnimation(DYING_ANIM);
}

