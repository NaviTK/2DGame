#include <GL/glew.h>
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include "Sprite.h"


Sprite *Sprite::createSprite(const glm::vec2 &quadSize, const glm::vec2 &sizeInSpritesheet,
                             Texture *spritesheet, ShaderProgram *program)
{
	return new Sprite(quadSize, sizeInSpritesheet, spritesheet, program);
}

Sprite::Sprite(const glm::vec2 &quadSize, const glm::vec2 &sizeInSpritesheet,
               Texture *spritesheet, ShaderProgram *program)
{
	quadSize_ = quadSize;
	sizeInSpritesheet_ = sizeInSpritesheet;
	texture = spritesheet;
	shaderProgram = program;

	pos = glm::vec2(0.0f);
	currentAnimation = -1;
	currentKeyframe = 0;
	timeAnimation = 0.0f;

	float vertices[] = {
		0.f, 0.f, 0.f, 0.f,
		quadSize.x, 0.f, sizeInSpritesheet.x, 0.f,
		quadSize.x, quadSize.y, sizeInSpritesheet.x, sizeInSpritesheet.y,
		0.f, 0.f, 0.f, 0.f,
		quadSize.x, quadSize.y, sizeInSpritesheet.x, sizeInSpritesheet.y,
		0.f, quadSize.y, 0.f, sizeInSpritesheet.y
	};

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), vertices, GL_DYNAMIC_DRAW);
	posLocation = shaderProgram->bindVertexAttribute("position", 2, 4 * sizeof(float), 0);
	texCoordLocation = shaderProgram->bindVertexAttribute("texCoord", 2, 4 * sizeof(float), (void *)(2 * sizeof(float)));
}

Sprite::~Sprite()
{
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
}

void Sprite::update(int deltaTime)
{
	if(currentAnimation >= 0 && currentAnimation < int(animations.size()))
	{
		const Animation &anim = animations[currentAnimation];
		if(!anim.keyframes.empty())
		{
			timeAnimation += float(deltaTime);
			while(timeAnimation > anim.millisecsPerKeyframe)
			{
				timeAnimation -= anim.millisecsPerKeyframe;
				currentKeyframe = (currentKeyframe + 1) % anim.keyframes.size();
			}
		}
	}
}

void Sprite::render() const
{
	if(currentAnimation >= 0 && currentAnimation < int(animations.size()))
	{
		const Animation &anim = animations[currentAnimation];
		if(!anim.keyframes.empty())
		{
			glm::vec2 disp = anim.keyframes[currentKeyframe].displacement;
			glm::vec2 texCoord0 = glm::vec2(disp.x * sizeInSpritesheet_.x, disp.y * sizeInSpritesheet_.y);
			glm::vec2 texCoord1 = texCoord0 + sizeInSpritesheet_;
			if(anim.keyframes[currentKeyframe].flipX)
				std::swap(texCoord0.x, texCoord1.x);

			float vertices[] = {
				0.f, 0.f, texCoord0.x, texCoord0.y,
				quadSize_.x, 0.f, texCoord1.x, texCoord0.y,
				quadSize_.x, quadSize_.y, texCoord1.x, texCoord1.y,
				0.f, 0.f, texCoord0.x, texCoord0.y,
				quadSize_.x, quadSize_.y, texCoord1.x, texCoord1.y,
				0.f, quadSize_.y, texCoord0.x, texCoord1.y
			};

			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, 24 * sizeof(float), vertices);
		}
	}

	glm::mat4 modelview = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, 0.0f));
	shaderProgram->setUniformMatrix4f("modelview", modelview);

	glEnable(GL_TEXTURE_2D);
	texture->use();
	glBindVertexArray(vao);
	glEnableVertexAttribArray(posLocation);
	glEnableVertexAttribArray(texCoordLocation);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_TEXTURE_2D);
}

void Sprite::setNumberAnimations(int nAnimations)
{
	animations.clear();
	animations.resize(nAnimations);
}

void Sprite::setAnimationSpeed(int animId, int keyframesPerSec)
{
	if(animId >= 0 && animId < int(animations.size()))
	{
		animations[animId].millisecsPerKeyframe = 1000.0f / float(keyframesPerSec);
	}
}

void Sprite::addKeyframe(int animId, const glm::vec2 &displacement, bool flipX)
{
	if(animId >= 0 && animId < int(animations.size()))
	{
		AnimKeyframe kf;
		kf.displacement = displacement;
		kf.flipX = flipX;
		animations[animId].keyframes.push_back(kf);
	}
}

void Sprite::changeAnimation(int animId)
{
	if(animId >= 0 && animId < int(animations.size()) && animId != currentAnimation)
	{
		currentAnimation = animId;
		currentKeyframe = 0;
		timeAnimation = 0.0f;
	}
}

void Sprite::setPosition(const glm::vec2 &newPos)
{
	pos = newPos;
}

glm::vec2 Sprite::position() const
{
	return pos;
}

int Sprite::animation() const
{
	return currentAnimation;
}

