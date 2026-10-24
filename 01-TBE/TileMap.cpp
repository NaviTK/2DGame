#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include "TileMap.h"


using namespace std;


TileMap *TileMap::createTileMap(const string &levelFile, const glm::vec2 &minCoords, ShaderProgram &program)
{
	TileMap *map = new TileMap(levelFile, minCoords, program);
	return map;
}


TileMap::TileMap(const string &levelFile, const glm::vec2 &minCoords, ShaderProgram &program)
{
	vao = 0;
	vbo = 0;
	map = NULL;
	minCoords_ = minCoords;
	shaderProgram_ = &program;
	loadLevel(levelFile);
	prepareArrays(minCoords_, program);
}

TileMap::~TileMap()
{
	if(map != NULL)
		delete [] map;
	if(vao != 0)
		glDeleteVertexArrays(1, &vao);
	if(vbo != 0)
		glDeleteBuffers(1, &vbo);
}

void TileMap::render() const
{
	glEnable(GL_TEXTURE_2D);
	tilesheet.use();
	glBindVertexArray(vao);
	glEnableVertexAttribArray(posLocation);
	glEnableVertexAttribArray(texCoordLocation);
	glDrawArrays(GL_TRIANGLES, 0, 6 * nTiles);
	glDisable(GL_TEXTURE_2D);
}

void TileMap::free()
{
	if(vbo != 0)
	{
		glDeleteBuffers(1, &vbo);
		vbo = 0;
	}
	if(vao != 0)
	{
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}
}

bool TileMap::loadLevel(const string &levelFile)
{
	ifstream fin;
	string line, tilesheetFile;
	stringstream sstream;

	fin.open(levelFile.c_str());
	if(!fin.is_open())
		return false;

	// Helper to read next non-empty line ignoring comments starting with "--"
	auto getCleanLine = [&fin](string &outLine) -> bool {
		while(getline(fin, outLine))
		{
			size_t comment = outLine.find("--");
			if(comment != string::npos)
				outLine = outLine.substr(0, comment);
			size_t first = outLine.find_first_not_of(" \t\r\n");
			if(first != string::npos)
			{
				size_t last = outLine.find_last_not_of(" \t\r\n");
				outLine = outLine.substr(first, (last - first + 1));
				return true;
			}
		}
		return false;
	};

	if(!getCleanLine(line)) return false;
	if(line.compare(0, 7, "TILEMAP") != 0)
		return false;

	if(!getCleanLine(line)) return false;
	sstream.clear();
	sstream.str(line);
	sstream >> mapSize.x >> mapSize.y;

	if(!getCleanLine(line)) return false;
	sstream.clear();
	sstream.str(line);
	sstream >> tileSize >> blockSize;
	if(tileSize < blockSize)
		std::swap(tileSize, blockSize);

	if(!getCleanLine(line)) return false;
	sstream.clear();
	sstream.str(line);
	sstream >> tilesheetFile;
	tilesheet.loadFromFile(tilesheetFile, TEXTURE_PIXEL_FORMAT_RGBA);
	tilesheet.setWrapS(GL_CLAMP_TO_EDGE);
	tilesheet.setWrapT(GL_CLAMP_TO_EDGE);
	tilesheet.setMinFilter(GL_NEAREST);
	tilesheet.setMagFilter(GL_NEAREST);

	if(!getCleanLine(line)) return false;
	sstream.clear();
	sstream.str(line);
	sstream >> tilesheetSize.x >> tilesheetSize.y;
	tileTexSize = glm::vec2(1.f / tilesheetSize.x, 1.f / tilesheetSize.y);

	// Automatically center map in standard 640x480 viewport if minCoords was default (0,0)
	if(minCoords_.x == 0.0f && minCoords_.y == 0.0f)
	{
		minCoords_.x = std::max(0.0f, (640.0f - mapSize.x * blockSize) * 0.5f);
		minCoords_.y = std::max(0.0f, (480.0f - mapSize.y * blockSize) * 0.5f);
	}

	playerSpawns_.clear();
	enemySpawns_.clear();

	if(map != NULL)
		delete [] map;
	map = new int[mapSize.x * mapSize.y];

	// Read each row of map data
	for(int j = 0; j < mapSize.y; j++)
	{
		string dataLine;
		while(getline(fin, dataLine))
		{
			size_t comment = dataLine.find("--");
			if(comment != string::npos)
				dataLine = dataLine.substr(0, comment);
			if(dataLine.find_first_not_of(" \t\r\n") != string::npos)
				break;
		}

		// Cells are separated by spaces; '.' is an empty cell
		stringstream lineStream(dataLine);
		string token;
		for(int i = 0; i < mapSize.x; i++)
		{
			int t = 0;
			if(lineStream >> token)
			{
				if(token == ".")
					t = 0;
				else
					t = atoi(token.c_str());
			}
			if(t == 3)
			{
				playerSpawns_.push_back(glm::ivec2(i, j));
				t = 0;
			}
			else if(t == 4)
			{
				enemySpawns_.push_back(glm::ivec2(i, j));
				t = 0;
			}
			map[j * mapSize.x + i] = t;
		}
	}
	fin.close();

	// Fallback spawns if none were explicitly tagged in the level
	if(playerSpawns_.empty())
	{
		if(isWalkable(1, 1))
			playerSpawns_.push_back(glm::ivec2(1, 1));
		else
		{
			for(int y = 0; y < mapSize.y && playerSpawns_.empty(); y++)
				for(int x = 0; x < mapSize.x; x++)
					if(isWalkable(x, y)) { playerSpawns_.push_back(glm::ivec2(x, y)); break; }
		}
	}

	if(enemySpawns_.empty())
	{
		if(isWalkable(mapSize.x - 2, 1))
			enemySpawns_.push_back(glm::ivec2(mapSize.x - 2, 1));
		if(isWalkable(1, mapSize.y - 2))
			enemySpawns_.push_back(glm::ivec2(1, mapSize.y - 2));
		if(isWalkable(mapSize.x - 2, mapSize.y - 2))
			enemySpawns_.push_back(glm::ivec2(mapSize.x - 2, mapSize.y - 2));
	}

	return true;
}

