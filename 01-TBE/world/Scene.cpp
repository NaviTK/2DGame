#include <iostream>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Scene.h"
#include "Game.h"


using namespace std;


static const float DEATH_DELAY_MS = 1400.0f; // Time to show the defeated player before GAME OVER
static const float CLEAR_DELAY_MS = 900.0f;  // Short celebration before the WORLD CLEAR screen
static const int   SCORE_ENEMY    = 100;
static const int   SCORE_BLOCK    = 10;
static const int   SCORE_POWERUP  = 50;


Scene::Scene()
{
	map = NULL;
	quadVAO = 0;
	quadVBO = 0;
	gameOver = false;
	win = false;
	currentTime = 0.0f;
	timeLimitMs_ = 120000.0f;
	remainingTimeMs_ = 120000.0f;
	timeoutRestartPending_ = false;
	cameraCenter = glm::vec2(0.0f);
	cameraViewSize = glm::vec2(160.0f, 120.0f);
	currentLevel = 1;
	prevKeyH = false;
	showHitboxes = false;
	worldClearedPending_ = false;
	lastClearedWorld_ = 0;
	powerUpPickedPending_ = false;
	lastPickedPowerUp_ = EXTRA_BOMB;
	hasExitDoor_ = false;
	levelEnding_ = false;
	endingIsWin_ = false;
	endTimer_ = 0.0f;
	score = 0;
	activeEnemies.reserve(MAX_ENEMIES * 3 + 4);
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
	loadLevel(1);
}

void Scene::loadLevel(int levelIndex, bool preservePlayerStats)
{
	int savedLives = player.getLives();
	int savedMaxBombs = player.getMaxBombs();
	int savedBombRange = player.getBombRange();
	int savedSpeedUps = player.getSpeedUps();
	bool savedHeart = player.hasHeartShield();
	bool savedGodMode = player.isGodMode();
	bool hadAlivePlayer = preservePlayerStats && player.isAlive();
	currentLevel = levelIndex;
	currentTime = 0.0f;
	gameOver = false;
	win = false;
	worldClearedPending_ = false;
	powerUpPickedPending_ = false;
	levelEnding_ = false;
	endingIsWin_ = false;
	endTimer_ = 0.0f;
	score = 0;
	bombs.releaseAll();
	explosions.releaseAll();
	powerups.releaseAll();
	pinkEnemies.releaseAll();
	kirbyEnemies.releaseAll();
	jumperEnemies.releaseAll();
	bossEnemies.releaseAll();
	activeEnemies.clear();
	enemyCounted_.clear();

	if(map != NULL)
	{
		delete map;
		map = NULL;
	}

	const WorldDef *wDef = SaveSystem::instance().getWorldById(levelIndex);
	string levelFile = "levels/level01.txt";
	if(wDef != NULL)
		levelFile = wDef->levelFile;
	else if(levelIndex == 5)
		levelFile = "levels/level05.txt";
	else if(levelIndex == 4)
		levelFile = "levels/level04.txt";
	else if(levelIndex == 3)
		levelFile = "levels/level03.txt";
	else if(levelIndex == 2)
		levelFile = "levels/level02.txt";

	map = TileMap::createTileMap(levelFile, glm::vec2(0.0f), texProgram);
	timeLimitMs_ = float(map->getTimeLimitSeconds()) * 1000.0f;
	remainingTimeMs_ = timeLimitMs_;

	float tSize = float(map->getBlockSize());

	// Worlds with an exit door are cleared by reaching it after defeating every enemy;
	// arena worlds without a door are cleared as soon as every enemy is defeated.
	hasExitDoor_ = false;
	for(int j = 0; j < map->getMapHeight() && !hasExitDoor_; j++)
		for(int i = 0; i < map->getMapWidth() && !hasExitDoor_; i++)
			if(map->isExitDoor(i, j))
				hasExitDoor_ = true;

	if(!map->getPlayerSpawns().empty())
		player.init(map->tileToWorld(map->getPlayerSpawns()[0]), tSize);
	else
		player.init(map->tileToWorld(glm::ivec2(1, 1)), tSize);
	player.initSprite(texProgram);
	player.setGodMode(savedGodMode);
	if(hadAlivePlayer)
		player.applyStats(savedLives, savedMaxBombs, savedBombRange, savedSpeedUps, savedHeart);
	player.syncInputLatch();

	// Spawn enemies from the level configuration tags (ENEMY_SPAWN <TYPE> <x> <y>)
	for(const auto &spawn : map->getEnemySpawns())
	{
		Enemy *enemy = NULL;
		if(spawn.type == "KIRBY")
			enemy = kirbyEnemies.acquire();
		else if(spawn.type == "JUMPER")
			enemy = jumperEnemies.acquire();
		else if(spawn.type == "BOSS")
			enemy = bossEnemies.acquire();
		else
			enemy = pinkEnemies.acquire();

		if(enemy != NULL)
		{
			enemy->init(map->tileToWorld(spawn.tilePos), tSize);
			enemy->initSprite(texProgram);
			activeEnemies.push_back(enemy);
			enemyCounted_.push_back(false);
		}
	}

	// Initialize Pocket Bomberman camera centered on the player and clamped to map bounds
	updateCamera(0, true);
}

