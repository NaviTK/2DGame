#include <algorithm>
#include "Explosion.h"
#include "Bomb.h"
#include "TileMap.h"
#include "ShaderProgram.h"


static const float EXP_STEP_MS = 100.0f; // 0.1s per tile expansion step (like the vine)
static const float EXP_HOLD_MS = 350.0f; // Hold full flame for 0.35s after reaching max step

Explosion::Explosion()
{
	center_ = glm::ivec2(0);
	range_ = 3;
	maxArmSteps_ = 0;
	visibleStep_ = 0;
	stepTimer_ = 0.0f;
	holdTimer_ = EXP_HOLD_MS;
	animTimer_ = 0.0f;
	active = false;
	finished = false;
	vineToggled_ = false;

	// Pre-allocate vectors once in the Object Pool constructor to avoid heap reallocations
	for(int d = 0; d < 4; d++)
	{
		armTiles_[d].reserve(8);
		armHitsDestructible_[d] = false;
		armBlockDestroyed_[d] = false;
	}
	affectedTiles.reserve(32);
	newlyDestroyedBlocks_.reserve(8);
}

void Explosion::init(const glm::ivec2 &center, int range, TileMap *map)
{
	center_ = center;
	range_ = range;
	maxArmSteps_ = 0;
	visibleStep_ = 0;
	stepTimer_ = 0.0f;
	holdTimer_ = EXP_HOLD_MS;
	animTimer_ = 0.0f;
	active = true;
	finished = false;
	vineToggled_ = false;

	affectedTiles.clear();
	newlyDestroyedBlocks_.clear();

	const glm::ivec2 dirs[4] = {
		glm::ivec2(0, -1), // DIR_UP
		glm::ivec2(0, 1),  // DIR_DOWN
		glm::ivec2(-1, 0), // DIR_LEFT
		glm::ivec2(1, 0)   // DIR_RIGHT
	};

	for(int d = 0; d < 4; d++)
	{
		armTiles_[d].clear();
		armHitsDestructible_[d] = false;
		armBlockDestroyed_[d] = false;

		for(int step = 1; step <= range_; step++)
		{
			glm::ivec2 target = center + dirs[d] * step;

			if(target.x < 0 || target.x >= map->getMapWidth() ||
			   target.y < 0 || target.y >= map->getMapHeight())
				break;

			if(map->isDestructible(target.x, target.y))
			{
				// Flame reaches and destroys this grass block, then stops expanding in this direction
				armTiles_[d].push_back(target);
				armHitsDestructible_[d] = true;
				break;
			}
			else if(!map->isWalkable(target.x, target.y)) // Indestructible solid wall/platform
			{
				break;
			}
			else
			{
				armTiles_[d].push_back(target);
			}
		}

		if(int(armTiles_[d].size()) > maxArmSteps_)
			maxArmSteps_ = int(armTiles_[d].size());
	}

	// If the center tile itself is destructible, queue it immediately
	if(map->isDestructible(center.x, center.y))
		newlyDestroyedBlocks_.push_back(center);

	rebuildVisibleTiles();
}

void Explosion::rebuildVisibleTiles()
{
	affectedTiles.clear();
	affectedTiles.push_back(center_);

	for(int d = 0; d < 4; d++)
	{
		int count = std::min(visibleStep_, int(armTiles_[d].size()));
		for(int s = 0; s < count; s++)
		{
			affectedTiles.push_back(armTiles_[d][s]);
		}

		// If the expanding flame just reached the destructible block at the end of arm d,
		// queue it to be destroyed on this exact step.
		if(armHitsDestructible_[d] && !armBlockDestroyed_[d] &&
		   visibleStep_ >= int(armTiles_[d].size()) && !armTiles_[d].empty())
		{
			armBlockDestroyed_[d] = true;
			newlyDestroyedBlocks_.push_back(armTiles_[d].back());
		}
	}
}

void Explosion::update(int deltaTime)
{
	if(!active || finished)
		return;

	float dt = float(deltaTime);
	animTimer_ += dt;

	// Progressive step-by-step expansion (0.1s per tile outward, like the vine)
	if(visibleStep_ < maxArmSteps_)
	{
		stepTimer_ += dt;
		bool advanced = false;
		while(stepTimer_ >= EXP_STEP_MS && visibleStep_ < maxArmSteps_)
		{
			stepTimer_ -= EXP_STEP_MS;
			visibleStep_++;
			advanced = true;
		}
		if(advanced)
			rebuildVisibleTiles();
	}
	else
	{
		holdTimer_ -= dt;
		if(holdTimer_ <= 0.0f)
		{
			finished = true;
		}
	}
}

