#ifndef _UI_RENDERER_INCLUDE
#define _UI_RENDERER_INCLUDE


#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "ShaderProgram.h"
#include "Texture.h"
#include "PixelFont.h"


// Immediate-mode 2D renderer for menus and HUD.
// Everything is drawn in a fixed 320x240 virtual screen (Game Boy Color style
// resolution scaled x3 on the 960x720 window), independent of the game camera.
class UIRenderer
{

public:
	static const int VIRTUAL_W = 320;
	static const int VIRTUAL_H = 240;

	UIRenderer();
	~UIRenderer();

	void init(ShaderProgram &texProgram, ShaderProgram &simpleProgram);

	// Must be called before drawing UI each frame (sets the 320x240 projection)
	void begin();

	// Solid shapes
	void rect(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color);
	void rectOutline(const glm::vec2 &pos, const glm::vec2 &size, const glm::vec4 &color, float thickness = 1.0f);
	void disc(const glm::vec2 &center, float radius, const glm::vec4 &color);
	void fullscreen(const glm::vec4 &color);

	// Game Boy style window: dark outline, light bevel and filled body with drop shadow
	void panel(const glm::vec2 &pos, const glm::vec2 &size,
	           const glm::vec4 &fill = glm::vec4(0.07f, 0.09f, 0.20f, 0.92f),
	           const glm::vec4 &border = glm::vec4(1.0f, 0.82f, 0.25f, 1.0f));

	// Textures
	void image(const Texture &tex, const glm::vec2 &pos, const glm::vec2 &size,
	           const glm::vec4 &color = glm::vec4(1.0f));
	// Sub-rectangle of images/caracters.png. Negative srcW mirrors horizontally.
	void sheet(const glm::vec2 &pos, const glm::vec2 &size, int srcX, int srcY, int srcW, int srcH,
	           const glm::vec4 &color = glm::vec4(1.0f));

	// Text (8x8 pixel font, scale 1 = 8 virtual px tall)
	void text(const std::string &str, const glm::vec2 &pos, float scale, const glm::vec4 &color, bool shadow = true);
	void textCentered(const std::string &str, float centerX, float y, float scale, const glm::vec4 &color, bool shadow = true);
	void textRight(const std::string &str, float rightX, float y, float scale, const glm::vec4 &color, bool shadow = true);
	float textWidth(const std::string &str, float scale) const;

private:
	ShaderProgram *texProgram_;
	ShaderProgram *simpleProgram_;
	PixelFont font_;
	glm::mat4 projection_;
	GLuint colorVAO_, colorVBO_;
	GLint colorPosLoc_;
	GLuint texVAO_, texVBO_;
	GLint texPosLoc_, texCoordLoc_;
	bool initialized_;

};


#endif // _UI_RENDERER_INCLUDE