void Scene::updateCamera(int deltaTime, bool instant)
{
	if(map == NULL)
		return;

	float tSize = float(map->getBlockSize());
	glm::vec2 minCoords = map->getMinCoords();
	float mapW = map->getMapWidth() * tSize;
	float mapH = map->getMapHeight() * tSize;

	// Pocket Bomberman (Game Boy) style zoomed viewport: 10 x 7.5 tiles (160x120 px for 16px tiles),
	// preserving the 4:3 window aspect ratio while magnifying sprites 6x on a 960x720 window.
	float viewW = 10.0f * tSize;
	float viewH = 7.5f * tSize;
	cameraViewSize = glm::vec2(viewW, viewH);

	glm::vec2 target = player.getHitboxPos() + player.getHitboxSize() * 0.5f;

	if(instant)
	{
		cameraCenter = target;
	}
	else
	{
		int dt = (deltaTime > 40) ? 40 : deltaTime;
		float followFactor = std::min(1.0f, 0.015f * float(dt));
		cameraCenter += (target - cameraCenter) * followFactor;
	}

	// Clamp camera inside the level bounds so nothing outside the outer walls is shown
	float halfW = viewW * 0.5f;
	float halfH = viewH * 0.5f;

	if(mapW <= viewW)
		cameraCenter.x = minCoords.x + mapW * 0.5f;
	else
		cameraCenter.x = std::max(minCoords.x + halfW, std::min(cameraCenter.x, minCoords.x + mapW - halfW));

	if(mapH <= viewH)
		cameraCenter.y = minCoords.y + mapH * 0.5f;
	else
		cameraCenter.y = std::max(minCoords.y + halfH, std::min(cameraCenter.y, minCoords.y + mapH - halfH));

	projection = glm::ortho(cameraCenter.x - halfW, cameraCenter.x + halfW,
	                        cameraCenter.y + halfH, cameraCenter.y - halfH);
}

