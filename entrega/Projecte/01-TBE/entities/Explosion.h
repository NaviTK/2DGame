#ifndef _EXPLOSION_INCLUDE
#define _EXPLOSION_INCLUDE


#include <vector>
#include <glm/glm.hpp>


using namespace std;


class TileMap;
class ShaderProgram;


// An Explosion spreads progressively from a center tile in four cardinal directions
// (0 = UP, 1 = DOWN, 2 = LEFT, 3 = RIGHT) up to a given range (default 3 tiles),
// advancing one tile every 0.1s (similar to the vine animation).
// It stops at indestructible walls and destroys destructible grass blocks
// (stopping further expansion in that direction after destroying the block).
// Renders directional explosion sprites from rows 4, 5, 6 of images/caracters.png.
class Explosion
{

public:
	enum ArmDir { DIR_UP = 0, DIR_DOWN = 1, DIR_LEFT = 2, DIR_RIGHT = 3 };

	Explosion();

	void init(const glm::ivec2 &center, int range, TileMap *map);
	void update(int deltaTime);
	void render(ShaderProgram &texProgram, TileMap *map) const;

	bool isActive() const;
	void deactivate();
	bool isFinished() const;
	glm::ivec2 getCenterTile() const;
	bool hasToggledVine() const;
	void markVineToggled();

	// Returns all tiles currently covered by the expanding flame (up to visibleStep_)
	const vector<glm::ivec2> &getAffectedTiles() const;

	// Returns destructible blocks newly reached by the flame on this update step,
	// clearing the pending list so each block is destroyed once when the flame hits it.
	void popNewlyDestroyedBlocks(vector<glm::ivec2> &outBlocks);

private:
	void rebuildVisibleTiles();

private:
	glm::ivec2 center_;
	int range_;
	int maxArmSteps_;      // Max steps reached by any of the 4 arms
	int visibleStep_;      // Current expansion step (0 = center only, 1..maxArmSteps_)
	float stepTimer_;      // Timer (ms) for progressive tile-by-tile expansion (100 ms/step)
	float holdTimer_;      // Timer (ms) to hold the full explosion before finishing
	float animTimer_;      // Total elapsed time (ms) for cycling the 3 flame intensity frames
	bool active;
	bool finished;
	bool vineToggled_;

	// Pre-computed arm tiles for each of the 4 cardinal directions
	vector<glm::ivec2> armTiles_[4];
	bool armHitsDestructible_[4];
	bool armBlockDestroyed_[4];

	vector<glm::ivec2> affectedTiles;
	vector<glm::ivec2> newlyDestroyedBlocks_;

};


#endif // _EXPLOSION_INCLUDE
