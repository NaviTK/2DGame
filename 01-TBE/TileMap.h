#ifndef _TILE_MAP_INCLUDE
#define _TILE_MAP_INCLUDE


#include <vector>
#include <glm/glm.hpp>
#include "Texture.h"
#include "ShaderProgram.h"


using namespace std;


// Class Tilemap is capable of loading a tile map from a text file in a very
// simple format (see level01.txt for an example). With this information
// it builds a single VBO that contains all tiles. As a result the render
// method draws the whole map independently of what is visible.
//
// Extended for Bomberman: supports tile queries, collision detection,
// destructible blocks, and spawn point parsing.


class TileMap
{

private:
	TileMap(const string &levelFile, const glm::vec2 &minCoords, ShaderProgram &program);

public:
	// Tile maps can only be created inside an OpenGL context
	static TileMap *createTileMap(const string &levelFile, const glm::vec2 &minCoords, ShaderProgram &program);

	~TileMap();

	void render() const;
	void free();

	// Tile queries
	int getTile(int x, int y) const;
	bool isWalkable(int x, int y) const;
	bool isDestructible(int x, int y) const;
	void destroyTile(int x, int y);

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

	// Spawn points (parsed from level: '3' = player, '4' = enemy)
	const vector<glm::ivec2> &getPlayerSpawns() const;
	const vector<glm::ivec2> &getEnemySpawns() const;

	// Collision detection for entity movement
	bool collisionMoveLeft(const glm::vec2 &pos, const glm::vec2 &size) const;
	bool collisionMoveRight(const glm::vec2 &pos, const glm::vec2 &size) const;
	bool collisionMoveUp(const glm::vec2 &pos, const glm::vec2 &size) const;
	bool collisionMoveDown(const glm::vec2 &pos, const glm::vec2 &size) const;

	bool collisionMoveLeft(const glm::vec2 &pos, const glm::ivec2 &size) const { return collisionMoveLeft(pos, glm::vec2(size)); }
	bool collisionMoveRight(const glm::vec2 &pos, const glm::ivec2 &size) const { return collisionMoveRight(pos, glm::vec2(size)); }
	bool collisionMoveUp(const glm::vec2 &pos, const glm::ivec2 &size) const { return collisionMoveUp(pos, glm::vec2(size)); }
	bool collisionMoveDown(const glm::vec2 &pos, const glm::ivec2 &size) const { return collisionMoveDown(pos, glm::vec2(size)); }

private:
	bool loadLevel(const string &levelFile);
	void prepareArrays(const glm::vec2 &minCoords, ShaderProgram &program);

private:
	GLuint vao;
	GLuint vbo;
	GLint posLocation, texCoordLocation;
	int nTiles;
	glm::ivec2 position, mapSize, tilesheetSize;
	int tileSize, blockSize;
	Texture tilesheet;
	glm::vec2 tileTexSize;
	int *map;

	// Bomberman extensions
	glm::vec2 minCoords_;
	ShaderProgram *shaderProgram_;
	vector<glm::ivec2> playerSpawns_;
	vector<glm::ivec2> enemySpawns_;

};


#endif // _TILE_MAP_INCLUDE

