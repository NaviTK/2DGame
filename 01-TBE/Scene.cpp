#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "Scene.h"


using namespace std;


Scene::Scene()
{
	map = NULL;
	quadVAO = 0;
	quadVBO = 0;
	gameOver = false;
	win = false;
	currentTime = 0.0f;
}

Scene::~Scene()
{
	if(map != NULL)
		delete map;
	if(quadVAO != 0)
		glDeleteVertexArrays(1, &quadVAO);
	if(quadVBO != 0)
		glDeleteBuffers(1, &quadVBO);
}

void Scene::init()
{
	initShaders();
	initQuadVAO();

	currentTime = 0.0f;
	gameOver = false;
	win = false;
	bombs.clear();
	explosions.clear();
	powerups.clear();
	enemies.clear();

	// Passing glm::vec2(0.0f) lets TileMap automatically center the map inside 640x480
	map = TileMap::createTileMap("levels/level01.txt", glm::vec2(0.0f), texProgram);
	projection = glm::ortho(0.f, float(CAMERA_WIDTH), float(CAMERA_HEIGHT), 0.f);

	float tSize = float(map->getBlockSize());

	if(!map->getPlayerSpawns().empty())
		player.init(map->tileToWorld(map->getPlayerSpawns()[0]), tSize);
	else
		player.init(map->tileToWorld(glm::ivec2(1, 1)), tSize);

	for(const auto &spawn : map->getEnemySpawns())
	{
		Enemy enemy;
		enemy.init(map->tileToWorld(spawn), tSize);
		enemies.push_back(enemy);
	}
}

void Scene::update(int deltaTime)
{
	currentTime += float(deltaTime);

	if(gameOver || win)
		return;

	player.update(deltaTime, map);

	for(size_t i = 0; i < enemies.size(); i++)
	{
		enemies[i].update(deltaTime, map);
	}

	for(size_t i = 0; i < bombs.size(); i++)
	{
		bombs[i].update(deltaTime);
	}

	for(size_t i = 0; i < explosions.size(); i++)
	{
		explosions[i].update(deltaTime);
	}

	handleBombPlacement();
	handleExplosions();
	handlePowerUpCollection();
	checkEntityCollisions();
	cleanupEntities();

	if(!player.isAlive())
		gameOver = true;
	else if(enemies.empty())
		win = true;
}

void Scene::render()
{
	// 1. Render tile map
	texProgram.use();
	texProgram.setUniformMatrix4f("projection", projection);
	texProgram.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	glm::mat4 modelview = glm::mat4(1.0f);
	texProgram.setUniformMatrix4f("modelview", modelview);
	if(map != NULL)
		map->render();

	float tSize = float(map->getBlockSize());

	// 2. Render power-ups (centered in tile)
	for(const auto &pw : powerups)
	{
		if(!pw.isCollected())
		{
			glm::vec4 color;
			switch(pw.getType())
			{
			case EXTRA_BOMB:
				color = glm::vec4(0.2f, 0.5f, 1.0f, 1.0f); // Blue
				break;
			case BOMB_RANGE:
				color = glm::vec4(1.0f, 0.85f, 0.1f, 1.0f); // Yellow
				break;
			case SPEED_UP:
				color = glm::vec4(0.1f, 0.9f, 0.3f, 1.0f); // Green
				break;
			}
			renderQuad(pw.getPosition() + glm::vec2(tSize * 0.2f), glm::vec2(tSize * 0.6f), color);
		}
	}

	// 3. Render bombs (pulsing red/dark quad)
	for(const auto &b : bombs)
	{
		if(!b.hasExploded())
		{
			float pulse = 0.5f + 0.5f * sin(currentTime * 0.015f);
			glm::vec4 bombColor(0.2f + 0.7f * pulse, 0.1f, 0.1f, 1.0f);
			renderQuad(b.getPosition() + glm::vec2(tSize * 0.125f), glm::vec2(tSize * 0.75f), bombColor);
		}
	}

	// 4. Render explosions (flames on all affected tiles)
	for(const auto &exp : explosions)
	{
		if(!exp.isFinished())
		{
			const auto &tiles = exp.getAffectedTiles();
			for(const auto &tile : tiles)
			{
				glm::vec2 worldPos = map->tileToWorld(tile);
				renderQuad(worldPos + glm::vec2(tSize * 0.05f), glm::vec2(tSize * 0.9f), glm::vec4(1.0f, 0.5f, 0.0f, 0.85f));
			}
		}
	}

	// 5. Render enemies (red quads)
	for(const auto &enemy : enemies)
	{
		if(enemy.isAlive())
		{
			renderQuad(enemy.getPosition() + glm::vec2(tSize * 0.125f), glm::vec2(tSize * 0.75f), glm::vec4(0.85f, 0.15f, 0.15f, 1.0f));
		}
	}

	// 6. Render player (cyan quad)
	if(player.isAlive())
	{
		renderQuad(player.getHitboxPos(), player.getHitboxSize(), glm::vec4(0.1f, 0.8f, 0.95f, 1.0f));
	}
	else
	{
		renderQuad(player.getHitboxPos(), player.getHitboxSize(), glm::vec4(0.4f, 0.4f, 0.4f, 1.0f));
	}

	// 7. Debug Draw: Colisiones
	renderDebugDraw();
}

