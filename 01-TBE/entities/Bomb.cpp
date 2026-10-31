#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include "Bomb.h"
#include "Texture.h"
#include "ShaderProgram.h"


static const float BOMB_SHEET_W = 253.0f;
static const float BOMB_SHEET_H = 632.0f;

void Bomb::drawSheetRect(ShaderProgram &texProgram, const glm::vec2 &worldPos, const glm::vec2 &worldSize,
                         int srcX, int srcY, int srcW, int srcH)
{
	static Texture sharedSheet;
	static GLuint sharedVAO = 0;
	static GLuint sharedVBO = 0;
	static GLint posLoc = -1;
	static GLint texLoc = -1;

	if(sharedSheet.width() == 0)
	{
		sharedSheet.loadFromFile("images/caracters.png", TEXTURE_PIXEL_FORMAT_RGBA);
		sharedSheet.setWrapS(GL_CLAMP_TO_EDGE);
		sharedSheet.setWrapT(GL_CLAMP_TO_EDGE);
		sharedSheet.setMinFilter(GL_NEAREST);
		sharedSheet.setMagFilter(GL_NEAREST);
	}

	if(sharedVAO == 0)
	{
		glGenVertexArrays(1, &sharedVAO);
		glBindVertexArray(sharedVAO);
		glGenBuffers(1, &sharedVBO);
		glBindBuffer(GL_ARRAY_BUFFER, sharedVBO);
		glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
		posLoc = texProgram.bindVertexAttribute("position", 2, 4 * sizeof(float), 0);
		texLoc = texProgram.bindVertexAttribute("texCoord", 2, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	}

	float u0 = float(srcX) / BOMB_SHEET_W;
	float v0 = float(srcY) / BOMB_SHEET_H;
	float u1 = float(srcX + srcW) / BOMB_SHEET_W;
	float v1 = float(srcY + srcH) / BOMB_SHEET_H;

	float vertices[24] = {
		0.0f,        0.0f,        u0, v0,
		worldSize.x, 0.0f,        u1, v0,
		worldSize.x, worldSize.y, u1, v1,
		0.0f,        0.0f,        u0, v0,
		worldSize.x, worldSize.y, u1, v1,
		0.0f,        worldSize.y, u0, v1
	};

	glm::mat4 modelview = glm::translate(glm::mat4(1.0f), glm::vec3(worldPos.x, worldPos.y, 0.0f));
	texProgram.setUniformMatrix4f("modelview", modelview);

	glEnable(GL_TEXTURE_2D);
	sharedSheet.use();
	glBindVertexArray(sharedVAO);
	glBindBuffer(GL_ARRAY_BUFFER, sharedVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glEnableVertexAttribArray(posLoc);
	glEnableVertexAttribArray(texLoc);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_TEXTURE_2D);
}

Bomb::Bomb()
{
	pos = glm::vec2(0.0f);
	tilePos_ = glm::ivec2(0);
	timer = 3000.0f; // 3 seconds total fuse countdown
	range = 3;
	active = false;
	exploded = false;
	processed = false;
}

void Bomb::init(const glm::ivec2 &tilePos, const glm::vec2 &worldPos, int bombRange)
{
	tilePos_ = tilePos;
	pos = worldPos;
	range = bombRange;
	timer = 3000.0f; // 3 seconds: changes sprite every 1 second
	active = true;
	exploded = false;
	processed = false;
}

void Bomb::update(int deltaTime)
{
	if(!active || exploded)
		return;

	timer -= float(deltaTime);
	if(timer <= 0.0f)
	{
		timer = 0.0f;
		exploded = true;
	}
}

int Bomb::getAnimStage() const
{
	// 3000..2000 ms -> stage 0 (long fuse)
	// 2000..1000 ms -> stage 1 (middle fuse)
	// 1000..0 ms    -> stage 2 (short fuse)
	if(timer > 2000.0f)
		return 0;
	if(timer > 1000.0f)
		return 1;
	return 2;
}

void Bomb::render(ShaderProgram &texProgram, float tileSize) const
{
	if(!active || exploded)
		return;

	// Row 3 of images/caracters.png (y = 51..66, 16x16 px):
	//   Stage 0: x = 57, y = 51
	//   Stage 1: x = 75, y = 51
	//   Stage 2: x = 92, y = 51
	static const int BOMB_SRC_X[3] = { 57, 75, 92 };
	int stage = getAnimStage();
	drawSheetRect(texProgram, pos, glm::vec2(tileSize, tileSize), BOMB_SRC_X[stage], 51, 16, 16);
}

glm::vec2 Bomb::getPosition() const
{
	return pos;
}

glm::ivec2 Bomb::getTilePosition() const
{
	return tilePos_;
}

glm::vec2 Bomb::getSize() const
{
	return glm::vec2(16.0f, 16.0f);
}

int Bomb::getRange() const
{
	return range;
}

bool Bomb::isActive() const
{
	return active;
}

void Bomb::deactivate()
{
	active = false;
}

bool Bomb::hasExploded() const
{
	return exploded;
}

bool Bomb::isProcessed() const
{
	return processed;
}

void Bomb::explode()
{
	timer = 0.0f;
	exploded = true;
}

void Bomb::markProcessed()
{
	processed = true;
	active = false;
}
