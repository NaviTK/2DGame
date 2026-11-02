#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include "UIRenderer.h"
#include "Bomb.h"


UIRenderer::UIRenderer()
{
	texProgram_ = NULL;
	simpleProgram_ = NULL;
	colorVAO_ = colorVBO_ = 0;
	texVAO_ = texVBO_ = 0;
	heartTex_ = 0;
	colorPosLoc_ = texPosLoc_ = texCoordLoc_ = -1;
	initialized_ = false;
	projection_ = glm::mat4(1.0f);
}

UIRenderer::~UIRenderer()
{
	if(colorVAO_ != 0) glDeleteVertexArrays(1, &colorVAO_);
	if(colorVBO_ != 0) glDeleteBuffers(1, &colorVBO_);
	if(texVAO_ != 0) glDeleteVertexArrays(1, &texVAO_);
	if(texVBO_ != 0) glDeleteBuffers(1, &texVBO_);
	if(heartTex_ != 0) glDeleteTextures(1, &heartTex_);
}

void UIRenderer::init(ShaderProgram &texProgram, ShaderProgram &simpleProgram)
{
	if(initialized_)
		return;

	texProgram_ = &texProgram;
	simpleProgram_ = &simpleProgram;

	// Unit quad for solid colors (simple shader)
	float quad[] = {
		0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
		0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f
	};
	glGenVertexArrays(1, &colorVAO_);
	glBindVertexArray(colorVAO_);
	glGenBuffers(1, &colorVBO_);
	glBindBuffer(GL_ARRAY_BUFFER, colorVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
	colorPosLoc_ = simpleProgram.bindVertexAttribute("position", 2, 2 * sizeof(float), 0);

	// Unit quad with full UVs for whole textures (texture shader)
	float texQuad[] = {
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f,
		0.0f, 1.0f, 0.0f, 1.0f
	};
	glGenVertexArrays(1, &texVAO_);
	glBindVertexArray(texVAO_);
	glGenBuffers(1, &texVBO_);
	glBindBuffer(GL_ARRAY_BUFFER, texVBO_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texQuad), texQuad, GL_STATIC_DRAW);
	texPosLoc_ = texProgram.bindVertexAttribute("position", 2, 4 * sizeof(float), 0);
	texCoordLoc_ = texProgram.bindVertexAttribute("texCoord", 2, 4 * sizeof(float), (void *)(2 * sizeof(float)));

	font_.init(texProgram);
	buildHeartTexture();

	projection_ = glm::ortho(0.0f, float(VIRTUAL_W), float(VIRTUAL_H), 0.0f);
	initialized_ = true;
}

void UIRenderer::begin()
{
	simpleProgram_->use();
	simpleProgram_->setUniformMatrix4f("projection", projection_);
	texProgram_->use();
	texProgram_->setUniformMatrix4f("projection", projection_);
	texProgram_->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
}

void UIRenderer::rect(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color)
{
	simpleProgram_->use();
	simpleProgram_->setUniformMatrix4f("projection", projection_);
	simpleProgram_->setUniform4f("color", color.r, color.g, color.b, color.a);
	glm::mat4 mv = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, 0.0f));
	mv = glm::scale(mv, glm::vec3(size.x, size.y, 1.0f));
	simpleProgram_->setUniformMatrix4f("modelview", mv);
	glBindVertexArray(colorVAO_);
	glEnableVertexAttribArray(colorPosLoc_);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void UIRenderer::rectOutline(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color, float t)
{
	rect(pos, glm::vec2(size.x, t), color);
	rect(glm::vec2(pos.x, pos.y + size.y - t), glm::vec2(size.x, t), color);
	rect(glm::vec2(pos.x, pos.y + t), glm::vec2(t, size.y - 2.0f * t), color);
	rect(glm::vec2(pos.x + size.x - t, pos.y + t), glm::vec2(t, size.y - 2.0f * t), color);
}

void UIRenderer::disc(const glm::vec2 &center, float radius, const glm::vec4 &color)
{
	// Pixel-art circle built from horizontal spans (one per virtual pixel row)
	int r = int(radius);
	for(int dy = -r; dy <= r; dy++)
	{
		float half = std::floor(std::sqrt(radius * radius - float(dy * dy)) + 0.5f);
		rect(glm::vec2(std::floor(center.x) - half, std::floor(center.y) + float(dy)),
		     glm::vec2(half * 2.0f, 1.0f), color);
	}
}

void UIRenderer::fullscreen(const glm::vec4 &color)
{
	rect(glm::vec2(0.0f), glm::vec2(float(VIRTUAL_W), float(VIRTUAL_H)), color);
}

void UIRenderer::panel(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &fill, const glm::vec4 &border)
{
	glm::vec4 dark(0.02f, 0.02f, 0.06f, 1.0f);
	rect(pos + glm::vec2(3.0f, 3.0f), size, glm::vec4(0.0f, 0.0f, 0.0f, 0.45f)); // drop shadow
	rect(pos, size, dark);                                                       // outer outline
	rect(pos + glm::vec2(1.0f), size - glm::vec2(2.0f), border);                 // colored frame
	rect(pos + glm::vec2(3.0f), size - glm::vec2(6.0f), dark);                   // inner outline
	rect(pos + glm::vec2(4.0f), size - glm::vec2(8.0f), fill);                   // body
	// Top highlight bevel on the frame
	rect(pos + glm::vec2(2.0f, 1.0f), glm::vec2(size.x - 4.0f, 1.0f),
	     glm::vec4(glm::min(glm::vec3(border) + 0.35f, glm::vec3(1.0f)), 1.0f));
}