bool Scene::isGameOver() const
{
	return gameOver;
}

bool Scene::isWin() const
{
	return win;
}

void Scene::initShaders()
{
	Shader vShader, fShader;

	// Texture shader for tilemap
	vShader.initFromFile(VERTEX_SHADER, "shaders/texture.vert");
	if(!vShader.isCompiled())
	{
		cout << "Vertex Shader Error: " << vShader.log() << endl;
	}
	fShader.initFromFile(FRAGMENT_SHADER, "shaders/texture.frag");
	if(!fShader.isCompiled())
	{
		cout << "Fragment Shader Error: " << fShader.log() << endl;
	}
	texProgram.init();
	texProgram.addShader(vShader);
	texProgram.addShader(fShader);
	texProgram.link();
	if(!texProgram.isLinked())
	{
		cout << "Shader Linking Error: " << texProgram.log() << endl;
	}
	texProgram.bindFragmentOutput("outColor");
	vShader.free();
	fShader.free();

	// Simple shader for colored entity quads
	vShader.initFromFile(VERTEX_SHADER, "shaders/simple.vert");
	if(!vShader.isCompiled())
	{
		cout << "Simple Vertex Shader Error: " << vShader.log() << endl;
	}
	fShader.initFromFile(FRAGMENT_SHADER, "shaders/simple.frag");
	if(!fShader.isCompiled())
	{
		cout << "Simple Fragment Shader Error: " << fShader.log() << endl;
	}
	simpleProgram.init();
	simpleProgram.addShader(vShader);
	simpleProgram.addShader(fShader);
	simpleProgram.link();
	if(!simpleProgram.isLinked())
	{
		cout << "Simple Shader Linking Error: " << simpleProgram.log() << endl;
	}
	simpleProgram.bindFragmentOutput("outColor");
	vShader.free();
	fShader.free();
}

void Scene::initQuadVAO()
{
	float quadVertices[] = {
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f
	};

	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
	quadPosLocation = simpleProgram.bindVertexAttribute("position", 2, 2 * sizeof(float), 0);
}

