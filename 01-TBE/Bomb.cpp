#include "Bomb.h"


Bomb::Bomb()
{
	pos = glm::vec2(0.0f);
	tilePos_ = glm::ivec2(0);
	timer = 2500.0f; // 2.5 seconds
	range = 1;
	exploded = false;
	processed = false;
}

void Bomb::init(const glm::ivec2 &tilePos, const glm::vec2 &worldPos, int bombRange)
{
	tilePos_ = tilePos;
	pos = worldPos;
	range = bombRange;
	timer = 2500.0f;
	exploded = false;
	processed = false;
}

void Bomb::update(int deltaTime)
{
	if(exploded)
		return;

	timer -= float(deltaTime);
	if(timer <= 0.0f)
	{
		exploded = true;
	}
}

glm::vec2 Bomb::getPosition() const
{
	return pos;
}

glm::ivec2 Bomb::getTilePosition() const
{
	return tilePos_;
}

glm::vec2 Bomb::getSize() const
{
	return glm::vec2(32.0f, 32.0f);
}

int Bomb::getRange() const
{
	return range;
}

bool Bomb::hasExploded() const
{
	return exploded;
}

bool Bomb::isProcessed() const
{
	return processed;
}

void Bomb::explode()
{
	timer = 0.0f;
	exploded = true;
}

void Bomb::markProcessed()
{
	processed = true;
}

