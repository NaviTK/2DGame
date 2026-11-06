#ifndef _SCENE_INCLUDE
#define _SCENE_INCLUDE


#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "ShaderProgram.h"
#include "TileMap.h"
#include "Player.h"
#include "Enemy.h"
#include "PinkEnemy.h"
#include "KirbyEnemy.h"
#include "JumperEnemy.h"
#include "BossEnemy.h"
#include "Bomb.h"
#include "Explosion.h"
#include "PowerUp.h"
#include "ObjectPool.h"
#include "SaveSystem.h"


using namespace std;


#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480

#define MAX_ENEMIES    16
#define MAX_BOMBS      16
#define MAX_EXPLOSIONS 16
#define MAX_POWERUPS   32


// Scene contains all the entities of our Bomberman game.
// It is responsible for updating and rendering them, and for
// handling the game logic (bomb placement, explosions, collisions,
// power-up collection, win/lose conditions).
// Menus, pause and HUD are handled by Game / MenuSystem on top of the Scene.
class Scene
{

public:
	Scene();
	~Scene();

	void init();
	void loadLevel(int levelIndex, bool preservePlayerStats = false);
	void update(int deltaTime);
	void render();

	bool isGameOver() const;
	bool isWin() const;
	int getCurrentLevel() const { return currentLevel; }
	const Player &getPlayer() const { return player; }
	Player &getPlayerMutable() { return player; }
	ShaderProgram &getTexProgram() { return texProgram; }
	ShaderProgram &getSimpleProgram() { return simpleProgram; }
	float getElapsedTime() const { return currentTime; }
	float getRemainingTime() const { return remainingTimeMs_; }
	float getTimeLimit() const { return timeLimitMs_; }
	int getEnemiesRemaining() const;
	const Enemy *getActiveBoss() const;
	int getScore() const { return score; }
	bool levelHasExitDoor() const { return hasExitDoor_; }
	bool isDoorOpen() const { return hasExitDoor_ && getEnemiesRemaining() == 0; }
	bool isPlayerOnExitDoor() const;
	void killAllEnemies();
	bool consumeTimeoutRestartEvent();

	void applySaveData(const SaveSlotData &data);
	void exportSaveData(SaveSlotData &data) const;
	void syncPlayerInputLatch();
	bool consumeWorldClearedEvent(int &outClearedWorld);
	bool consumePickedPowerUp(PowerUpType &outType);

private:
	void initShaders();
	void initQuadVAO();
	void updateCamera(int deltaTime, bool instant = false);
	void renderQuad(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color);
	void renderWireQuad(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color);
	void renderDebugDraw();

	void handleBombPlacement();
	void handleExplosions();
	void handlePowerUpCollection();
	void checkEntityCollisions();
	void spawnPowerUp(const glm::ivec2 &tilePos);
	void cleanupEntities();

private:
	TileMap *map;
	ShaderProgram texProgram, simpleProgram;
	float currentTime;
	glm::mat4 projection;
	glm::vec2 cameraCenter;
	glm::vec2 cameraViewSize;
	int currentLevel;
	bool prevKeyH;
	bool showHitboxes; // Toggled with the 'H' key to show/hide debug collision boxes

	// Entities (pre-allocated in contiguous fixed-capacity Object Pools per enemy type)
	Player player;
	ObjectPool<PinkEnemy, MAX_ENEMIES> pinkEnemies;
	ObjectPool<KirbyEnemy, MAX_ENEMIES> kirbyEnemies;
	ObjectPool<JumperEnemy, MAX_ENEMIES> jumperEnemies;
	ObjectPool<BossEnemy, 4> bossEnemies;
	vector<Enemy *> activeEnemies; // Non-owning pointers into enemy pools

	ObjectPool<Bomb, MAX_BOMBS> bombs;
	ObjectPool<Explosion, MAX_EXPLOSIONS> explosions;
	ObjectPool<PowerUp, MAX_POWERUPS> powerups;

	// Quad rendering for simple colored UI/debug shapes
	GLuint quadVAO, quadVBO;
	GLint quadPosLocation;

	float timeLimitMs_;
	float remainingTimeMs_;
	bool timeoutRestartPending_;
	bool gameOver, win;
	bool worldClearedPending_;
	int lastClearedWorld_;
	bool powerUpPickedPending_;
	PowerUpType lastPickedPowerUp_;
	bool hasExitDoor_;
	bool levelEnding_;     // Player died or cleared the level; short delay before the result screen
	bool endingIsWin_;
	float endTimer_;
	int score;
	vector<bool> enemyCounted_; // Tracks which enemies already granted score

};


#endif // _SCENE_INCLUDE
