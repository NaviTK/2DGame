#ifndef _SCENE_INCLUDE
#define _SCENE_INCLUDE


#include <vector>
#include <glm/glm.hpp>
#include "ShaderProgram.h"
#include "TileMap.h"
#include "Player.h"
#include "Enemy.h"
#include "Bomb.h"
#include "Explosion.h"
#include "PowerUp.h"


using namespace std;


#define CAMERA_WIDTH 640
#define CAMERA_HEIGHT 480


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
	void update(int deltaTime);
	void render();

	bool isGameOver() const;
	bool isWin() const;

private:
	void initShaders();
	void initQuadVAO();
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

	// Entities
	Player player;
	vector<Enemy> enemies;
	vector<Bomb> bombs;
	vector<Explosion> explosions;
	vector<PowerUp> powerups;

	// Quad rendering for entities (uses simple shader)
	GLuint quadVAO, quadVBO;
	GLint quadPosLocation;

	bool gameOver, win;

};


#endif // _SCENE_INCLUDE

