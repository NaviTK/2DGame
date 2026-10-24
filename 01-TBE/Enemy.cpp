#include <cstdlib>
#include <cmath>
#include "Enemy.h"
#include "TileMap.h"


Enemy::Enemy()
{
	pos = glm::vec2(0.0f);
	tileSize_ = 16.0f;
	speed = tileSize_ * 0.0035f;
	direction = rand() % 4;
	dirTimer = float(1000 + rand() % 2000);
	alive = true;
}

void Enemy::init(const glm::vec2 &initialPos, float tileSize)
{
	pos = initialPos;
	tileSize_ = tileSize;
	speed = tileSize_ * 0.0035f;
	direction = rand() % 4;
	dirTimer = float(1000 + rand() % 2000);
	alive = true;
}

void Enemy::update(int deltaTime, TileMap *map)
{
	if(!alive)
		return;

	float tSize = float(map->getBlockSize());
	tileSize_ = tSize;
	float moveDist = speed * float(deltaTime);

	// Collision box: 75% of block size, centered within the corridor
	glm::vec2 colSize(tSize * 0.75f, tSize * 0.75f);
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, (tSize - colSize.y) * 0.5f);
	bool collided = false;

	dirTimer -= float(deltaTime);
	if(dirTimer <= 0.0f)
	{
		direction = rand() % 4;
		dirTimer = float(1000 + rand() % 2000);
	}

	// Move according to current direction: 0=up, 1=down, 2=left, 3=right
	switch(direction)
	{
	case 0: // UP
	{
		pos.y -= moveDist;
		int curCol = int(std::floor((pos.x + tSize * 0.5f - map->getMinCoords().x) / tSize));
		pos.x = map->getMinCoords().x + curCol * tSize;
		if(map->collisionMoveUp(pos + colOffset, colSize))
		{
			int tileY = int(std::floor((pos.y + colOffset.y - map->getMinCoords().y) / tSize));
			pos.y = map->getMinCoords().y + (tileY + 1) * tSize - colOffset.y;
			collided = true;
		}
		break;
	}
	case 1: // DOWN
	{
		pos.y += moveDist;
		int curCol = int(std::floor((pos.x + tSize * 0.5f - map->getMinCoords().x) / tSize));
		pos.x = map->getMinCoords().x + curCol * tSize;
		if(map->collisionMoveDown(pos + colOffset, colSize))
		{
			int tileY = int(std::floor((pos.y + colOffset.y + colSize.y - 0.001f - map->getMinCoords().y) / tSize));
			pos.y = map->getMinCoords().y + tileY * tSize - colSize.y - colOffset.y;
			collided = true;
		}
		break;
	}
	case 2: // LEFT
	{
		pos.x -= moveDist;
		int curRow = int(std::floor((pos.y + tSize * 0.5f - map->getMinCoords().y) / tSize));
		pos.y = map->getMinCoords().y + curRow * tSize;
		if(map->collisionMoveLeft(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + (tileX + 1) * tSize - colOffset.x;
			collided = true;
		}
		break;
	}
	case 3: // RIGHT
	{
		pos.x += moveDist;
		int curRow = int(std::floor((pos.y + tSize * 0.5f - map->getMinCoords().y) / tSize));
		pos.y = map->getMinCoords().y + curRow * tSize;
		if(map->collisionMoveRight(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x + colSize.x - 0.001f - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + tileX * tSize - colSize.x - colOffset.x;
			collided = true;
		}
		break;
	}
	}

	if(collided)
	{
		direction = rand() % 4;
		dirTimer = float(800 + rand() % 1500);
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

