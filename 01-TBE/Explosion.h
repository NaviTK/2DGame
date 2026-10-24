#ifndef _EXPLOSION_INCLUDE
#define _EXPLOSION_INCLUDE


#include <vector>
#include <glm/glm.hpp>


using namespace std;


class TileMap;


// An Explosion spreads from a center tile in four cardinal directions
// up to a given range. It stops at indestructible walls and destroys
// destructible blocks (stopping after each one).


class Explosion
{

public:
	Explosion();

	void init(const glm::ivec2 &center, int range, TileMap *map);
	void update(int deltaTime);

	bool isFinished() const;
	const vector<glm::ivec2> &getAffectedTiles() const;
	const vector<glm::ivec2> &getDestroyedBlocks() const;

private:
	glm::ivec2 center_;
	float timer;
	bool finished;
	vector<glm::ivec2> affectedTiles;
	vector<glm::ivec2> destroyedBlocks_;

};


#endif // _EXPLOSION_INCLUDE

