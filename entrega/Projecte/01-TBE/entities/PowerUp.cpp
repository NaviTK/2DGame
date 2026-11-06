#include <cmath>
#include <cstdlib>
#include "PowerUp.h"
#include "Bomb.h"
#include "ShaderProgram.h"


static const float POWERUP_GRACE_MS = 700.0f; // Longer than the explosion hold time (350 ms)


PowerUp::PowerUp()
{
	pos = glm::vec2(0.0f);
	tilePos_ = glm::ivec2(0);
	type_ = EXTRA_BOMB;
	animTimer_ = 0.0f;
	graceTimer_ = 0.0f;
	active = false;
	collected = false;
}

void PowerUp::init(const glm::ivec2 &tilePos, const glm::vec2 &worldPos, PowerUpType type)
{
	tilePos_ = tilePos;
	pos = worldPos;
	type_ = type;
	animTimer_ = 0.0f;
	graceTimer_ = POWERUP_GRACE_MS;
	active = true;
	collected = false;
}

void PowerUp::update(int deltaTime)
{
	if(!active || collected)
		return;
	animTimer_ += float(deltaTime);
	if(graceTimer_ > 0.0f)
		graceTimer_ -= float(deltaTime);
}

void PowerUp::render(ShaderProgram &texProgram, float tileSize) const
{
	if(!active || collected)
		return;

	// Soft floating bob (1 px amplitude at 16 px tiles) like the Game Boy original
	float bob = std::floor(std::sin(animTimer_ * 0.006f) * (tileSize / 16.0f) + 0.5f);
	glm::ivec2 src = getSheetCoords(type_);

	// Flash white quickly during the spawn grace period
	if(graceTimer_ > 0.0f && (int(graceTimer_ / 70.0f) % 2) == 0)
		texProgram.setUniform4f("color", 1.6f, 1.6f, 1.6f, 1.0f);
	else
		texProgram.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);

	Bomb::drawSheetRect(texProgram, pos + glm::vec2(0.0f, bob), glm::vec2(tileSize, tileSize), src.x, src.y, 16, 16);
	texProgram.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
}

glm::ivec2 PowerUp::getSheetCoords(PowerUpType type)
{
	// Item panels in images/caracters.png: x = 21 + 18 * column, y = 117 (row A) / 135 (row B)
	switch(type)
	{
	case EXTRA_BOMB: return glm::ivec2(21, 117);  // Bomb
	case BOMB_RANGE: return glm::ivec2(39, 117);  // Fire face
	case SPEED_UP:   return glm::ivec2(57, 117);  // Roller skate
	case VEST:       return glm::ivec2(75, 117);  // Vest
	case EXTRA_LIFE: return glm::ivec2(93, 117);  // Heart
	case FULL_FIRE:  return glm::ivec2(57, 135);  // "F" panel
	default:         return glm::ivec2(21, 117);
	}
}

PowerUpType PowerUp::rollRandomType(bool allowSpeedUp)
{
	// Drop table restricted strictly to the 4 required power-ups:
	// 1. Bomb Up (EXTRA_BOMB)
	// 2. Fire Up (BOMB_RANGE)
	// 3. Cor / Heart Shield (HEART_SHIELD)
	// 4. Speed Up / Roller Skate (SPEED_UP) — excluded once speed cap is reached
	if(!allowSpeedUp)
	{
		int roll = rand() % 100;
		if(roll < 37) return EXTRA_BOMB;
		if(roll < 74) return BOMB_RANGE;
		return HEART_SHIELD;
	}

	int roll = rand() % 100;
	if(roll < 28) return EXTRA_BOMB;
	if(roll < 56) return BOMB_RANGE;
	if(roll < 80) return SPEED_UP;
	return HEART_SHIELD;
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
	return glm::vec2(16.0f, 16.0f);
}

PowerUpType PowerUp::getType() const
{
	return type_;
}

bool PowerUp::isActive() const
{
	return active;
}

void PowerUp::deactivate()
{
	active = false;
}

bool PowerUp::isCollected() const
{
	return collected;
}

void PowerUp::collect()
{
	collected = true;
	active = false;
}

bool PowerUp::canBeBurned() const
{
	return graceTimer_ <= 0.0f;
}
