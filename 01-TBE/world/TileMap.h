#ifndef _TILE_MAP_INCLUDE
#define _TILE_MAP_INCLUDE


#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Texture.h"
#include "ShaderProgram.h"


using namespace std;


// Semantic tile types so gameplay logic queries tile qualities (sky, solid ground/wall,
// destructible grass, climbable vine, exit door) instead of raw sprite numbers.
// Any spriteId from the tilesheet can be associated with any TileType for visual variety.
enum TileType
{
	TILE_EMPTY = 0,     // Empty cell (used on upper layers 1 and 2 when nothing is drawn there)
	TILE_SKY,           // Walkable background/sky tile
	TILE_SOLID,         // Solid ground, wall, or platform block (blocks movement on all sides)
	TILE_DESTRUCTIBLE,  // Destructible grass/bush block (one-way platform from above, destroyed by bombs)
	TILE_VINE_HEAD,     // Vine source block (one-way platform from above, toggles vine when hit by bomb)
	TILE_VINE_STEM,     // Climbable vine stem tile
	TILE_EXIT_DOOR      // Level exit door tile
};


// A single cell in the 3D tile matrix [layer][y][x], pairing visual spriteId with semantic TileType.
struct TileCell
{
	int spriteId;   // Tile index in tilesheet (-1 if empty/not drawn)
	TileType type;  // Semantic behavior of this tile
};


// Enemy spawn entry parsed from the level configuration file (e.g. "ENEMY_SPAWN KIRBY 12 14")
struct EnemySpawnInfo
{
	string type;        // Enemy type identifier ("PINK", "KIRBY", etc.)
	glm::ivec2 tilePos; // Tile coordinates (x, y)
};


class TileMap
{

public:
	static const int NUM_LAYERS = 3; // Layer 0 (base), Layer 1 (mid), Layer 2 (top)

private:
	TileMap(const string &levelFile, const glm::vec2 &minCoords, ShaderProgram &program);

public:
	// Tile maps can only be created inside an OpenGL context
	static TileMap *createTileMap(const string &levelFile, const glm::vec2 &minCoords, ShaderProgram &program);

	~TileMap();

	void update(int deltaTime);
	void render() const;
	void free();

	// 3D Tile matrix accessors (layer in 0..2, x in 0..width-1, y in 0..height-1)
	const TileCell &getCell(int layer, int x, int y) const;
	void setCell(int layer, int x, int y, int spriteId, TileType type);
	void setCellFromSprite(int layer, int x, int y, int spriteId);

	// Semantic tile queries by TileType (independent of raw sprite numbers)
	TileType getTileType(int x, int y, int layer = 0) const;
	bool hasTileType(int x, int y, TileType type) const;
	int getTile(int x, int y, int layer = 0) const;

	bool isWalkable(int x, int y) const;
	bool isOneWayPlatform(int x, int y) const;
	bool isClimbable(int x, int y) const;
	bool isClimbableAt(const glm::vec2 &pos, const glm::vec2 &size) const;
	bool isDestructible(int x, int y) const;
	bool isExitDoor(int x, int y) const;

	void destroyTile(int x, int y);
	bool toggleVineAtHead(int x, int y);

	// Maps a raw spriteId from the tileset to its default semantic TileType
	TileType classifySpriteId(int spriteId) const;

	// Getters
	int getMapWidth() const;
	int getMapHeight() const;
	int getTileSize() const;
	int getBlockSize() const;
	glm::vec2 getMinCoords() const;
	void setMinCoords(const glm::vec2 &minCoords, ShaderProgram &program);

	// Coordinate conversion
	glm::ivec2 worldToTile(const glm::vec2 &worldPos) const;
	glm::vec2 tileToWorld(const glm::ivec2 &tilePos) const;

	// Spawn points parsed from level tags (PLAYER_SPAWN x y, ENEMY_SPAWN TYPE x y)
	const vector<glm::ivec2> &getPlayerSpawns() const;
	const vector<EnemySpawnInfo> &getEnemySpawns() const;

	// Collision detection for entity movement
	bool collisionMoveLeft(const glm::vec2 &pos, const glm::vec2 &size) const;
	bool collisionMoveRight(const glm::vec2 &pos, const glm::vec2 &size) const;
	bool collisionMoveUp(const glm::vec2 &pos, const glm::vec2 &size) const;
	bool collisionMoveDown(const glm::vec2 &pos, const glm::vec2 &size, float *posY = NULL, float prevBottomY = -1e9f) const;

	bool collisionMoveLeft(const glm::vec2 &pos, const glm::ivec2 &size) const { return collisionMoveLeft(pos, glm::vec2(size)); }
	bool collisionMoveRight(const glm::vec2 &pos, const glm::ivec2 &size) const { return collisionMoveRight(pos, glm::vec2(size)); }
	bool collisionMoveUp(const glm::vec2 &pos, const glm::ivec2 &size) const { return collisionMoveUp(pos, glm::vec2(size)); }
	bool collisionMoveDown(const glm::vec2 &pos, const glm::ivec2 &size, float *posY = NULL, float prevBottomY = -1e9f) const { return collisionMoveDown(pos, glm::vec2(size), posY, prevBottomY); }

private:
	bool loadLevel(const string &levelFile);
	void prepareArrays(const glm::vec2 &minCoords, ShaderProgram &program);
	int cellIndex(int layer, int x, int y) const;

private:
	GLuint vao;
	GLuint vbo;
	GLint posLocation, texCoordLocation;
	int nTiles;
	glm::ivec2 position, mapSize, tilesheetSize;
	int tileSize, blockSize;
	Texture tilesheet;
	glm::vec2 tileTexSize;

	// 3D tile matrix stored contiguously: [NUM_LAYERS][mapSize.y][mapSize.x]
	// Rendered in layer order: 0 first, then 1, then 2.
	TileCell *map3D_;
	int defaultSkySprite_;
	bool zeroIndexedTiles_;

	// Dynamic vine groups
	struct VineGroup
	{
		glm::ivec2 headPos;
		vector<glm::ivec2> stemPositions;
		vector<TileCell> stemCells;
		bool expanded;
		int visibleCount;
		float stepTimer;
	};
	glm::vec2 minCoords_;
	ShaderProgram *shaderProgram_;
	vector<float> vertices_;
	vector<glm::ivec2> playerSpawns_;
	vector<EnemySpawnInfo> enemySpawns_;
	vector<VineGroup> vines_;

};


#endif // _TILE_MAP_INCLUDE
