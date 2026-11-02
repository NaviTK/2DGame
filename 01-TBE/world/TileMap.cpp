#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include "TileMap.h"


using namespace std;


static string toUpperStr(string s)
{
	for(size_t i = 0; i < s.size(); i++)
		s[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(s[i])));
	return s;
}

TileMap *TileMap::createTileMap(const string &levelFile, const glm::vec2 &minCoords, ShaderProgram &program)
{
	TileMap *map = new TileMap(levelFile, minCoords, program);
	return map;
}


TileMap::TileMap(const string &levelFile, const glm::vec2 &minCoords, ShaderProgram &program)
{
	vao = 0;
	vbo = 0;
	map3D_ = NULL;
	defaultSkySprite_ = 17;
	zeroIndexedTiles_ = false;
	isNewTileset_ = false;
	minCoords_ = minCoords;
	shaderProgram_ = &program;
	loadLevel(levelFile);
	prepareArrays(minCoords_, program);
}

TileMap::~TileMap()
{
	if(map3D_ != NULL)
		delete [] map3D_;
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

int TileMap::cellIndex(int layer, int x, int y) const
{
	return (layer * mapSize.y + y) * mapSize.x + x;
}

TileType TileMap::classifySpriteId(int t) const
{
	if(t < 0)
		return TILE_EMPTY;

	if(!zeroIndexedTiles_)
	{
		if(t == 2)
			return TILE_DESTRUCTIBLE;
		return TILE_SOLID;
	}

	// Semantic classification for images/tileset_nuevo.png (384x256, 12x8 grid of 32x32 tiles)
	if(isNewTileset_)
	{
		// 1. Exit doors (Forest wooden arch, Ruins golden arch, Cave portal)
		if(t == 11 || t == 35 || t == 59)
			return TILE_EXIT_DOOR;

		// 2. Vine / ladder / pipe source blocks (one-way platform on top + toggleable stem below)
		if(t == 10 || t == 34 || t == 58)
			return TILE_VINE_HEAD;

		// 3. Climbable vine, wooden ladder, and pipe stem segments
		if(t == 17 || t == 18 || t == 41 || t == 42 || t == 65 || t == 66)
			return TILE_VINE_STEM;

		// 4. Destructible blocks (berry bush, crates, cracked stone, amphora, ice crystal)
		if(t == 8 || t == 9 || t == 32 || t == 33 || t == 56 || t == 57)
			return TILE_DESTRUCTIBLE;

		// 5. Indestructible thin jump-through platforms (wooden, sandstone, metal ledges)
		if((t >= 5 && t <= 7) || (t >= 29 && t <= 31) || (t >= 53 && t <= 55))
			return TILE_ONE_WAY;

		// 6. Solid terrain, walls, outer jeweled border frame, sand, bomb block, and stone bridge
		if((t >= 0 && t <= 4) || (t >= 12 && t <= 16) ||
		   (t >= 24 && t <= 28) || (t >= 36 && t <= 39) ||
		   (t >= 48 && t <= 52) || (t >= 60 && t <= 64) ||
		   (t >= 72 && t <= 79) ||
		   (t >= 84 && t <= 86) || (t >= 92 && t <= 95))
			return TILE_SOLID;

		// 7. Sky, clouds, canopy, ruin bricks/torches, columns, water, and decorative props
		return TILE_SKY;
	}

	// 1. Exit door tile
	if(t == 121)
		return TILE_EXIT_DOOR;

	// 2. Vine head block (source of the vine, also acts as a one-way platform on top)
	if(t == 147)
		return TILE_VINE_HEAD;

	// 3. Climbable vine stem segments
	if(t == 163 || t == 179 || t == 195 || t == 211)
		return TILE_VINE_STEM;

	// 4. Destructible green foliage/bush blocks (also act as one-way platforms from above)
	switch(t)
	{
	case 19: case 25: case 29:
	case 88: case 126:
	case 209: case 217:
		return TILE_DESTRUCTIBLE;
	default:
		break;
	}

	// 5. Solid walls, outer border frame, bottom floor row, and interior grass/dirt blocks
	if(t <= 15 || t >= 240 || (t % 16) == 0 || (t % 16) == 15)
		return TILE_SOLID;
	if(t >= 225 && t <= 238)
		return TILE_SOLID;

	switch(t)
	{
	case 82: case 83: case 84:
	case 86: case 87:
	case 89: case 90: case 93:
	case 104: case 105: case 106: case 109:
	case 122: case 125:
	case 129:
	case 133: case 134: case 135: case 136: case 137: case 138:
	case 140: case 141: case 142:
	case 161: case 162:
	case 180: case 187: case 188:
		return TILE_SOLID;
	default:
		break;
	}

	// 6. All remaining tiles in tilesetworld.png are sky/background variants
	return TILE_SKY;
}

bool TileMap::loadLevel(const string &levelFile)
{
	ifstream fin(levelFile.c_str());
	if(!fin.is_open())
		return false;

	// Read all non-empty cleaned lines
	vector<string> lines;
	string rawLine;
	while(getline(fin, rawLine))
	{
		size_t comment = rawLine.find("--");
		if(comment != string::npos)
			rawLine = rawLine.substr(0, comment);
		size_t hashComment = rawLine.find('#');
		if(hashComment != string::npos)
			rawLine = rawLine.substr(0, hashComment);
		size_t first = rawLine.find_first_not_of(" \t\r\n");
		if(first != string::npos)
		{
			size_t last = rawLine.find_last_not_of(" \t\r\n");
			lines.push_back(rawLine.substr(first, (last - first + 1)));
		}
	}
	fin.close();

	if(lines.empty())
		return false;

	playerSpawns_.clear();
	enemySpawns_.clear();
	vines_.clear();

	mapSize = glm::ivec2(16, 16);
	tileSize = 16;
	blockSize = 16;
	tilesheetSize = glm::ivec2(16, 16);
	string tilesheetFile = "images/tilesetworld.png";

	vector<string> layerDataLines[NUM_LAYERS];
	int activeLayerSection = -1;
	int legacyHeaderStep = 0;

	// Parse self-documenting tags (MAP_SIZE, TILE_SIZE, TILESHEET, TILESHEET_GRID,
	// TILES / LAYER_0 / LAYER_1 / LAYER_2, PLAYER_SPAWN, ENEMY_SPAWN) as well as legacy positional lines.
	for(size_t idx = 0; idx < lines.size(); idx++)
	{
		stringstream ss(lines[idx]);
		string firstToken;
		ss >> firstToken;
		string upperToken = toUpperStr(firstToken);

		if(upperToken == "TILEMAP")
		{
			activeLayerSection = -1;
			continue;
		}
		else if(upperToken == "MAP_SIZE")
		{
			ss >> mapSize.x >> mapSize.y;
			activeLayerSection = -1;
			continue;
		}
		else if(upperToken == "TILE_SIZE")
		{
			ss >> tileSize >> blockSize;
			if(tileSize < blockSize)
				std::swap(tileSize, blockSize);
			activeLayerSection = -1;
			continue;
		}
		else if(upperToken == "TILESHEET")
		{
			ss >> tilesheetFile;
			activeLayerSection = -1;
			continue;
		}
		else if(upperToken == "TILESHEET_GRID" || upperToken == "TILESHEET_SIZE")
		{
			ss >> tilesheetSize.x >> tilesheetSize.y;
			activeLayerSection = -1;
			continue;
		}
		else if(upperToken == "TILES" || upperToken == "TILES:" ||
		        upperToken == "LAYER_0" || upperToken == "LAYER_0:")
		{
			activeLayerSection = 0;
			continue;
		}
		else if(upperToken == "LAYER_1" || upperToken == "LAYER_1:")
		{
			activeLayerSection = 1;
			continue;
		}
		else if(upperToken == "LAYER_2" || upperToken == "LAYER_2:")
		{
			activeLayerSection = 2;
			continue;
		}
		else if(upperToken == "PLAYER_SPAWN")
		{
			int px = 1, py = 1;
			if(ss >> px >> py)
				playerSpawns_.push_back(glm::ivec2(px, py));
			activeLayerSection = -1;
			continue;
		}
		else if(upperToken == "ENEMY_SPAWN")
		{
			string eType;
			int ex = 1, ey = 1;
			if(ss >> eType >> ex >> ey)
			{
				EnemySpawnInfo info;
				info.type = toUpperStr(eType);
				info.tilePos = glm::ivec2(ex, ey);
				enemySpawns_.push_back(info);
			}
			activeLayerSection = -1;
			continue;
		}

		// If currently inside a layer section and we still need rows, collect this row
		if(activeLayerSection >= 0 && activeLayerSection < NUM_LAYERS)
		{
			if(int(layerDataLines[activeLayerSection].size()) < mapSize.y)
				layerDataLines[activeLayerSection].push_back(lines[idx]);
			continue;
		}

		// Fallback for legacy untagged header lines
		if(legacyHeaderStep == 0)
		{
			stringstream lss(lines[idx]);
			lss >> mapSize.x >> mapSize.y;
			legacyHeaderStep++;
		}
		else if(legacyHeaderStep == 1)
		{
			stringstream lss(lines[idx]);
			lss >> tileSize >> blockSize;
			if(tileSize < blockSize)
				std::swap(tileSize, blockSize);
			legacyHeaderStep++;
		}
		else if(legacyHeaderStep == 2)
		{
			stringstream lss(lines[idx]);
			lss >> tilesheetFile;
			legacyHeaderStep++;
		}
		else if(legacyHeaderStep == 3)
		{
			stringstream lss(lines[idx]);
			lss >> tilesheetSize.x >> tilesheetSize.y;
			legacyHeaderStep++;
			activeLayerSection = 0;
		}
	}

	string upperSheet = toUpperStr(tilesheetFile);
	isNewTileset_ = (upperSheet.find("TILESET_NUEVO") != string::npos);
	defaultSkySprite_ = isNewTileset_ ? 19 : 17;

	tilesheet.loadFromFile(tilesheetFile, TEXTURE_PIXEL_FORMAT_RGBA);
	tilesheet.setWrapS(GL_CLAMP_TO_EDGE);
	tilesheet.setWrapT(GL_CLAMP_TO_EDGE);
	tilesheet.setMinFilter(GL_NEAREST);
	tilesheet.setMagFilter(GL_NEAREST);

	if(tilesheet.width() >= tileSize && tilesheet.height() >= tileSize)
	{
		int actualCols = tilesheet.width() / tileSize;
		int actualRows = (tilesheet.height() + tileSize - 1) / tileSize;
		if(actualCols >= tilesheetSize.x)
		{
			tilesheetSize.x = actualCols;
			tilesheetSize.y = actualRows;
			zeroIndexedTiles_ = true;
		}
	}

	tileTexSize = glm::vec2(float(tileSize) / float(tilesheet.width()),
	                        float(tileSize) / float(tilesheet.height()));

	if(minCoords_.x == 0.0f && minCoords_.y == 0.0f)
	{
		minCoords_.x = std::max(0.0f, (640.0f - mapSize.x * blockSize) * 0.5f);
		minCoords_.y = std::max(0.0f, (480.0f - mapSize.y * blockSize) * 0.5f);
	}

	// Allocate the 3D tile matrix [NUM_LAYERS][mapSize.y][mapSize.x]
	if(map3D_ != NULL)
		delete [] map3D_;
	int totalCells = NUM_LAYERS * mapSize.x * mapSize.y;
	map3D_ = new TileCell[totalCells];
	for(int idx = 0; idx < totalCells; idx++)
	{
		map3D_[idx].spriteId = -1;
		map3D_[idx].type = TILE_EMPTY;
	}

	vertices_.reserve(std::size_t(NUM_LAYERS * mapSize.x * mapSize.y * 24));

	bool hasUpperLayers = !layerDataLines[1].empty() || !layerDataLines[2].empty();

	// Populate Layers 0..2 from the parsed tile matrices
	for(int layer = 0; layer < NUM_LAYERS; layer++)
	{
		if(layer > 0 && layerDataLines[layer].empty())
			continue;

		for(int j = 0; j < mapSize.y; j++)
		{
			string dataLine = (j < int(layerDataLines[layer].size())) ? layerDataLines[layer][j] : "";
			stringstream lineStream(dataLine);
			string token;
			for(int i = 0; i < mapSize.x; i++)
			{
				int t = -1;
				if(lineStream >> token)
				{
					if(token == ".")
						t = -1;
					else
						t = atoi(token.c_str());
				}
				if(layer == 0 && !zeroIndexedTiles_)
				{
					if(t == 3)
					{
						playerSpawns_.push_back(glm::ivec2(i, j));
						t = -1;
					}
					else if(t == 4)
					{
						EnemySpawnInfo info;
						info.type = "PINK";
						info.tilePos = glm::ivec2(i, j);
						enemySpawns_.push_back(info);
						t = -1;
					}
				}

				TileType cellType = classifySpriteId(t);
				if(layer == 0 && cellType == TILE_SKY && t >= 0)
				{
					if(!isNewTileset_ || t == 19 || t == 43 || t == 67)
						defaultSkySprite_ = t;
				}

				setCell(layer, i, j, t, cellType);
			}
		}
	}

	// Single-layer convenience for tileset_nuevo.png: automatically back cutout tiles with sky on Layer 0
	if(isNewTileset_ && !hasUpperLayers)
	{
		for(int j = 0; j < mapSize.y; j++)
		{
			for(int i = 0; i < mapSize.x; i++)
			{
				TileCell c0 = getCell(0, i, j);
				bool isOpaqueFrame = (c0.spriteId >= 72 && c0.spriteId <= 79);
				if(c0.spriteId >= 0 && c0.type != TILE_SKY && !isOpaqueFrame)
				{
					setCell(0, i, j, defaultSkySprite_, TILE_SKY);
					setCell(1, i, j, c0.spriteId, c0.type);
				}
			}
		}
	}

	// Scan initial level layout for vine/ladder source blocks (TILE_VINE_HEAD) and record
	// the contiguous chain of stem cells (TILE_VINE_STEM), their layer, and their orientation.
	const glm::ivec2 dirs[4] = {
		glm::ivec2(0, 1),   // DOWN
		glm::ivec2(0, -1),  // UP
		glm::ivec2(1, 0),   // RIGHT
		glm::ivec2(-1, 0)   // LEFT
	};

	for(int y = 0; y < mapSize.y; y++)
	{
		for(int x = 0; x < mapSize.x; x++)
		{
			if(hasTileType(x, y, TILE_VINE_HEAD))
			{
				VineGroup vg;
				vg.headPos = glm::ivec2(x, y);
				vg.stemLayer = 0;
				vg.expanded = true;

				for(int d = 0; d < 4 && vg.stemPositions.empty(); d++)
				{
					glm::ivec2 cur = vg.headPos + dirs[d];
					while(cur.x >= 0 && cur.x < mapSize.x && cur.y >= 0 && cur.y < mapSize.y &&
					      hasTileType(cur.x, cur.y, TILE_VINE_STEM))
					{
						int foundLayer = 0;
						for(int sl = NUM_LAYERS - 1; sl >= 0; sl--)
						{
							if(getCell(sl, cur.x, cur.y).type == TILE_VINE_STEM)
							{
								foundLayer = sl;
								break;
							}
						}
						if(vg.stemPositions.empty())
							vg.stemLayer = foundLayer;
						vg.stemPositions.push_back(cur);
						vg.stemCells.push_back(getCell(foundLayer, cur.x, cur.y));
						cur += dirs[d];
					}
				}

				if(!vg.stemPositions.empty())
				{
					vg.visibleCount = int(vg.stemPositions.size());
					vg.stepTimer = 0.0f;
					vines_.push_back(vg);
				}
			}
		}
	}

	// Fallback spawns only if none were defined via PLAYER_SPAWN / ENEMY_SPAWN tags
	auto hasFloorBelow = [this](int x, int y) -> bool {
		return !isWalkable(x, y + 1) || isOneWayPlatform(x, y + 1);
	};

	if(playerSpawns_.empty())
	{
		for(int y = mapSize.y - 2; y >= 1 && playerSpawns_.empty(); y--)
		{
			for(int x = 1; x < mapSize.x - 1; x++)
			{
				if(isWalkable(x, y) && !isOneWayPlatform(x, y) && hasFloorBelow(x, y))
				{
					playerSpawns_.push_back(glm::ivec2(x, y));
					break;
				}
			}
		}
		if(playerSpawns_.empty() && isWalkable(1, 1))
			playerSpawns_.push_back(glm::ivec2(1, 1));
	}

	if(enemySpawns_.empty())
	{
		for(int y = 1; y < mapSize.y - 1 && enemySpawns_.size() < 3; y++)
		{
			for(int x = mapSize.x - 2; x >= 2 && enemySpawns_.size() < 3; x--)
			{
				if(isWalkable(x, y) && !isOneWayPlatform(x, y) && hasFloorBelow(x, y))
				{
					if(playerSpawns_.empty() || glm::ivec2(x, y) != playerSpawns_[0])
					{
						EnemySpawnInfo info;
						info.type = (enemySpawns_.size() % 2 == 0) ? "PINK" : "KIRBY";
						info.tilePos = glm::ivec2(x, y);
						enemySpawns_.push_back(info);
						x -= 3;
					}
				}
			}
		}
	}

	return true;
}

void TileMap::prepareArrays(const glm::vec2 &minCoords, ShaderProgram &program)
{
	glm::vec2 posTile, texCoordTile[2], halfTexel;
	vertices_.clear();

	nTiles = 0;
	float insetPx = isNewTileset_ ? 0.002f : 0.5f;
	halfTexel = glm::vec2(insetPx / tilesheet.width(), insetPx / tilesheet.height());

	// Render the 3D tile matrix in strict layer order: Layer 0 first, then Layer 1, then Layer 2
	for(int layer = 0; layer < NUM_LAYERS; layer++)
	{
		for(int j = 0; j < mapSize.y; j++)
		{
			for(int i = 0; i < mapSize.x; i++)
			{
				const TileCell &cell = getCell(layer, i, j);
				if(cell.spriteId >= 0 && cell.type != TILE_EMPTY)
				{
					nTiles++;
					posTile = glm::vec2(minCoords.x + i * blockSize, minCoords.y + j * blockSize);
					int idx = zeroIndexedTiles_ ? cell.spriteId : (cell.spriteId - 1);
					int col = idx % tilesheetSize.x;
					int row = idx / tilesheetSize.x;
					texCoordTile[0] = glm::vec2(float(col * tileSize) / float(tilesheet.width()),
					                            float(row * tileSize) / float(tilesheet.height()));
					texCoordTile[1] = texCoordTile[0] + tileTexSize;
					if(texCoordTile[1].y > 1.0f)
						texCoordTile[1].y = 1.0f;
					texCoordTile[0] += halfTexel;
					texCoordTile[1] -= halfTexel;

					// First triangle
					vertices_.push_back(posTile.x); vertices_.push_back(posTile.y);
					vertices_.push_back(texCoordTile[0].x); vertices_.push_back(texCoordTile[0].y);
					vertices_.push_back(posTile.x + blockSize); vertices_.push_back(posTile.y);
					vertices_.push_back(texCoordTile[1].x); vertices_.push_back(texCoordTile[0].y);
					vertices_.push_back(posTile.x + blockSize); vertices_.push_back(posTile.y + blockSize);
					vertices_.push_back(texCoordTile[1].x); vertices_.push_back(texCoordTile[1].y);

					// Second triangle
					vertices_.push_back(posTile.x); vertices_.push_back(posTile.y);
					vertices_.push_back(texCoordTile[0].x); vertices_.push_back(texCoordTile[0].y);
					vertices_.push_back(posTile.x + blockSize); vertices_.push_back(posTile.y + blockSize);
					vertices_.push_back(texCoordTile[1].x); vertices_.push_back(texCoordTile[1].y);
					vertices_.push_back(posTile.x); vertices_.push_back(posTile.y + blockSize);
					vertices_.push_back(texCoordTile[0].x); vertices_.push_back(texCoordTile[1].y);
				}
			}
		}
	}

	if(vao == 0)
	{
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		posLocation = program.bindVertexAttribute("position", 2, 4 * sizeof(float), 0);
		texCoordLocation = program.bindVertexAttribute("texCoord", 2, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	}
	else
	{
		glBindVertexArray(vao);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
	}

	if(!vertices_.empty())
		glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(float), &vertices_[0], GL_DYNAMIC_DRAW);
}

const TileCell &TileMap::getCell(int layer, int x, int y) const
{
	static const TileCell outOfBoundsCell = { 0, TILE_SOLID };
	static const TileCell emptyLayerCell  = { -1, TILE_EMPTY };

	if(x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y)
		return (layer == 0) ? outOfBoundsCell : emptyLayerCell;
	if(layer < 0 || layer >= NUM_LAYERS || map3D_ == NULL)
		return emptyLayerCell;

	return map3D_[cellIndex(layer, x, y)];
}

void TileMap::setCell(int layer, int x, int y, int spriteId, TileType type)
{
	if(layer < 0 || layer >= NUM_LAYERS || x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y || map3D_ == NULL)
		return;
	TileCell &cell = map3D_[cellIndex(layer, x, y)];
	cell.spriteId = spriteId;
	cell.type = type;
}

void TileMap::setCellFromSprite(int layer, int x, int y, int spriteId)
{
	setCell(layer, x, y, spriteId, classifySpriteId(spriteId));
}

TileType TileMap::getTileType(int x, int y, int layer) const
{
	return getCell(layer, x, y).type;
}

bool TileMap::hasTileType(int x, int y, TileType type) const
{
	if(x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y)
		return (type == TILE_SOLID);

	for(int layer = NUM_LAYERS - 1; layer >= 0; layer--)
	{
		if(getCell(layer, x, y).type == type)
			return true;
	}
	return false;
}

int TileMap::getTile(int x, int y, int layer) const
{
	return getCell(layer, x, y).spriteId;
}

bool TileMap::isWalkable(int x, int y) const
{
	if(x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y)
		return false;
	return !hasTileType(x, y, TILE_SOLID);
}

bool TileMap::isOneWayPlatform(int x, int y) const
{
	if(x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y)
		return false;
	if(hasTileType(x, y, TILE_SOLID))
		return false;
	return hasTileType(x, y, TILE_ONE_WAY) ||
	       hasTileType(x, y, TILE_DESTRUCTIBLE) ||
	       hasTileType(x, y, TILE_VINE_HEAD);
}

bool TileMap::isClimbable(int x, int y) const
{
	if(x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y)
		return false;
	return hasTileType(x, y, TILE_VINE_HEAD) || hasTileType(x, y, TILE_VINE_STEM);
}

bool TileMap::isClimbableAt(const glm::vec2 &pos, const glm::vec2 &size) const
{
	const float eps = 1.0f;
	int x0 = int(std::floor((pos.x + eps - minCoords_.x) / blockSize));
	int x1 = int(std::floor((pos.x + size.x - eps - minCoords_.x) / blockSize));
	int y0 = int(std::floor((pos.y + eps - minCoords_.y) / blockSize));
	int y1 = int(std::floor((pos.y + size.y - eps - minCoords_.y) / blockSize));
	for(int y = y0; y <= y1; y++)
	{
		for(int x = x0; x <= x1; x++)
		{
			if(isClimbable(x, y))
				return true;
		}
	}
	return false;
}

bool TileMap::isDestructible(int x, int y) const
{
	return hasTileType(x, y, TILE_DESTRUCTIBLE);
}

bool TileMap::isExitDoor(int x, int y) const
{
	return hasTileType(x, y, TILE_EXIT_DOOR);
}

void TileMap::destroyTile(int x, int y)
{
	if(x < 0 || x >= mapSize.x || y < 0 || y >= mapSize.y)
		return;

	bool modified = false;
	for(int layer = NUM_LAYERS - 1; layer >= 0; layer--)
	{
		if(getCell(layer, x, y).type == TILE_DESTRUCTIBLE)
		{
			if(layer == 0)
			{
				int skyId = zeroIndexedTiles_ ? defaultSkySprite_ : -1;
				TileType newType = zeroIndexedTiles_ ? TILE_SKY : TILE_EMPTY;
				setCell(0, x, y, skyId, newType);
			}
			else
			{
				setCell(layer, x, y, -1, TILE_EMPTY);
			}
			modified = true;
		}
	}

	if(modified && shaderProgram_ != NULL)
		prepareArrays(minCoords_, *shaderProgram_);
}

bool TileMap::toggleVineAtHead(int x, int y)
{
	for(size_t i = 0; i < vines_.size(); i++)
	{
		VineGroup &vg = vines_[i];
		bool hitHead = (vg.headPos.x == x && vg.headPos.y == y);
		bool hitFirstStem = (!vg.stemPositions.empty() && vg.stemPositions[0].x == x && vg.stemPositions[0].y == y);
		if(hitHead || hitFirstStem)
		{
			vg.expanded = !vg.expanded;
			vg.stepTimer = 0.0f;
			return true;
		}
	}
	return false;
}

void TileMap::update(int deltaTime)
{
	if(vines_.empty())
		return;

	const float STEP_MS = 100.0f; // 0.1 seconds per vine tile
	bool changed = false;

	for(size_t i = 0; i < vines_.size(); i++)
	{
		VineGroup &vg = vines_[i];
		int targetCount = vg.expanded ? int(vg.stemPositions.size()) : 0;
		if(vg.visibleCount == targetCount)
			continue;

		vg.stepTimer += float(deltaTime);
		while(vg.stepTimer >= STEP_MS && vg.visibleCount != targetCount)
		{
			vg.stepTimer -= STEP_MS;
			if(vg.expanded)
			{
				glm::ivec2 pos = vg.stemPositions[vg.visibleCount];
				const TileCell &stemCell = vg.stemCells[vg.visibleCount];
				setCell(vg.stemLayer, pos.x, pos.y, stemCell.spriteId, stemCell.type);
				vg.visibleCount++;
				changed = true;
			}
			else
			{
				vg.visibleCount--;
				glm::ivec2 pos = vg.stemPositions[vg.visibleCount];
				if(vg.stemLayer == 0)
					setCell(0, pos.x, pos.y, defaultSkySprite_, TILE_SKY);
				else
					setCell(vg.stemLayer, pos.x, pos.y, -1, TILE_EMPTY);
				changed = true;
			}
		}
	}

	if(changed && shaderProgram_ != NULL)
		prepareArrays(minCoords_, *shaderProgram_);
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

const vector<EnemySpawnInfo> &TileMap::getEnemySpawns() const
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

bool TileMap::collisionMoveDown(const glm::vec2 &pos, const glm::vec2 &size, float *posY, float prevBottomY) const
{
	const float eps = 0.01f;
	int x0 = int(std::floor((pos.x + eps - minCoords_.x) / blockSize));
	int x1 = int(std::floor((pos.x + size.x - eps - minCoords_.x) / blockSize));

	float newBottomY = pos.y + size.y;
	int yEnd = int(std::floor((newBottomY - 0.001f - minCoords_.y) / blockSize));
	int yStart = yEnd;
	if(prevBottomY > -1e8f)
	{
		int yPrev = int(std::floor((prevBottomY - 0.5f - minCoords_.y) / blockSize));
		if(yPrev < yStart)
			yStart = yPrev;
	}

	for(int y = yStart; y <= yEnd; y++)
	{
		float tileTopY = minCoords_.y + float(y * blockSize);
		for(int x = x0; x <= x1; x++)
		{
			if(!isWalkable(x, y))
			{
				if(posY != NULL)
					*posY = tileTopY - size.y;
				return true;
			}
			if(isOneWayPlatform(x, y))
			{
				if(prevBottomY <= tileTopY + 4.0f && newBottomY >= tileTopY)
				{
					if(posY != NULL)
						*posY = tileTopY - size.y;
					return true;
				}
			}
		}
	}
	return false;
}
