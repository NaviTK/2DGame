#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include "Player.h"
#include "TileMap.h"
#include "Game.h"


Player::Player()
{
	pos = glm::vec2(0.0f);
	tileSize_ = 16.0f;
	speed = tileSize_ * 0.005f; // Scales with tile size (~80 px/s for 16px tile)
	maxBombs = 1;
	activeBombs = 0;
	bombRange = 1;
	alive = true;
	wantsPlaceBomb_ = false;
	prevSpaceState = false;
}

void Player::init(const glm::vec2 &initialPos, float tileSize)
{
	pos = initialPos;
	tileSize_ = tileSize;
	speed = tileSize_ * 0.005f;
	maxBombs = 1;
	activeBombs = 0;
	bombRange = 1;
	alive = true;
	wantsPlaceBomb_ = false;
	prevSpaceState = false;
}

void Player::update(int deltaTime, TileMap *map)
{
	if(!alive)
		return;

	float tSize = float(map->getBlockSize());
	tileSize_ = tSize;
	float moveDist = speed * float(deltaTime);

	// Collision box: matches visual quad dimensions
	glm::vec2 colSize = getHitboxSize();
	glm::vec2 colOffset((tSize - colSize.x) * 0.5f, (tSize - colSize.y) * 0.5f);

	// Horizontal movement
	if(Game::instance().getKey(GLFW_KEY_LEFT) || Game::instance().getKey(GLFW_KEY_A))
	{
		pos.x -= moveDist;
		if(map->collisionMoveLeft(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + (tileX + 1) * tSize - colOffset.x;

			// Corner sliding assistance: align with nearest row corridor
			int nearestRow = int((pos.y + tSize * 0.5f - map->getMinCoords().y) / tSize);
			float idealY = map->getMinCoords().y + nearestRow * tSize;
			float diffY = idealY - pos.y;
			if(fabs(diffY) <= tSize * 0.45f)
			{
				float slide = (diffY > 0 ? 1.0f : -1.0f) * moveDist;
				if(fabs(slide) > fabs(diffY)) slide = diffY;
				pos.y += slide;
			}
		}
	}
	else if(Game::instance().getKey(GLFW_KEY_RIGHT) || Game::instance().getKey(GLFW_KEY_D))
	{
		pos.x += moveDist;
		if(map->collisionMoveRight(pos + colOffset, colSize))
		{
			int tileX = int(std::floor((pos.x + colOffset.x + colSize.x - 0.001f - map->getMinCoords().x) / tSize));
			pos.x = map->getMinCoords().x + tileX * tSize - colSize.x - colOffset.x;

			// Corner sliding assistance: align with nearest row corridor
			int nearestRow = int((pos.y + tSize * 0.5f - map->getMinCoords().y) / tSize);
			float idealY = map->getMinCoords().y + nearestRow * tSize;
			float diffY = idealY - pos.y;
			if(fabs(diffY) <= tSize * 0.45f)
			{
				float slide = (diffY > 0 ? 1.0f : -1.0f) * moveDist;
				if(fabs(slide) > fabs(diffY)) slide = diffY;
				pos.y += slide;
			}
		}
	}

	// Vertical movement
	if(Game::instance().getKey(GLFW_KEY_UP) || Game::instance().getKey(GLFW_KEY_W))
	{
		pos.y -= moveDist;
		if(map->collisionMoveUp(pos + colOffset, colSize))
		{
			int tileY = int(std::floor((pos.y + colOffset.y - map->getMinCoords().y) / tSize));
			pos.y = map->getMinCoords().y + (tileY + 1) * tSize - colOffset.y;

			// Corner sliding assistance: align with nearest column corridor
			int nearestCol = int((pos.x + tSize * 0.5f - map->getMinCoords().x) / tSize);
			float idealX = map->getMinCoords().x + nearestCol * tSize;
			float diffX = idealX - pos.x;
			if(fabs(diffX) <= tSize * 0.45f)
			{
				float slide = (diffX > 0 ? 1.0f : -1.0f) * moveDist;
				if(fabs(slide) > fabs(diffX)) slide = diffX;
				pos.x += slide;
			}
		}
	}
	else if(Game::instance().getKey(GLFW_KEY_DOWN) || Game::instance().getKey(GLFW_KEY_S))
	{
		pos.y += moveDist;
		if(map->collisionMoveDown(pos + colOffset, colSize))
		{
			int tileY = int(std::floor((pos.y + colOffset.y + colSize.y - 0.001f - map->getMinCoords().y) / tSize));
			pos.y = map->getMinCoords().y + tileY * tSize - colSize.y - colOffset.y;

			// Corner sliding assistance: align with nearest column corridor
			int nearestCol = int((pos.x + tSize * 0.5f - map->getMinCoords().x) / tSize);
			float idealX = map->getMinCoords().x + nearestCol * tSize;
			float diffX = idealX - pos.x;
			if(fabs(diffX) <= tSize * 0.45f)
			{
				float slide = (diffX > 0 ? 1.0f : -1.0f) * moveDist;
				if(fabs(slide) > fabs(diffX)) slide = diffX;
				pos.x += slide;
			}
		}
	}

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

