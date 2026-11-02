#include "PixelFont.h"
#include <cstring>
#include <cctype>
#include <glm/gtc/matrix_transform.hpp>


namespace
{
	enum SpecialGlyph
	{
		GLYPH_A_ACUTE = 128,
		GLYPH_E_ACUTE = 129,
		GLYPH_I_ACUTE = 130,
		GLYPH_O_ACUTE = 131,
		GLYPH_U_ACUTE = 132,
		GLYPH_N_TILDE = 133,
		GLYPH_U_UML   = 134,
		GLYPH_INV_EX  = 135,
		GLYPH_INV_QM  = 136,
		GLYPH_STAR    = 137,
		GLYPH_HEART   = 138,
		GLYPH_ARR_R   = 139,
		GLYPH_ARR_L   = 140
	};

	struct GlyphDef
	{
		int code;
		unsigned char rows[8]; // 5-bit pattern in bits 4..0 (0x10 = left, 0x01 = right)
	};

	static const GlyphDef kGlyphTable[] = {
		// Space & Punctuation (baseline at row 7, standard cap top at row 1)
		{ ' ', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
		{ '!', { 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 } },
		{ '"', { 0x00, 0x0A, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00 } },
		{ '#', { 0x00, 0x0A, 0x1F, 0x0A, 0x0A, 0x1F, 0x0A, 0x00 } },
		{ '%', { 0x00, 0x19, 0x1A, 0x04, 0x04, 0x0B, 0x13, 0x00 } },
		{ '&', { 0x00, 0x0C, 0x12, 0x14, 0x08, 0x15, 0x12, 0x0D } },
		{ '\'',{ 0x00, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 } },
		{ '(', { 0x00, 0x02, 0x04, 0x08, 0x08, 0x08, 0x04, 0x02 } },
		{ ')', { 0x00, 0x08, 0x04, 0x02, 0x02, 0x02, 0x04, 0x08 } },
		{ '*', { 0x00, 0x00, 0x04, 0x15, 0x0E, 0x15, 0x04, 0x00 } },
		{ '+', { 0x00, 0x00, 0x04, 0x04, 0x1F, 0x04, 0x04, 0x00 } },
		{ ',', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08 } },
		{ '-', { 0x00, 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 } },
		{ '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04 } },
		{ '/', { 0x00, 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10 } },

		// Digits 0..9
		{ '0', { 0x00, 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
		{ '1', { 0x00, 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
		{ '2', { 0x00, 0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F } },
		{ '3', { 0x00, 0x1F, 0x02, 0x04, 0x06, 0x01, 0x11, 0x0E } },
		{ '4', { 0x00, 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
		{ '5', { 0x00, 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
		{ '6', { 0x00, 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
		{ '7', { 0x00, 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
		{ '8', { 0x00, 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
		{ '9', { 0x00, 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },

		{ ':', { 0x00, 0x00, 0x04, 0x04, 0x00, 0x04, 0x04, 0x00 } },
		{ ';', { 0x00, 0x00, 0x04, 0x00, 0x00, 0x04, 0x04, 0x08 } },
		{ '<', { 0x00, 0x02, 0x04, 0x08, 0x10, 0x08, 0x04, 0x02 } },
		{ '=', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x00 } },
		{ '>', { 0x00, 0x08, 0x04, 0x02, 0x01, 0x02, 0x04, 0x08 } },
		{ '?', { 0x00, 0x0E, 0x11, 0x01, 0x06, 0x04, 0x00, 0x04 } },
		{ '[', { 0x00, 0x0E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x0E } },
		{ ']', { 0x00, 0x0E, 0x02, 0x02, 0x02, 0x02, 0x02, 0x0E } },
		{ '_', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F } },

		// Uppercase A..Z
		{ 'A', { 0x00, 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
		{ 'B', { 0x00, 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
		{ 'C', { 0x00, 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
		{ 'D', { 0x00, 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
		{ 'E', { 0x00, 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
		{ 'F', { 0x00, 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
		{ 'G', { 0x00, 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
		{ 'H', { 0x00, 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
		{ 'I', { 0x00, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
		{ 'J', { 0x00, 0x07, 0x02, 0x02, 0x02, 0x12, 0x12, 0x0C } },
		{ 'K', { 0x00, 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 } },
		{ 'L', { 0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
		{ 'M', { 0x00, 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
		{ 'N', { 0x00, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
		{ 'O', { 0x00, 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
		{ 'P', { 0x00, 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
		{ 'Q', { 0x00, 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D } },
		{ 'R', { 0x00, 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
		{ 'S', { 0x00, 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
		{ 'T', { 0x00, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
		{ 'U', { 0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
		{ 'V', { 0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
		{ 'W', { 0x00, 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11 } },
		{ 'X', { 0x00, 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11 } },
		{ 'Y', { 0x00, 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } },
		{ 'Z', { 0x00, 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F } },

		// Spanish accented glyphs & custom UI icons
		{ GLYPH_A_ACUTE, { 0x02, 0x04, 0x0E, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
		{ GLYPH_E_ACUTE, { 0x02, 0x04, 0x1F, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
		{ GLYPH_I_ACUTE, { 0x02, 0x04, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x0E } },
		{ GLYPH_O_ACUTE, { 0x02, 0x04, 0x0E, 0x11, 0x11, 0x11, 0x11, 0x0E } },
		{ GLYPH_U_ACUTE, { 0x02, 0x04, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
		{ GLYPH_N_TILDE, { 0x0E, 0x00, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11 } },
		{ GLYPH_U_UML,   { 0x0A, 0x00, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
		{ GLYPH_INV_EX,  { 0x00, 0x04, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04 } },
		{ GLYPH_INV_QM,  { 0x00, 0x04, 0x00, 0x04, 0x0C, 0x10, 0x11, 0x0E } },
		{ GLYPH_STAR,    { 0x00, 0x04, 0x0E, 0x1F, 0x0E, 0x0E, 0x1B, 0x11 } },
		{ GLYPH_HEART,   { 0x00, 0x0A, 0x1F, 0x1F, 0x1F, 0x0E, 0x04, 0x00 } },
		{ GLYPH_ARR_R,   { 0x00, 0x10, 0x18, 0x1C, 0x1E, 0x1C, 0x18, 0x10 } },
		{ GLYPH_ARR_L,   { 0x00, 0x01, 0x03, 0x07, 0x0F, 0x07, 0x03, 0x01 } }
	};
}


PixelFont::PixelFont()
{
	texId_ = 0;
	vao_ = 0;
	vbo_ = 0;
	posLoc_ = -1;
	texLoc_ = -1;
	initialized_ = false;
}

PixelFont::~PixelFont()
{
	if(texId_ != 0)
		glDeleteTextures(1, &texId_);
	if(vao_ != 0)
		glDeleteVertexArrays(1, &vao_);
	if(vbo_ != 0)
		glDeleteBuffers(1, &vbo_);
}

void PixelFont::init(ShaderProgram &texProgram)
{
	if(initialized_)
		return;

	buildFontAtlas();

	glGenVertexArrays(1, &vao_);
	glBindVertexArray(vao_);
	glGenBuffers(1, &vbo_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, 24 * 128 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	posLoc_ = texProgram.bindVertexAttribute("position", 2, 4 * sizeof(float), 0);
	texLoc_ = texProgram.bindVertexAttribute("texCoord", 2, 4 * sizeof(float), (void *)(2 * sizeof(float)));

	initialized_ = true;
}

void PixelFont::buildFontAtlas()
{
	const int atlasW = ATLAS_COLS * CELL_SIZE; // 128
	const int atlasH = ATLAS_ROWS * CELL_SIZE; // 128
	std::vector<unsigned char> rgba(atlasW * atlasH * 4, 0);

	size_t numGlyphs = sizeof(kGlyphTable) / sizeof(kGlyphTable[0]);
	for(size_t g = 0; g < numGlyphs; g++)
	{
		int code = kGlyphTable[g].code;
		if(code < 0 || code >= ATLAS_COLS * ATLAS_ROWS)
			continue;

		int cellX = (code % ATLAS_COLS) * CELL_SIZE;
		int cellY = (code / ATLAS_COLS) * CELL_SIZE;

		for(int ry = 0; ry < CELL_SIZE; ry++)
		{
			unsigned char rowBits = kGlyphTable[g].rows[ry];
			for(int rx = 0; rx < 5; rx++)
			{
				if((rowBits & (0x10 >> rx)) != 0)
				{
					int px = cellX + rx;
					int py = cellY + ry;
					int idx = (py * atlasW + px) * 4;
					rgba[idx + 0] = 255;
					rgba[idx + 1] = 255;
					rgba[idx + 2] = 255;
					rgba[idx + 3] = 255;
				}
			}
		}
	}

	if(texId_ == 0)
		glGenTextures(1, &texId_);
	glBindTexture(GL_TEXTURE_2D, texId_);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasW, atlasH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void PixelFont::decodeUtf8ToGlyphs(const std::string &utf8Text, std::vector<int> &outGlyphs) const
{
	outGlyphs.clear();
	outGlyphs.reserve(utf8Text.size());

	for(size_t i = 0; i < utf8Text.size(); i++)
	{
		unsigned char c = static_cast<unsigned char>(utf8Text[i]);
		if(c < 0x80)
		{
			// Classic arcade font renders lowercase cleanly as uppercase caps
			if(c >= 'a' && c <= 'z')
				outGlyphs.push_back(int(c - 'a' + 'A'));
			else
				outGlyphs.push_back(int(c));
		}
		else if(c == 0xC2 && i + 1 < utf8Text.size())
		{
			unsigned char c2 = static_cast<unsigned char>(utf8Text[++i]);
			if(c2 == 0xA1)      outGlyphs.push_back(GLYPH_INV_EX); // ¡
			else if(c2 == 0xBF) outGlyphs.push_back(GLYPH_INV_QM); // ¿
			else                outGlyphs.push_back(' ');
		}
		else if(c == 0xC3 && i + 1 < utf8Text.size())
		{
			unsigned char c2 = static_cast<unsigned char>(utf8Text[++i]);
			switch(c2)
			{
			case 0x81: case 0xA1: outGlyphs.push_back(GLYPH_A_ACUTE); break; // Á, á
			case 0x89: case 0xA9: outGlyphs.push_back(GLYPH_E_ACUTE); break; // É, é
			case 0x8D: case 0xAD: outGlyphs.push_back(GLYPH_I_ACUTE); break; // Í, í
			case 0x93: case 0xB3: outGlyphs.push_back(GLYPH_O_ACUTE); break; // Ó, ó
			case 0x9A: case 0xBA: outGlyphs.push_back(GLYPH_U_ACUTE); break; // Ú, ú
			case 0x91: case 0xB1: outGlyphs.push_back(GLYPH_N_TILDE); break; // Ñ, ñ
			case 0x9C: case 0xBC: outGlyphs.push_back(GLYPH_U_UML);   break; // Ü, ü
			default:              outGlyphs.push_back('?');           break;
			}
		}
		else if(c == 0xE2 && i + 2 < utf8Text.size())
		{
			unsigned char c2 = static_cast<unsigned char>(utf8Text[i + 1]);
			unsigned char c3 = static_cast<unsigned char>(utf8Text[i + 2]);
			i += 2;
			if(c2 == 0x98 && c3 == 0x85)      outGlyphs.push_back(GLYPH_STAR);  // ★ (U+2605)
			else if(c2 == 0x99 && c3 == 0xA5) outGlyphs.push_back(GLYPH_HEART); // ♥ (U+2665)
			else if(c2 == 0x96 && c3 == 0xBA) outGlyphs.push_back(GLYPH_ARR_R); // ► (U+25BA)
			else if(c2 == 0x97 && c3 == 0x84) outGlyphs.push_back(GLYPH_ARR_L); // ◄ (U+25C4)
			else                              outGlyphs.push_back('*');
		}
	}
}

float PixelFont::measureWidth(const std::string &utf8Text, float scale) const
{
	std::vector<int> glyphs;
	decodeUtf8ToGlyphs(utf8Text, glyphs);
	if(glyphs.empty())
		return 0.0f;
	return (float(glyphs.size() * ADVANCE_X) - 1.0f) * scale;
}

void PixelFont::drawText(ShaderProgram &texProgram, const std::string &utf8Text,
                         const glm::vec2 &pos, float scale, const glm::vec4 &color,
                         bool dropShadow) const
{
	if(!initialized_ || utf8Text.empty())
		return;

	std::vector<int> glyphs;
	decodeUtf8ToGlyphs(utf8Text, glyphs);
	if(glyphs.empty())
		return;

	const float atlasW = float(ATLAS_COLS * CELL_SIZE);
	const float atlasH = float(ATLAS_ROWS * CELL_SIZE);
	const float cellW  = float(CELL_SIZE) * scale;
	const float cellH  = float(CELL_SIZE) * scale;
	const float stepX  = float(ADVANCE_X) * scale;

	static std::vector<float> verts;
	verts.clear();
	verts.reserve(glyphs.size() * 24);

	float curX = 0.0f;
	for(size_t i = 0; i < glyphs.size(); i++)
	{
		int g = glyphs[i];
		if(g != ' ' && g >= 0 && g < ATLAS_COLS * ATLAS_ROWS)
		{
			float u0 = float((g % ATLAS_COLS) * CELL_SIZE) / atlasW;
			float v0 = float((g / ATLAS_COLS) * CELL_SIZE) / atlasH;
			float u1 = u0 + float(CELL_SIZE) / atlasW;
			float v1 = v0 + float(CELL_SIZE) / atlasH;

			float x0 = curX;
			float y0 = 0.0f;
			float x1 = curX + cellW;
			float y1 = cellH;

			float quad[24] = {
				x0, y0, u0, v0,
				x1, y0, u1, v0,
				x1, y1, u1, v1,
				x0, y0, u0, v0,
				x1, y1, u1, v1,
				x0, y1, u0, v1
			};
			verts.insert(verts.end(), quad, quad + 24);
		}
		curX += stepX;
	}

	if(verts.empty())
		return;

	texProgram.use();
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texId_);
	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(posLoc_);
	glEnableVertexAttribArray(texLoc_);

	int vertexCount = int(verts.size() / 4);

	if(dropShadow)
	{
		float shOffset = std::max(1.0f, scale);
		glm::mat4 shadowMv = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x + shOffset, pos.y + shOffset, 0.0f));
		texProgram.setUniformMatrix4f("modelview", shadowMv);
		texProgram.setUniform4f("color", 0.05f, 0.07f, 0.14f, color.a);
		glDrawArrays(GL_TRIANGLES, 0, vertexCount);
	}

	glm::mat4 mv = glm::translate(glm::mat4(1.0f), glm::vec3(pos.x, pos.y, 0.0f));
	texProgram.setUniformMatrix4f("modelview", mv);
	texProgram.setUniform4f("color", color.r, color.g, color.b, color.a);
	glDrawArrays(GL_TRIANGLES, 0, vertexCount);

	glDisable(GL_TEXTURE_2D);
}

void PixelFont::drawTextCentered(ShaderProgram &texProgram, const std::string &utf8Text,
                                 const glm::vec2 &centerTopPos, float scale, const glm::vec4 &color,
                                 bool dropShadow) const
{
	float w = measureWidth(utf8Text, scale);
	glm::vec2 startPos(std::floor(centerTopPos.x - w * 0.5f), std::floor(centerTopPos.y));
	drawText(texProgram, utf8Text, startPos, scale, color, dropShadow);
}

void PixelFont::drawTextRight(ShaderProgram &texProgram, const std::string &utf8Text,
                              const glm::vec2 &rightTopPos, float scale, const glm::vec4 &color,
                              bool dropShadow) const
{
	float w = measureWidth(utf8Text, scale);
	glm::vec2 startPos(std::floor(rightTopPos.x - w), std::floor(rightTopPos.y));
	drawText(texProgram, utf8Text, startPos, scale, color, dropShadow);
}
