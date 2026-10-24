#include "Explosion.h"
#include "TileMap.h"


Explosion::Explosion()
{
	center_ = glm::ivec2(0);
	timer = 600.0f; // 0.6 seconds flame duration
	finished = false;
}

void Explosion::init(const glm::ivec2 &center, int range, TileMap *map)
{
	center_ = center;
	timer = 600.0f;
	finished = false;
	affectedTiles.clear();
	destroyedBlocks_.clear();

	affectedTiles.push_back(center);

	const glm::ivec2 dirs[4] = {
		glm::ivec2(0, -1), // UP
		glm::ivec2(0, 1),  // DOWN
		glm::ivec2(-1, 0), // LEFT
		glm::ivec2(1, 0)   // RIGHT
	};

	for(int d = 0; d < 4; d++)
	{
		for(int step = 1; step <= range; step++)
		{
			glm::ivec2 target = center + dirs[d] * step;

			if(target.x < 0 || target.x >= map->getMapWidth() ||
			   target.y < 0 || target.y >= map->getMapHeight())
				break;

			int tile = map->getTile(target.x, target.y);

			if(tile == 1) // Indestructible wall
			{
				break;
			}
			else if(tile == 2) // Destructible block
			{
				affectedTiles.push_back(target);
				destroyedBlocks_.push_back(target);
				break; // Flame stops at destructible block
			}
			else
			{
				affectedTiles.push_back(target);
			}
		}
	}
}

void Explosion::update(int deltaTime)
{
	if(finished)
		return;

	timer -= float(deltaTime);
	if(timer <= 0.0f)
	{
		finished = true;
	}
}

bool Explosion::isFinished() const
{
	return finished;
}

const vector<glm::ivec2> &Explosion::getAffectedTiles() const
{
	return affectedTiles;
}

const vector<glm::ivec2> &Explosion::getDestroyedBlocks() const
{
	return destroyedBlocks_;
}