void Scene::update(int deltaTime)
{
	// 'H' toggles the debug hitbox overlay (works even on game over / win screens)
	bool kH = Game::instance().getKey(GLFW_KEY_H);
	if(kH && !prevKeyH)
		showHitboxes = !showHitboxes;
	prevKeyH = kH;

	if(gameOver || win)
		return;

	// Short delay after dying / clearing so the player can see what happened
	if(levelEnding_)
	{
		endTimer_ -= float(deltaTime);
		for(auto &exp : explosions)
			if(exp.isActive())
				exp.update(deltaTime);
		cleanupEntities();
		if(endTimer_ <= 0.0f)
		{
			if(endingIsWin_)
			{
				win = true;
				worldClearedPending_ = true;
				lastClearedWorld_ = currentLevel;
			}
			else
			{
				gameOver = true;
			}
		}
		return;
	}

	currentTime += float(deltaTime);

	// Descending countdown timer per level:
	// If it reaches zero before beating the level, the player loses 1 life and restarts the level
	// (or immediate Game Over if 0 lives remain).
	remainingTimeMs_ -= float(deltaTime);
	if(remainingTimeMs_ <= 0.0f)
	{
		remainingTimeMs_ = 0.0f;
		if(player.loseLifeFromTimeout())
		{
			timeoutRestartPending_ = true;
			loadLevel(currentLevel, true);
			return;
		}
		else
		{
			gameOver = true;
			return;
		}
	}

	if(map != NULL)
		map->update(deltaTime);

	// Feed active bomb positions so the player can stand/bomb-jump on placed bombs
	static vector<glm::vec2> activeBombPositions;
	activeBombPositions.clear();
	for(size_t i = 0; i < bombs.capacity(); i++)
	{
		if(bombs[i].isActive() && !bombs[i].hasExploded())
			activeBombPositions.push_back(bombs[i].getPosition());
	}
	player.setBombPlatforms(activeBombPositions);

	player.update(deltaTime, map);

	glm::vec2 pHitboxPos = player.getHitboxPos();
	glm::vec2 pHitboxSize = player.getHitboxSize();
	bool pAlive = player.isAlive();

	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(activeEnemies[i]->isActive())
		{
			activeEnemies[i]->setTargetPlayer(pHitboxPos, pHitboxSize, pAlive);
			activeEnemies[i]->update(deltaTime, map);
		}
	}

	for(size_t i = 0; i < bombs.capacity(); i++)
	{
		if(bombs[i].isActive())
			bombs[i].update(deltaTime, map);
	}

	for(size_t i = 0; i < explosions.capacity(); i++)
	{
		if(explosions[i].isActive())
			explosions[i].update(deltaTime);
	}

	for(size_t i = 0; i < powerups.capacity(); i++)
	{
		if(powerups[i].isActive())
			powerups[i].update(deltaTime);
	}

	handleBombPlacement();
	handleExplosions();
	handlePowerUpCollection();
	checkEntityCollisions();
	cleanupEntities();
	updateCamera(deltaTime, false);

	// Award score for enemies defeated this frame
	bool allEnemiesCleared = true;
	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(activeEnemies[i]->isAlive())
		{
			allEnemiesCleared = false;
		}
		else if(i < enemyCounted_.size() && !enemyCounted_[i])
		{
			enemyCounted_[i] = true;
			score += SCORE_ENEMY;
		}
	}

	// Losing all 3 lives triggers immediate Game Over (returning directly to Main Menu)
	if(!player.isAlive())
	{
		gameOver = true;
		return;
	}

	// Level clear condition:
	// 1. Eliminate all enemies on the map (including Boss on Level 5).
	// 2. Exit door activates/opens.
	// 3. Bomberman physically reaches the exit door to advance to the next level.
	bool cleared = false;
	if(hasExitDoor_)
		cleared = allEnemiesCleared && isPlayerOnExitDoor();
	else
		cleared = allEnemiesCleared;

	if(cleared)
	{
		win = true;
		worldClearedPending_ = true;
		lastClearedWorld_ = currentLevel;
	}
}

bool Scene::isPlayerOnExitDoor() const
{
	if(map == NULL)
		return false;
	glm::ivec2 pTile = player.getTilePosition(map);
	return map->isExitDoor(pTile.x, pTile.y);
}

int Scene::getEnemiesRemaining() const
{
	int count = 0;
	for(size_t i = 0; i < activeEnemies.size(); i++)
		if(activeEnemies[i]->isAlive())
			count++;
	return count;
}

const Enemy *Scene::getActiveBoss() const
{
	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(activeEnemies[i]->isActive() && activeEnemies[i]->isBoss())
			return activeEnemies[i];
	}
	return NULL;
}

void Scene::killAllEnemies()
{
	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(activeEnemies[i]->isAlive() || activeEnemies[i]->isActive())
		{
			activeEnemies[i]->forceKill();
			if(i < enemyCounted_.size() && !enemyCounted_[i])
			{
				enemyCounted_[i] = true;
				score += SCORE_ENEMY;
			}
		}
	}
}