void Explosion::render(ShaderProgram &texProgram, TileMap *map) const
{
	if(!active || finished || map == NULL)
		return;

	float tSize = float(map->getBlockSize());
	float halfT = tSize * 0.5f;

	// Cycle through the 3 flame thickness frames (0 -> 1 -> 2) in rows 4, 5, 6 of caracters.png
	int f = int(animTimer_ / 90.0f) % 3;

	// Source X coordinates in images/caracters.png for each of the 3 animation frames:
	static const int CENTER_X[3]    = { 47, 65, 83 };   // y = 89, 16x16
	static const int VERT_MID_X[3]  = { 48, 65, 83 };   // y = 79, 16x8
	static const int UP_TIP_X[3]    = { 48, 65, 83 };   // y = 70, 16x8
	static const int DOWN_TIP_X[3]  = { 48, 65, 83 };   // y = 107, 16x8
	static const int HORIZ_MID_X[3] = { 17, 27, 37 };   // y = 71, 8x16
	static const int LEFT_TIP_X[3]  = { 17, 28, 37 };   // y = 89, 8x16
	static const int RIGHT_TIP_X[3] = { 101, 111, 121 }; // y = 89, 8x16

	// 1. Render Center tile (row 5: y = 89, 16x16)
	glm::vec2 centerWorld = map->tileToWorld(center_);
	Bomb::drawSheetRect(texProgram, centerWorld, glm::vec2(tSize, tSize), CENTER_X[f], 89, 16, 16);

	// 2. Render each directional arm up to visibleStep_
	for(int d = 0; d < 4; d++)
	{
		int count = std::min(visibleStep_, int(armTiles_[d].size()));
		for(int s = 0; s < count; s++)
		{
			bool isTip = (s == count - 1);
			glm::vec2 wPos = map->tileToWorld(armTiles_[d][s]);

			if(d == DIR_UP)
			{
				if(isTip)
				{
					// Top half = UpTip (y=70, 16x8), Bottom half = VertMid (y=79, 16x8)
					Bomb::drawSheetRect(texProgram, wPos, glm::vec2(tSize, halfT), UP_TIP_X[f], 70, 16, 8);
					Bomb::drawSheetRect(texProgram, wPos + glm::vec2(0.0f, halfT), glm::vec2(tSize, halfT), VERT_MID_X[f], 79, 16, 8);
				}
				else
				{
					Bomb::drawSheetRect(texProgram, wPos, glm::vec2(tSize, halfT), VERT_MID_X[f], 79, 16, 8);
					Bomb::drawSheetRect(texProgram, wPos + glm::vec2(0.0f, halfT), glm::vec2(tSize, halfT), VERT_MID_X[f], 79, 16, 8);
				}
			}
			else if(d == DIR_DOWN)
			{
				if(isTip)
				{
					// Top half = VertMid (y=79, 16x8), Bottom half = DownTip (y=107, 16x8)
					Bomb::drawSheetRect(texProgram, wPos, glm::vec2(tSize, halfT), VERT_MID_X[f], 79, 16, 8);
					Bomb::drawSheetRect(texProgram, wPos + glm::vec2(0.0f, halfT), glm::vec2(tSize, halfT), DOWN_TIP_X[f], 107, 16, 8);
				}
				else
				{
					Bomb::drawSheetRect(texProgram, wPos, glm::vec2(tSize, halfT), VERT_MID_X[f], 79, 16, 8);
					Bomb::drawSheetRect(texProgram, wPos + glm::vec2(0.0f, halfT), glm::vec2(tSize, halfT), VERT_MID_X[f], 79, 16, 8);
				}
			}
			else if(d == DIR_LEFT)
			{
				if(isTip)
				{
					// Left half = LeftTip (y=89, 8x16), Right half = HorizMid (y=71, 8x16)
					Bomb::drawSheetRect(texProgram, wPos, glm::vec2(halfT, tSize), LEFT_TIP_X[f], 89, 8, 16);
					Bomb::drawSheetRect(texProgram, wPos + glm::vec2(halfT, 0.0f), glm::vec2(halfT, tSize), HORIZ_MID_X[f], 71, 8, 16);
				}
				else
				{
					Bomb::drawSheetRect(texProgram, wPos, glm::vec2(halfT, tSize), HORIZ_MID_X[f], 71, 8, 16);
					Bomb::drawSheetRect(texProgram, wPos + glm::vec2(halfT, 0.0f), glm::vec2(halfT, tSize), HORIZ_MID_X[f], 71, 8, 16);
				}
			}
			else if(d == DIR_RIGHT)
			{
				if(isTip)
				{
					// Left half = HorizMid (y=71, 8x16), Right half = RightTip (y=89, 8x16)
					Bomb::drawSheetRect(texProgram, wPos, glm::vec2(halfT, tSize), HORIZ_MID_X[f], 71, 8, 16);
					Bomb::drawSheetRect(texProgram, wPos + glm::vec2(halfT, 0.0f), glm::vec2(halfT, tSize), RIGHT_TIP_X[f], 89, 8, 16);
				}
				else
				{
					Bomb::drawSheetRect(texProgram, wPos, glm::vec2(halfT, tSize), HORIZ_MID_X[f], 71, 8, 16);
					Bomb::drawSheetRect(texProgram, wPos + glm::vec2(halfT, 0.0f), glm::vec2(halfT, tSize), HORIZ_MID_X[f], 71, 8, 16);
				}
			}
		}
	}
}

bool Explosion::isActive() const
{
	return active;
}

void Explosion::deactivate()
{
	active = false;
}

bool Explosion::isFinished() const
{
	return finished;
}

glm::ivec2 Explosion::getCenterTile() const
{
	return center_;
}

bool Explosion::hasToggledVine() const
{
	return vineToggled_;
}

void Explosion::markVineToggled()
{
	vineToggled_ = true;
}

const vector<glm::ivec2> &Explosion::getAffectedTiles() const
{
	return affectedTiles;
}

void Explosion::popNewlyDestroyedBlocks(vector<glm::ivec2> &outBlocks)
{
	outBlocks.clear();
	for(size_t i = 0; i < newlyDestroyedBlocks_.size(); i++)
		outBlocks.push_back(newlyDestroyedBlocks_[i]);
	newlyDestroyedBlocks_.clear();
}