void TileMap::prepareArrays(const glm::vec2 &minCoords, ShaderProgram &program)
{
	int tile;
	glm::vec2 posTile, texCoordTile[2], halfTexel;
	vector<float> vertices;

	nTiles = 0;
	halfTexel = glm::vec2(0.5f / tilesheet.width(), 0.5f / tilesheet.height());
	for(int j = 0; j < mapSize.y; j++)
	{
		for(int i = 0; i < mapSize.x; i++)
		{
			tile = map[j * mapSize.x + i];
			if(tile != 0)
			{
				nTiles++;
				posTile = glm::vec2(minCoords.x + i * blockSize, minCoords.y + j * blockSize);
				texCoordTile[0] = glm::vec2(float((tile - 1) % tilesheetSize.x) / tilesheetSize.x,
				                            float((tile - 1) / tilesheetSize.x) / tilesheetSize.y);
				texCoordTile[1] = texCoordTile[0] + tileTexSize;
				texCoordTile[1] -= halfTexel;

				// First triangle
				vertices.push_back(posTile.x); vertices.push_back(posTile.y);
				vertices.push_back(texCoordTile[0].x); vertices.push_back(texCoordTile[0].y);
				vertices.push_back(posTile.x + tileSize); vertices.push_back(posTile.y);
				vertices.push_back(texCoordTile[1].x); vertices.push_back(texCoordTile[0].y);
				vertices.push_back(posTile.x + tileSize); vertices.push_back(posTile.y + tileSize);
				vertices.push_back(texCoordTile[1].x); vertices.push_back(texCoordTile[1].y);

				// Second triangle
				vertices.push_back(posTile.x); vertices.push_back(posTile.y);
				vertices.push_back(texCoordTile[0].x); vertices.push_back(texCoordTile[0].y);
				vertices.push_back(posTile.x + tileSize); vertices.push_back(posTile.y + tileSize);
				vertices.push_back(texCoordTile[1].x); vertices.push_back(texCoordTile[1].y);
				vertices.push_back(posTile.x); vertices.push_back(posTile.y + tileSize);
				vertices.push_back(texCoordTile[0].x); vertices.push_back(texCoordTile[1].y);
			}
		}
	}

	if(vao != 0)
		glDeleteVertexArrays(1, &vao);
	if(vbo != 0)
		glDeleteBuffers(1, &vbo);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	if(!vertices.empty())
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

	posLocation = program.bindVertexAttribute("position", 2, 4 * sizeof(float), 0);
	texCoordLocation = program.bindVertexAttribute("texCoord", 2, 4 * sizeof(float), (void *)(2 * sizeof(float)));
}

