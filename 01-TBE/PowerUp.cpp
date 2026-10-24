#include "PowerUp.h"


PowerUp::PowerUp()
{
	pos = glm::vec2(0.0f);
	tilePos_ = glm::ivec2(0);
	type_ = EXTRA_BOMB;
	collected = false;
}

void PowerUp::init(const glm::ivec2 &tilePos, const glm::vec2 &worldPos, PowerUpType type)
{
	tilePos_ = tilePos;
	pos = worldPos;
	type_ = type;
	collected = false;
}

glm::vec2 PowerUp::getPosition() const
{
	return pos;
}

glm::ivec2 PowerUp::getTilePosition() const
{
	return tilePos_;
}

glm::vec2 PowerUp::getSize() const
{
	return glm::vec2(32.0f, 32.0f);
}

PowerUpType PowerUp::getType() const
{
	return type_;
}

bool PowerUp::isCollected() const
{
	return collected;
}

void PowerUp::collect()
{
	collected = true;
}