bool Scene::consumeTimeoutRestartEvent()
{
	if(!timeoutRestartPending_)
		return false;
	timeoutRestartPending_ = false;
	return true;
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

	// 1b. Exit door visual state:
	//     - Closed (red/dark lock overlay) while enemies remain alive
	//     - Open (golden pulsing glow) once all enemies have been eliminated
	if(hasExitDoor_)
	{
		bool open = (getEnemiesRemaining() == 0);
		float alpha = open ? (0.26f + 0.18f * std::sin(currentTime * 0.008f)) : 0.42f;
		glm::vec4 doorOverlay = open ? glm::vec4(1.0f, 0.92f, 0.25f, alpha)
		                             : glm::vec4(0.18f, 0.05f, 0.08f, alpha);
		for(int j = 0; j < map->getMapHeight(); j++)
		{
			for(int i = 0; i < map->getMapWidth(); i++)
			{
				if(map->isExitDoor(i, j))
					renderQuad(map->tileToWorld(glm::ivec2(i, j)), glm::vec2(tSize, tSize), doorOverlay);
			}
		}
	}

	// 2. Render power-ups (Pocket Bomberman item panels from caracters.png)
	texProgram.use();
	texProgram.setUniformMatrix4f("projection", projection);
	for(const auto &pw : powerups)
	{
		if(pw.isActive() && !pw.isCollected())
			pw.render(texProgram, tSize);
	}

	// 3. Render bombs (3-second animated fuse from row 3 of caracters.png)
	texProgram.use();
	texProgram.setUniformMatrix4f("projection", projection);
	texProgram.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	for(const auto &b : bombs)
	{
		if(b.isActive() && !b.hasExploded())
		{
			b.render(texProgram, tSize);
		}
	}

	// 4. Render explosions (directional sprites from rows 4, 5, 6 of caracters.png)
	for(const auto &exp : explosions)
	{
		if(exp.isActive() && !exp.isFinished())
		{
			exp.render(texProgram, map);
		}
	}

	// 5. Render enemies (animated sprites from caracters.png)
	texProgram.use();
	texProgram.setUniformMatrix4f("projection", projection);
	texProgram.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(activeEnemies[i]->isActive())
		{
			activeEnemies[i]->render();
		}
	}

	// 6. Render player (animated sprite, greyed out when dead, rainbow tint with the Vest)
	texProgram.use();
	texProgram.setUniformMatrix4f("projection", projection);
	if(!player.isAlive())
	{
		texProgram.setUniform4f("color", 0.4f, 0.4f, 0.4f, 1.0f);
	}
	else if(player.hasVest())
	{
		// Flash faster during the last 2 seconds to warn the vest is about to expire
		float vt = player.getVestTimer();
		float rate = (vt < 2000.0f) ? 0.03f : 0.012f;
		float t = currentTime * rate;
		texProgram.setUniform4f("color",
		                        1.0f + 0.6f * std::sin(t),
		                        1.0f + 0.6f * std::sin(t + 2.1f),
		                        1.0f + 0.6f * std::sin(t + 4.2f), 1.0f);
	}
	else
	{
		texProgram.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	}
	player.render();
	texProgram.setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);

	// 7. Debug Draw: Colisiones (toggle with 'H')
	if(showHitboxes)
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

void Scene::applySaveData(const SaveSlotData &data)
{
	player.applyStats(data.lives, data.maxBombs, data.bombRange, data.speedUps);
}

void Scene::exportSaveData(SaveSlotData &data) const
{
	data.currentWorld = currentLevel;
	data.lives = std::max(1, player.getLives());
	data.maxBombs = player.getMaxBombs();
	data.bombRange = player.getBombRange();
	data.speedUps = player.getSpeedUps();
}

void Scene::syncPlayerInputLatch()
{
	prevKeyH = Game::instance().getKey(GLFW_KEY_H);
	player.syncInputLatch();
}

bool Scene::consumeWorldClearedEvent(int &outClearedWorld)
{
	if(!worldClearedPending_)
		return false;
	outClearedWorld = lastClearedWorld_;
	worldClearedPending_ = false;
	return true;
}

