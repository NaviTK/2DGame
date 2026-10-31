#include <cstdlib>
#include <cmath>
#include <GL/glew.h>
#include "Enemy.h"
#include "TileMap.h"


Texture &Enemy::getSharedSpritesheet()
{
	static Texture sharedSheet;
	if(sharedSheet.width() == 0)
	{
		sharedSheet.loadFromFile("images/caracters.png", TEXTURE_PIXEL_FORMAT_RGBA);
		sharedSheet.setWrapS(GL_CLAMP_TO_EDGE);
		sharedSheet.setWrapT(GL_CLAMP_TO_EDGE);
		sharedSheet.setMinFilter(GL_NEAREST);
		sharedSheet.setMagFilter(GL_NEAREST);
	}
	return sharedSheet;
}

Enemy::Enemy()
{
	pos = glm::vec2(0.0f);
	velY = 0.0f;
	tileSize_ = 16.0f;
	speed = tileSize_ * 0.0025f;
	hitboxScale_ = glm::vec2(0.75f, 0.75f);
	direction = (rand() % 2 == 0) ? DIR_LEFT : DIR_RIGHT;
	onGround = false;
	alive = false;
	dying = false;
	deathTimer = 0.0f;
	targetPlayerPos_ = glm::vec2(0.0f);
	targetPlayerSize_ = glm::vec2(12.0f);
	targetPlayerAlive_ = false;
	sprite = NULL;
	spriteTileSize_ = 0.0f;
	spriteFrameSize_ = glm::vec2(16.0f, 16.0f);
}

Enemy::~Enemy()
{
	if(sprite != NULL)
	{
		delete sprite;
		sprite = NULL;
	}
}

void Enemy::init(const glm::vec2 &initialPos, float tileSize)
{
	pos = initialPos;
	velY = 0.0f;
	tileSize_ = tileSize;
	speed = tileSize_ * 0.0025f;
	direction = (rand() % 2 == 0) ? DIR_LEFT : DIR_RIGHT;
	onGround = false;
	alive = true;
	dying = false;
	deathTimer = 0.0f;
}

void Enemy::initSprite(ShaderProgram &shaderProgram)
{
	(void)shaderProgram;
}

void Enemy::setTargetPlayer(const glm::vec2 &playerHitboxPos, const glm::vec2 &playerHitboxSize, bool playerAlive)
{
	targetPlayerPos_ = playerHitboxPos;
	targetPlayerSize_ = playerHitboxSize;
	targetPlayerAlive_ = playerAlive;
}

bool Enemy::hasFloorAtTile(TileMap *map, int tx, int ty) const
{
	return !map->isWalkable(tx, ty) || map->isOneWayPlatform(tx, ty);
}

void Enemy::update(int deltaTime, TileMap *map)
{
	if(!isActive())
		return;

	int dt = (deltaTime > 40) ? 40 : deltaTime;

	if(dying)
	{
		deathTimer -= float(dt);
		if(sprite != NULL)
			sprite->update(dt);
		if(deathTimer <= 0.0f)
			dying = false;
		return;
	}

	float tSize = float(map->getBlockSize());
	tileSize_ = tSize;
	float moveDist = speed * float(dt);

	glm::vec2 colSize = getHitboxSize();
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, tSize - colSize.y);

	// Apply gravity
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

	// Horizontal patrol
	if(direction == DIR_LEFT)
	{
		pos.x -= moveDist;
		if(map->collisionMoveLeft(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + (tileX + 1) * tSize - colOffset.x;
			direction = DIR_RIGHT;
		}
	}
	else
	{
		pos.x += moveDist;
		if(map->collisionMoveRight(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x + colSize.x - 0.001f - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + tileX * tSize - colSize.x - colOffset.x;
			direction = DIR_LEFT;
		}
	}

	if(sprite != NULL)
		sprite->update(dt);
}

void Enemy::render() const
{
	if(!isActive() || sprite == NULL)
		return;

	float quadW = tileSize_ * (spriteFrameSize_.x / 16.0f);
	float quadH = tileSize_ * (spriteFrameSize_.y / 16.0f);
	glm::vec2 hPos = getHitboxPos();
	glm::vec2 hSize = getHitboxSize();

	// Center horizontally on the enemy's tile and align feet with the bottom of the hitbox
	float drawX = pos.x + (tileSize_ - quadW) * 0.5f;
	float drawY = (hPos.y + hSize.y) - quadH;
	sprite->setPosition(glm::vec2(drawX, drawY));
	sprite->render();
}

glm::vec2 Enemy::getPosition() const
{
	return pos;
}

void Enemy::setPosition(const glm::vec2 &newPos)
{
	pos = newPos;
}

glm::vec2 Enemy::getSize() const
{
	return glm::vec2(tileSize_, tileSize_);
}

glm::vec2 Enemy::getHitboxPos() const
{
	glm::vec2 hSize = getHitboxSize();
	// Horizontally centered in the tile, bottom-aligned with the floor of the tile
	return pos + glm::vec2((tileSize_ - hSize.x) * 0.5f, tileSize_ - hSize.y);
}

glm::vec2 Enemy::getHitboxSize() const
{
	return glm::vec2(tileSize_ * hitboxScale_.x, tileSize_ * hitboxScale_.y);
}

void Enemy::setHitboxScale(const glm::vec2 &scale)
{
	hitboxScale_ = scale;
}

void Enemy::reverseDirection()
{
	direction = (direction == DIR_LEFT) ? DIR_RIGHT : DIR_LEFT;
}

int Enemy::getDirection() const
{
	return direction;
}

bool Enemy::isActive() const
{
	return alive || dying;
}

void Enemy::deactivate()
{
	alive = false;
	dying = false;
	deathTimer = 0.0f;
}

bool Enemy::isAlive() const
{
	return alive && !dying;
}

void Enemy::kill()
{
	alive = false;
	dying = false;
}

glm::ivec2 Enemy::getTilePosition(TileMap *map) const
{
	glm::vec2 center = getHitboxPos() + getHitboxSize() * 0.5f;
	return map->worldToTile(center);
}
