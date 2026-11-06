#ifndef _PIXEL_FONT_INCLUDE
#define _PIXEL_FONT_INCLUDE


#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "ShaderProgram.h"


// Self-contained Game Boy Color / arcade 8x8 bitmap font renderer with Spanish UTF-8 support
// (Á, É, Í, Ó, Ú, Ñ, ¡, ¿) and custom symbols (★, ♥, ►, ◄), requiring zero external font libraries.
class PixelFont
{

public:
	PixelFont();
	~PixelFont();

	void init(ShaderProgram &texProgram);

	float measureWidth(const std::string &utf8Text, float scale = 1.0f) const;
	float lineHeight(float scale = 1.0f) const { return 8.0f * scale; }

	void drawText(ShaderProgram &texProgram, const std::string &utf8Text,
	              const glm::vec2 &pos, float scale, const glm::vec4 &color,
	              bool dropShadow = true) const;

	void drawTextCentered(ShaderProgram &texProgram, const std::string &utf8Text,
	                      const glm::vec2 &centerTopPos, float scale, const glm::vec4 &color,
	                      bool dropShadow = true) const;

	void drawTextRight(ShaderProgram &texProgram, const std::string &utf8Text,
	                   const glm::vec2 &rightTopPos, float scale, const glm::vec4 &color,
	                   bool dropShadow = true) const;

private:
	void decodeUtf8ToGlyphs(const std::string &utf8Text, std::vector<int> &outGlyphs) const;
	void buildFontAtlas();

	GLuint texId_;
	GLuint vao_, vbo_;
	GLint posLoc_, texLoc_;
	bool initialized_;

	static const int ATLAS_COLS = 16;
	static const int ATLAS_ROWS = 16;
	static const int CELL_SIZE  = 8;
	static const int ADVANCE_X  = 6; // 5px glyph + 1px kerning gap (compact GBC style)

};


#endif // _PIXEL_FONT_INCLUDE