void Scene::renderQuad(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color)
{
	simpleProgram.use();
	simpleProgram.setUniformMatrix4f("projection", projection);
	simpleProgram.setUniform4f("color", color.r, color.g, color.b, color.a);

	glm::mat4 modelview = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, 0.0f));
	modelview = glm::scale(modelview, glm::vec3(size.x, size.y, 1.0f));
	simpleProgram.setUniformMatrix4f("modelview", modelview);

	glBindVertexArray(quadVAO);
	glEnableVertexAttribArray(quadPosLocation);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Scene::renderWireQuad(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color)
{
	simpleProgram.use();
	simpleProgram.setUniformMatrix4f("projection", projection);
	simpleProgram.setUniform4f("color", color.r, color.g, color.b, color.a);

	glm::mat4 modelview = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, 0.0f));
	modelview = glm::scale(modelview, glm::vec3(size.x, size.y, 1.0f));
	simpleProgram.setUniformMatrix4f("modelview", modelview);

	// Usamos los 4 vertices del quad [ (0,0), (1,0), (1,1), (0,1) ] en GL_LINE_LOOP
	glBindVertexArray(quadVAO);
	glEnableVertexAttribArray(quadPosLocation);
	// Los vertices 0, 1, 2 son (0,0), (1,0), (1,1) y el vertice 5 es (0,1)
	// Para un bucle cerrado independiente del orden de triangulos, trazamos line loop:
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Scene::renderDebugDraw()
{
	if(map == NULL)
		return;

	float tSize = float(map->getBlockSize());
	glm::vec2 minCoords = map->getMinCoords();

	// 1. Dibujar hitboxes de los tiles solidos del mapa en VERDE
	for(int j = 0; j < map->getMapHeight(); j++)
	{
		for(int i = 0; i < map->getMapWidth(); i++)
		{
			if(!map->isWalkable(i, j))
			{
				glm::vec2 tileWorldPos = minCoords + glm::vec2(i * tSize, j * tSize);
				renderWireQuad(tileWorldPos, glm::vec2(tSize, tSize), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}
		}
	}

	// 2. Dibujar hitbox del Player en ROJO
	renderWireQuad(player.getHitboxPos(), player.getHitboxSize(), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void Scene::handleBombPlacement()
{
	if(player.wantsToPlaceBomb())
	{
		glm::ivec2 pTile = player.getTilePosition(map);

		bool bombAlreadyThere = false;
		for(const auto &b : bombs)
		{
			if(b.getTilePosition() == pTile && !b.hasExploded())
			{
				bombAlreadyThere = true;
				break;
			}
		}

		if(!bombAlreadyThere)
		{
			Bomb b;
			b.init(pTile, map->tileToWorld(pTile), player.getBombRange());
			bombs.push_back(b);
			player.bombPlaced();
		}
	}
}

void Scene::handleExplosions()
{
	for(size_t i = 0; i < bombs.size(); i++)
	{
		if(bombs[i].hasExploded() && !bombs[i].isProcessed())
		{
			bombs[i].markProcessed();
			player.bombExploded();

			Explosion exp;
			exp.init(bombs[i].getTilePosition(), bombs[i].getRange(), map);
			explosions.push_back(exp);

			// Destroy destructible blocks & chance to spawn power-up
			for(const auto &blockPos : exp.getDestroyedBlocks())
			{
				map->destroyTile(blockPos.x, blockPos.y);
				if((rand() % 100) < 40) // 40% chance of power-up
				{
					spawnPowerUp(blockPos);
				}
			}

			// Chain reaction: any bomb caught in affected tiles explodes immediately
			for(size_t j = 0; j < bombs.size(); j++)
			{
				if(!bombs[j].hasExploded())
				{
					for(const auto &t : exp.getAffectedTiles())
					{
						if(bombs[j].getTilePosition() == t)
						{
							bombs[j].explode();
							break;
						}
					}
				}
			}

			// Check player in blast
			glm::ivec2 pTile = player.getTilePosition(map);
			for(const auto &t : exp.getAffectedTiles())
			{
				if(t == pTile)
				{
					player.kill();
					break;
				}
			}

			// Check enemies in blast
			for(auto &enemy : enemies)
			{
				if(enemy.isAlive())
				{
					glm::ivec2 eTile = enemy.getTilePosition(map);
					for(const auto &t : exp.getAffectedTiles())
					{
						if(t == eTile)
						{
							enemy.kill();
							break;
						}
					}
				}
			}

			// Destroy uncollected power-ups caught in flame
			for(auto &pw : powerups)
			{
				if(!pw.isCollected())
				{
					for(const auto &t : exp.getAffectedTiles())
					{
						if(t == pw.getTilePosition())
						{
							pw.collect(); // burned
							break;
						}
					}
				}
			}
		}
	}
}

void Scene::handlePowerUpCollection()
{
	if(!player.isAlive())
		return;

	glm::ivec2 pTile = player.getTilePosition(map);
	for(auto &pw : powerups)
	{
		if(!pw.isCollected() && pw.getTilePosition() == pTile)
		{
			player.addPowerUp(pw.getType());
			pw.collect();
		}
	}
}

void Scene::checkEntityCollisions()
{
	if(!player.isAlive())
		return;

	float tSize = float(map->getBlockSize());
	glm::vec2 pPos = player.getHitboxPos();
	glm::vec2 pSize = player.getHitboxSize();

	for(const auto &enemy : enemies)
	{
		if(enemy.isAlive())
		{
			glm::vec2 ePos = enemy.getPosition() + glm::vec2(tSize * 0.125f);
			glm::vec2 eSize(tSize * 0.75f);

			// AABB overlap check
			bool overlapX = (pPos.x < ePos.x + eSize.x) && (pPos.x + pSize.x > ePos.x);
			bool overlapY = (pPos.y < ePos.y + eSize.y) && (pPos.y + pSize.y > ePos.y);

			if(overlapX && overlapY)
			{
				player.kill();
				break;
			}
		}
	}
}

void Scene::spawnPowerUp(const glm::ivec2 &tilePos)
{
	int roll = rand() % 3;
	PowerUpType type = PowerUpType(roll);
	PowerUp pw;
	pw.init(tilePos, map->tileToWorld(tilePos), type);
	powerups.push_back(pw);
}

void Scene::cleanupEntities()
{
	for(auto it = explosions.begin(); it != explosions.end(); )
	{
		if(it->isFinished())
			it = explosions.erase(it);
		else
			++it;
	}

	for(auto it = bombs.begin(); it != bombs.end(); )
	{
		if(it->isProcessed())
			it = bombs.erase(it);
		else
			++it;
	}

	for(auto it = powerups.begin(); it != powerups.end(); )
	{
		if(it->isCollected())
			it = powerups.erase(it);
		else
			++it;
	}

	for(auto it = enemies.begin(); it != enemies.end(); )
	{
		if(!it->isAlive())
			it = enemies.erase(it);
		else
			++it;
	}
}

