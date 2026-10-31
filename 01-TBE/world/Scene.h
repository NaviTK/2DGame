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
#include "Bomb.h"
#include "Explosion.h"
#include "PowerUp.h"
#include "ObjectPool.h"


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
class Scene
{

public:
	Scene();
	~Scene();

	void init();
	void loadLevel(int levelIndex);
	void update(int deltaTime);
	void render();

	bool isGameOver() const;
	bool isWin() const;

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
	bool prevKey1, prevKey2, prevKeyR, prevKeyH;
	bool showHitboxes; // Toggled with the 'H' key to show/hide debug collision boxes

	// Entities (pre-allocated in contiguous fixed-capacity Object Pools per enemy type)
	Player player;
	ObjectPool<PinkEnemy, MAX_ENEMIES> pinkEnemies;
	ObjectPool<KirbyEnemy, MAX_ENEMIES> kirbyEnemies;
	vector<Enemy *> activeEnemies; // Non-owning pointers into pinkEnemies / kirbyEnemies pools

	ObjectPool<Bomb, MAX_BOMBS> bombs;
	ObjectPool<Explosion, MAX_EXPLOSIONS> explosions;
	ObjectPool<PowerUp, MAX_POWERUPS> powerups;

	// Quad rendering for simple colored UI/debug shapes
	GLuint quadVAO, quadVBO;
	GLint quadPosLocation;

	bool gameOver, win;

};


#endif // _SCENE_INCLUDE