int TileMap::getTile(int x, int y) const
{
	if(x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y)
		return 1; // Out of bounds is solid wall
	return map[y * mapSize.x + x];
}

bool TileMap::isWalkable(int x, int y) const
{
	int t = getTile(x, y);
	// Only 0 (empty space) is walkable
	return (t == 0);
}

bool TileMap::isDestructible(int x, int y) const
{
	int t = getTile(x, y);
	// Only 2 (brick in blocks.png) is destructible; 1 is indestructible wall
	return (t == 2);
}

void TileMap::destroyTile(int x, int y)
{
	if(x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y)
		return;
	if(map[y * mapSize.x + x] == 2)
	{
		map[y * mapSize.x + x] = 0;
		if(shaderProgram_ != NULL)
			prepareArrays(minCoords_, *shaderProgram_);
	}
}

int TileMap::getMapWidth() const { return mapSize.x; }
int TileMap::getMapHeight() const { return mapSize.y; }
int TileMap::getTileSize() const { return tileSize; }
int TileMap::getBlockSize() const { return blockSize; }
glm::vec2 TileMap::getMinCoords() const { return minCoords_; }

void TileMap::setMinCoords(const glm::vec2 &minCoords, ShaderProgram &program)
{
	minCoords_ = minCoords;
	shaderProgram_ = &program;
	prepareArrays(minCoords_, program);
}

glm::ivec2 TileMap::worldToTile(const glm::vec2 &worldPos) const
{
	return glm::ivec2(int(std::floor((worldPos.x - minCoords_.x) / blockSize)),
	                  int(std::floor((worldPos.y - minCoords_.y) / blockSize)));
}

glm::vec2 TileMap::tileToWorld(const glm::ivec2 &tilePos) const
{
	return glm::vec2(minCoords_.x + tilePos.x * blockSize, minCoords_.y + tilePos.y * blockSize);
}

const vector<glm::ivec2> &TileMap::getPlayerSpawns() const
{
	return playerSpawns_;
}

const vector<glm::ivec2> &TileMap::getEnemySpawns() const
{
	return enemySpawns_;
}

bool TileMap::collisionMoveLeft(const glm::vec2 &pos, const glm::vec2 &size) const
{
	int x = int(std::floor((pos.x - minCoords_.x) / blockSize));
	const float eps = 0.01f;
	int y0 = int(std::floor((pos.y + eps - minCoords_.y) / blockSize));
	int y1 = int(std::floor((pos.y + size.y - eps - minCoords_.y) / blockSize));
	for(int y = y0; y <= y1; y++)
	{
		if(!isWalkable(x, y))
			return true;
	}
	return false;
}

bool TileMap::collisionMoveRight(const glm::vec2 &pos, const glm::vec2 &size) const
{
	int x = int(std::floor((pos.x + size.x - 0.001f - minCoords_.x) / blockSize));
	const float eps = 0.01f;
	int y0 = int(std::floor((pos.y + eps - minCoords_.y) / blockSize));
	int y1 = int(std::floor((pos.y + size.y - eps - minCoords_.y) / blockSize));
	for(int y = y0; y <= y1; y++)
	{
		if(!isWalkable(x, y))
			return true;
	}
	return false;
}

bool TileMap::collisionMoveUp(const glm::vec2 &pos, const glm::vec2 &size) const
{
	int y = int(std::floor((pos.y - minCoords_.y) / blockSize));
	const float eps = 0.01f;
	int x0 = int(std::floor((pos.x + eps - minCoords_.x) / blockSize));
	int x1 = int(std::floor((pos.x + size.x - eps - minCoords_.x) / blockSize));
	for(int x = x0; x <= x1; x++)
	{
		if(!isWalkable(x, y))
			return true;
	}
	return false;
}

bool TileMap::collisionMoveDown(const glm::vec2 &pos, const glm::vec2 &size) const
{
	int y = int(std::floor((pos.y + size.y - 0.001f - minCoords_.y) / blockSize));
	const float eps = 0.01f;
	int x0 = int(std::floor((pos.x + eps - minCoords_.x) / blockSize));
	int x1 = int(std::floor((pos.x + size.x - eps - minCoords_.x) / blockSize));
	for(int x = x0; x <= x1; x++)
	{
		if(!isWalkable(x, y))
			return true;
	}
	return false;
}