void UIRenderer::image(const Texture &tex, const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color)
{
	texProgram_->use();
	texProgram_->setUniformMatrix4f("projection", projection_);
	texProgram_->setUniform4f("color", color.r, color.g, color.b, color.a);
	glm::mat4 mv = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, 0.0f));
	mv = glm::scale(mv, glm::vec3(size.x, size.y, 1.0f));
	texProgram_->setUniformMatrix4f("modelview", mv);
	glEnable(GL_TEXTURE_2D);
	tex.use();
	glBindVertexArray(texVAO_);
	glEnableVertexAttribArray(texPosLoc_);
	glEnableVertexAttribArray(texCoordLoc_);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_TEXTURE_2D);
	texProgram_->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
}

void UIRenderer::sheet(const glm::vec2 &pos, const glm::vec2 &size, int srcX, int srcY, int srcW, int srcH,
                       const glm::vec4 &color)
{
	texProgram_->use();
	texProgram_->setUniformMatrix4f("projection", projection_);
	texProgram_->setUniform4f("color", color.r, color.g, color.b, color.a);
	if(srcW < 0)
		Bomb::drawSheetRect(*texProgram_, pos, size, srcX - srcW, srcY, srcW, srcH); // mirrored UVs
	else
		Bomb::drawSheetRect(*texProgram_, pos, size, srcX, srcY, srcW, srcH);
	texProgram_->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
}

void UIRenderer::buildHeartTexture()
{
	static const char *kHeartRows[HEART_H] = {
		".###...###.",
		"#HWR#.#RRD#",
		"#WRRR#RRRD#",
		"#RRRRRRRRD#",
		"#RRRRRRRRD#",
		".#RRRRRRD#.",
		"..#RRRRD#..",
		"...#RRD#...",
		"....#D#....",
		".....#....."
	};

	unsigned char rgba[HEART_W * HEART_H * 4];
	for(int ry = 0; ry < HEART_H; ry++)
	{
		for(int rx = 0; rx < HEART_W; rx++)
		{
			int idx = (ry * HEART_W + rx) * 4;
			unsigned char r = 0, g = 0, b = 0, a = 0;
			switch(kHeartRows[ry][rx])
			{
			case '#': r = 10;  g = 8;   b = 20;  a = 255; break; // Dark pixel-art outline
			case 'R': r = 238; g = 36;  b = 54;  a = 255; break; // Vibrant arcade red
			case 'H': r = 255; g = 112; b = 128; a = 255; break; // Soft coral highlight
			case 'W': r = 255; g = 245; b = 248; a = 255; break; // Specular white shine
			case 'D': r = 158; g = 16;  b = 38;  a = 255; break; // Crimson shadow
			default:  break;
			}
			rgba[idx + 0] = r;
			rgba[idx + 1] = g;
			rgba[idx + 2] = b;
			rgba[idx + 3] = a;
		}
	}

	if(heartTex_ == 0)
		glGenTextures(1, &heartTex_);
	glBindTexture(GL_TEXTURE_2D, heartTex_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HEART_W, HEART_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void UIRenderer::heart(const glm::vec2 &pos, float scale)
{
	if(heartTex_ == 0)
		return;

	glm::vec2 snapped = glm::floor(pos);
	texProgram_->use();
	texProgram_->setUniformMatrix4f("projection", projection_);
	texProgram_->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
	glm::mat4 mv = glm::translate(glm::mat4(1.0f), glm::vec3(snapped.x, snapped.y, 0.0f));
	mv = glm::scale(mv, glm::vec3(float(HEART_W) * scale, float(HEART_H) * scale, 1.0f));
	texProgram_->setUniformMatrix4f("modelview", mv);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, heartTex_);
	glBindVertexArray(texVAO_);
	glEnableVertexAttribArray(texPosLoc_);
	glEnableVertexAttribArray(texCoordLoc_);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisable(GL_TEXTURE_2D);
}

void UIRenderer::text(const std::string &str, const glm::vec2 &pos, float scale, const glm::vec4 &color, bool shadow)
{
	texProgram_->use();
	texProgram_->setUniformMatrix4f("projection", projection_);
	font_.drawText(*texProgram_, str, glm::floor(pos), scale, color, shadow);
	texProgram_->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
}

void UIRenderer::textCentered(const std::string &str, float centerX, float y, float scale, const glm::vec4 &color, bool shadow)
{
	texProgram_->use();
	texProgram_->setUniformMatrix4f("projection", projection_);
	font_.drawTextCentered(*texProgram_, str, glm::vec2(centerX, y), scale, color, shadow);
	texProgram_->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
}

void UIRenderer::textRight(const std::string &str, float rightX, float y, float scale, const glm::vec4 &color, bool shadow)
{
	texProgram_->use();
	texProgram_->setUniformMatrix4f("projection", projection_);
	font_.drawTextRight(*texProgram_, str, glm::vec2(rightX, y), scale, color, shadow);
	texProgram_->setUniform4f("color", 1.0f, 1.0f, 1.0f, 1.0f);
}

float UIRenderer::textWidth(const std::string &str, float scale) const
{
	return font_.measureWidth(str, scale);
}