bool Scene::consumePickedPowerUp(PowerUpType &outType)
{
	if(!powerUpPickedPending_)
		return false;
	outType = lastPickedPowerUp_;
	powerUpPickedPending_ = false;
	return true;
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

	glBindVertexArray(quadVAO);
	glEnableVertexAttribArray(quadPosLocation);
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

	// 1. Dibujar hitboxes de los tiles del mapa:
	//    - VERDE: muros/suelo sólidos completos
	//    - AMARILLO: plataformas one-way (atravesables desde abajo)
	//    - CIAN: enredaderas/lianas trepables
	for(int j = 0; j < map->getMapHeight(); j++)
	{
		for(int i = 0; i < map->getMapWidth(); i++)
		{
			glm::vec2 tileWorldPos = minCoords + glm::vec2(i * tSize, j * tSize);
			if(!map->isWalkable(i, j))
			{
				renderWireQuad(tileWorldPos, glm::vec2(tSize, tSize), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
			}
			else if(map->isOneWayPlatform(i, j))
			{
				renderWireQuad(tileWorldPos, glm::vec2(tSize, tSize), glm::vec4(1.0f, 0.9f, 0.1f, 1.0f));
			}
			else if(map->isClimbable(i, j))
			{
				renderWireQuad(tileWorldPos, glm::vec2(tSize, tSize), glm::vec4(0.1f, 0.85f, 1.0f, 1.0f));
			}
		}
	}

	// 2. Dibujar hitbox de cada Enemigo vivo en MAGENTA
	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(activeEnemies[i]->isAlive())
		{
			renderWireQuad(activeEnemies[i]->getHitboxPos(), activeEnemies[i]->getHitboxSize(), glm::vec4(1.0f, 0.2f, 0.85f, 1.0f));
		}
	}

	// 3. Dibujar hitbox del Player en ROJO
	renderWireQuad(player.getHitboxPos(), player.getHitboxSize(), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

void Scene::handleBombPlacement()
{
	if(player.wantsToPlaceBomb())
	{
		glm::ivec2 pTile = player.getTilePosition(map);

		// Bombs can only be placed on walkable cells (never inside a wall)
		bool blocked = !map->isWalkable(pTile.x, pTile.y);
		for(const auto &b : bombs)
		{
			if(b.isActive() && b.getTilePosition() == pTile && !b.hasExploded())
			{
				blocked = true;
				break;
			}
		}

		if(!blocked)
		{
			Bomb *b = bombs.acquire();
			if(b != NULL)
			{
				b->init(pTile, map->tileToWorld(pTile), player.getBombRange());
				player.bombPlaced();
				return;
			}
		}
		// Could not place the bomb: consume the request so it doesn't trigger later
		player.syncInputLatch();
	}
}

void Scene::handleExplosions()
{
	// 1. Spawn a new progressive Explosion when a Bomb finishes its 3-second countdown
	for(size_t i = 0; i < bombs.capacity(); i++)
	{
		if(bombs[i].isActive() && bombs[i].hasExploded() && !bombs[i].isProcessed())
		{
			glm::ivec2 bombTile = bombs[i].getTilePosition();
			int bombRange = bombs[i].getRange();

			bombs[i].markProcessed();
			player.bombExploded();

			Explosion *exp = explosions.acquire();
			if(exp != NULL)
			{
				exp->init(bombTile, bombRange, map);
			}
		}
	}

	// 2. Process all active Explosions as their flames expand step-by-step (0.1s per tile)
	static vector<glm::ivec2> newlyDestroyed;
	for(auto &exp : explosions)
	{
		if(!exp.isActive() || exp.isFinished())
			continue;

		// Destroy destructible grass blocks newly reached by the expanding flame on this step
		exp.popNewlyDestroyedBlocks(newlyDestroyed);
		for(const auto &blockPos : newlyDestroyed)
		{
			map->destroyTile(blockPos.x, blockPos.y);
			score += SCORE_BLOCK;
			if((rand() % 100) < 40) // 40% chance of power-up
			{
				spawnPowerUp(blockPos);
			}
		}

		// Toggle vine retraction/expansion once when the expanding flame reaches a vine head block
		if(!exp.hasToggledVine())
		{
			for(const auto &t : exp.getAffectedTiles())
			{
				if(map->toggleVineAtHead(t.x, t.y))
				{
					exp.markVineToggled();
					break;
				}
			}
		}

		// Chain reaction: any bomb caught in currently visible flame tiles explodes immediately
		for(size_t j = 0; j < bombs.capacity(); j++)
		{
			if(bombs[j].isActive() && !bombs[j].hasExploded())
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

		// Check player in currently visible flame tiles (loses 1 life, 2s invulnerability, and knockback)
		glm::ivec2 pTile = player.getTilePosition(map);
		for(const auto &t : exp.getAffectedTiles())
		{
			if(t == pTile)
			{
				if(!player.isInvulnerable())
				{
					float tSize = float(map->getBlockSize());
					glm::vec2 pCenter = player.getHitboxPos() + player.getHitboxSize() * 0.5f;
					glm::vec2 bCenter = map->tileToWorld(exp.getCenterTile()) + glm::vec2(tSize * 0.5f);
					float dirX = (pCenter.x >= bCenter.x) ? 1.0f : -1.0f;
					player.takeDamage();
					player.applyKnockback(glm::vec2(dirX * tSize * 0.008f, -tSize * 0.011f));
				}
				break;
			}
		}

		// Check enemies (and multi-tile Boss) in currently visible flame tiles
		float tSize = float(map->getBlockSize());
		for(size_t k = 0; k < activeEnemies.size(); k++)
		{
			if(activeEnemies[k]->isAlive())
			{
				glm::vec2 ePos = activeEnemies[k]->getHitboxPos();
				glm::vec2 eSize = activeEnemies[k]->getHitboxSize();
				glm::ivec2 eTile = activeEnemies[k]->getTilePosition(map);
				for(const auto &t : exp.getAffectedTiles())
				{
					glm::vec2 flamePos = map->tileToWorld(t);
					bool boxOverlap = (ePos.x < flamePos.x + tSize && ePos.x + eSize.x > flamePos.x &&
					                   ePos.y < flamePos.y + tSize && ePos.y + eSize.y > flamePos.y);
					if(t == eTile || boxOverlap)
					{
						activeEnemies[k]->kill();
						break;
					}
				}
			}
		}

		// Destroy uncollected power-ups caught in currently visible flame tiles
		// (freshly revealed power-ups are protected by a short grace period)
		for(auto &pw : powerups)
		{
			if(pw.isActive() && !pw.isCollected() && pw.canBeBurned())
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

void Scene::handlePowerUpCollection()
{
	if(!player.isAlive())
		return;

	// AABB overlap between the player hitbox and the (slightly shrunk) item panel
	float tSize = float(map->getBlockSize());
	glm::vec2 pPos = player.getHitboxPos();
	glm::vec2 pSize = player.getHitboxSize();
	float inset = tSize * 0.15f;

	for(auto &pw : powerups)
	{
		if(!pw.isActive() || pw.isCollected())
			continue;

		glm::vec2 wPos = pw.getPosition() + glm::vec2(inset);
		glm::vec2 wSize = glm::vec2(tSize - 2.0f * inset);
		bool overlap = pPos.x < wPos.x + wSize.x && pPos.x + pSize.x > wPos.x &&
		               pPos.y < wPos.y + wSize.y && pPos.y + pSize.y > wPos.y;
		if(overlap)
		{
			lastPickedPowerUp_ = pw.getType();
			powerUpPickedPending_ = true;
			player.addPowerUp(int(pw.getType()));
			pw.collect();
			score += SCORE_POWERUP;
		}
	}
}

void Scene::checkEntityCollisions()
{
	if(!player.isAlive())
		return;

	float tSize = float(map->getBlockSize());
	glm::vec2 colOffset((tSize - player.getHitboxSize().x) * 0.5f,
	                    (tSize - player.getHitboxSize().y) * 0.5f);

	// 1. Enemy-vs-Enemy solid collisions using each enemy's custom hitbox
	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(!activeEnemies[i]->isAlive())
			continue;
		for(size_t j = i + 1; j < activeEnemies.size(); j++)
		{
			if(!activeEnemies[j]->isAlive())
				continue;

			glm::vec2 e1Pos = activeEnemies[i]->getHitboxPos();
			glm::vec2 e1Size = activeEnemies[i]->getHitboxSize();
			glm::vec2 e2Pos = activeEnemies[j]->getHitboxPos();
			glm::vec2 e2Size = activeEnemies[j]->getHitboxSize();

			float ox = std::min(e1Pos.x + e1Size.x, e2Pos.x + e2Size.x) - std::max(e1Pos.x, e2Pos.x);
			float oy = std::min(e1Pos.y + e1Size.y, e2Pos.y + e2Size.y) - std::max(e1Pos.y, e2Pos.y);
			if(ox > 0.0f && oy > 0.0f)
			{
				float push = ox * 0.5f;
				if(e1Pos.x < e2Pos.x)
				{
					activeEnemies[i]->setPosition(activeEnemies[i]->getPosition() - glm::vec2(push, 0.0f));
					activeEnemies[j]->setPosition(activeEnemies[j]->getPosition() + glm::vec2(push, 0.0f));
				}
				else
				{
					activeEnemies[i]->setPosition(activeEnemies[i]->getPosition() + glm::vec2(push, 0.0f));
					activeEnemies[j]->setPosition(activeEnemies[j]->getPosition() - glm::vec2(push, 0.0f));
				}
				activeEnemies[i]->reverseDirection();
				activeEnemies[j]->reverseDirection();
			}
		}
	}

	// 1b. Enemy-vs-Bomb horizontal bounce so patrolling enemies turn around at placed bombs
	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(!activeEnemies[i]->isAlive())
			continue;
		glm::vec2 ePos = activeEnemies[i]->getHitboxPos();
		glm::vec2 eSize = activeEnemies[i]->getHitboxSize();
		for(size_t b = 0; b < bombs.capacity(); b++)
		{
			if(!bombs[b].isActive() || bombs[b].hasExploded())
				continue;
			glm::vec2 bPos = bombs[b].getPosition();
			float ox = std::min(ePos.x + eSize.x, bPos.x + tSize) - std::max(ePos.x, bPos.x);
			float oy = std::min(ePos.y + eSize.y, bPos.y + tSize) - std::max(ePos.y, bPos.y);
			if(ox > 0.0f && oy > tSize * 0.25f && ox < tSize * 0.45f)
			{
				if(ePos.x < bPos.x)
					activeEnemies[i]->setPosition(activeEnemies[i]->getPosition() - glm::vec2(ox, 0.0f));
				else
					activeEnemies[i]->setPosition(activeEnemies[i]->getPosition() + glm::vec2(ox, 0.0f));
				activeEnemies[i]->reverseDirection();
				ePos = activeEnemies[i]->getHitboxPos();
			}
		}
	}

	// 2. Player-vs-Enemy solid collision, knockback & life loss using each enemy's custom hitbox
	glm::vec2 pPos = player.getHitboxPos();
	glm::vec2 pSize = player.getHitboxSize();

	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		Enemy *enemy = activeEnemies[i];
		if(!enemy->isAlive())
			continue;

		glm::vec2 ePos = enemy->getHitboxPos();
		glm::vec2 eSize = enemy->getHitboxSize();

		float overlapX = std::min(pPos.x + pSize.x, ePos.x + eSize.x) - std::max(pPos.x, ePos.x);
		float overlapY = std::min(pPos.y + pSize.y, ePos.y + eSize.y) - std::max(pPos.y, ePos.y);

		if(overlapX > 0.0f && overlapY > 0.0f)
		{
			// With the Vest active, touching an enemy defeats it instead of hurting the player
			if(player.hasVest())
			{
				enemy->kill();
				continue;
			}

			glm::vec2 pCenter = pPos + pSize * 0.5f;
			glm::vec2 eCenter = ePos + eSize * 0.5f;
			float signX = (pCenter.x < eCenter.x) ? -1.0f : 1.0f;
			float signY = (pCenter.y < eCenter.y) ? -1.0f : 1.0f;

			glm::vec2 newPlayerPos = player.getPosition();
			if(overlapX < overlapY)
			{
				newPlayerPos.x += signX * overlapX;
				if(signX < 0.0f && map->collisionMoveLeft(newPlayerPos + colOffset, pSize))
					newPlayerPos.x = player.getPosition().x;
				else if(signX > 0.0f && map->collisionMoveRight(newPlayerPos + colOffset, pSize))
					newPlayerPos.x = player.getPosition().x;
				enemy->reverseDirection();
			}
			else
			{
				newPlayerPos.y += signY * overlapY;
				if(signY < 0.0f && map->collisionMoveUp(newPlayerPos + colOffset, pSize))
					newPlayerPos.y = player.getPosition().y;
			}
			player.setPosition(newPlayerPos);

			if(!player.isInvulnerable())
			{
				player.takeDamage();
				player.applyKnockback(glm::vec2(signX * tSize * 0.006f, -tSize * 0.009f));
			}

			pPos = player.getHitboxPos();
		}
	}
}

void Scene::spawnPowerUp(const glm::ivec2 &tilePos)
{
	PowerUp *pw = powerups.acquire();
	if(pw != NULL)
	{
		pw->init(tilePos, map->tileToWorld(tilePos), PowerUp::rollRandomType());
	}
}

void Scene::cleanupEntities()
{
	for(auto &exp : explosions)
	{
		if(exp.isActive() && exp.isFinished())
			exp.deactivate();
	}

	for(auto &b : bombs)
	{
		if(b.isActive() && b.isProcessed())
			b.deactivate();
	}

	for(auto &pw : powerups)
	{
		if(pw.isActive() && pw.isCollected())
			pw.deactivate();
	}

	for(size_t i = 0; i < activeEnemies.size(); i++)
	{
		if(!activeEnemies[i]->isActive())
			activeEnemies[i]->deactivate();
	}
}
