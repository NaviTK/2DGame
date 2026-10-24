#ifndef _SPRITE_INCLUDE
#define _SPRITE_INCLUDE


#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include "Texture.h"
#include "ShaderProgram.h"


using namespace std;


// Sprite renders an animated quad from a spritesheet texture.
// It supports multiple named animations, each composed of a
// sequence of keyframes (positions in the spritesheet grid).
//
// Usage:
//   Sprite *spr = Sprite::createSprite(quadSize, frameSizeNorm, &tex, &prog);
//   spr->setNumberAnimations(4);
//   spr->setAnimationSpeed(0, 8);            // 8 fps
//   spr->addKeyframe(0, glm::vec2(0, 0));    // column 0, row 0
//   spr->addKeyframe(0, glm::vec2(1, 0));    // column 1, row 0
//   spr->changeAnimation(0);
//   // in game loop:
//   spr->setPosition(worldPos);
//   spr->update(deltaTime);
//   spr->render();


class Sprite
{

private:
	Sprite(const glm::vec2 &quadSize, const glm::vec2 &sizeInSpritesheet,
	       Texture *spritesheet, ShaderProgram *program);

public:
	static Sprite *createSprite(const glm::vec2 &quadSize, const glm::vec2 &sizeInSpritesheet,
	                            Texture *spritesheet, ShaderProgram *program);
	~Sprite();

	void update(int deltaTime);
	void render() const;

	void setNumberAnimations(int nAnimations);
	void setAnimationSpeed(int animId, int keyframesPerSec);
	void addKeyframe(int animId, const glm::vec2 &displacement);
	void changeAnimation(int animId);

	void setPosition(const glm::vec2 &pos);
	glm::vec2 position() const;

	int animation() const;

private:
	struct AnimKeyframe
	{
		glm::vec2 displacement;
	};

	struct Animation
	{
		float millisecsPerKeyframe;
		vector<AnimKeyframe> keyframes;
	};

	Texture *texture;
	ShaderProgram *shaderProgram;
	GLuint vao, vbo;
	GLint posLocation, texCoordLocation;
	glm::vec2 pos, quadSize_, sizeInSpritesheet_;
	int currentAnimation, currentKeyframe;
	float timeAnimation;
	vector<Animation> animations;

};


#endif // _SPRITE_INCLUDE

