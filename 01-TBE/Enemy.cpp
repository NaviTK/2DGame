#include <cstdlib>
#include <cmath>
#include "Enemy.h"
#include "TileMap.h"


Enemy::Enemy()
{
	pos = glm::vec2(0.0f);
	velY = 0.0f;
	tileSize_ = 16.0f;
	speed = tileSize_ * 0.0035f;
	direction = (rand() % 2 == 0) ? 2 : 3;
	dirTimer = float(1500 + rand() % 2000);
	alive = true;
}

void Enemy::init(const glm::vec2 &initialPos, float tileSize)
{
	pos = initialPos;
	velY = 0.0f;
	tileSize_ = tileSize;
	speed = tileSize_ * 0.0035f;
	direction = (rand() % 2 == 0) ? 2 : 3;
	dirTimer = float(1500 + rand() % 2000);
	alive = true;
}

void Enemy::update(int deltaTime, TileMap *map)
{
	if(!alive)
		return;

	int dt = (deltaTime > 40) ? 40 : deltaTime;
	float tSize = float(map->getBlockSize());
	tileSize_ = tSize;
	float moveDist = speed * float(dt);

	// Collision box: 75% of block size, centered within the corridor
	glm::vec2 colSize(tSize * 0.75f, tSize * 0.75f);
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, (tSize - colSize.y) * 0.5f);

	// Apply gravity
	float gravity = tSize * 0.000042f;
	velY += gravity * float(dt);
	float maxFall = tSize * 0.018f;
	if(velY > maxFall)
		velY = maxFall;

	pos.y += velY * float(dt);
	if(map->collisionMoveDown(pos + colOffset, colSize))
	{
		int tileY = int(std::floor((pos.y + colOffset.y + colSize.y - 0.001f - map->getMinCoords().y) / tSize));
		pos.y = map->getMinCoords().y + tileY * tSize - colSize.y - colOffset.y;
		velY = 0.0f;
	}

	dirTimer -= float(dt);
	if(dirTimer <= 0.0f)
	{
		direction = (rand() % 2 == 0) ? 2 : 3;
		dirTimer = float(1500 + rand() % 2000);
	}

	// Horizontal patrol: 2=left, 3=right
	if(direction == 2)
	{
		pos.x -= moveDist;
		if(map->collisionMoveLeft(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + (tileX + 1) * tSize - colOffset.x;
			direction = 3;
		}
	}
	else
	{
		pos.x += moveDist;
		if(map->collisionMoveRight(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x + colSize.x - 0.001f - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + tileX * tSize - colSize.x - colOffset.x;
			direction = 2;
		}
	}
}

glm::vec2 Enemy::getPosition() const
{
	return pos;
}

glm::vec2 Enemy::getSize() const
{
	return glm::vec2(tileSize_, tileSize_);
}

bool Enemy::isAlive() const
{
	return alive;
}

void Enemy::kill()
{
	alive = false;
}

glm::ivec2 Enemy::getTilePosition(TileMap *map) const
{
	float tSize = float(map->getBlockSize());
	glm::vec2 center = pos + glm::vec2(tSize * 0.5f, tSize * 0.5f);
	return map->worldToTile(center);
}
