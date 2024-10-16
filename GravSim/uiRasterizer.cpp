#include <vulkan/vulkan.h>

#include <array>
#include <vector>

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_BITMAP_H

#include <fstream>

#include "uiRasterizer.h"
#include "structs.h"
#include "player.h"

void UIRasterizer::initUI_A(UIInit details) {
	device = details.device;
	descriptorPool = details.descriptorPool;
	renderPass = details.renderPass;

	memProperties = details.memProperties;
	shaderCode = details.shaderCode;

	player = details.player;


	createBuffers();
	
}

void UIRasterizer::initUI_B() {
	initFreetype();
}

void UIRasterizer::createBuffers() {

}
void createPolygons() {

}

void UIRasterizer::initFreetype() {
	if (FT_Init_FreeType(&library)) { throw std::runtime_error("Failed to intialize FreeType Library"); }

	if (FT_New_Face(library, "C:/Windows/Fonts/CascadiaCode.ttf", 0, &face)) { throw std::runtime_error("Failed to load Font"); }

	FT_Set_Pixel_Sizes(face, 0, 48);


	char character = 22;

	uint32_t glyphIndex = FT_Get_Char_Index(face, 'c');

	FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER);
	FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

	std::vector<FT_GlyphSlot> glyphs;

	int16_t xMin = 10;
	int16_t xMax = 10;
	int16_t bxMax = 0;
	int16_t yMin = 10;
	int16_t yMax = 10;

	std::vector<textBitmapWrapper> textContainers;

	 int16_t charCount = CHAR_MAX;
	charCount = 120-32;
	int16_t charStart = 34;

	textContainers.reserve(charCount);

	for (char i = charStart; i < charStart + charCount; i++) {
		//std::cout << i << ": ";
		FT_Load_Char(face, i, FT_LOAD_RENDER);
		//std::cout << face->glyph->bitmap.width << ", " << face->glyph->bitmap_top << ", " << face->glyph->advance.x / 64 << std::endl;

		textBitmapWrapper tempContainer;
		tempContainer.character = i;
		tempContainer.advance = face->glyph->advance.x / 64;
		tempContainer.bearingX = face->glyph->bitmap_left;
		tempContainer.bearingY = face->glyph->bitmap_top;
		tempContainer.address = new FT_Bitmap;

		xMin = std::min(xMin, tempContainer.bearingX);
		xMax = std::max(xMax, static_cast<int16_t>(face->glyph->bitmap.width));
		bxMax = std::max(bxMax, tempContainer.bearingX);
		yMin = std::min(yMin, (int16_t)(tempContainer.bearingY - face->glyph->bitmap.rows));
		yMax = std::max(yMax, tempContainer.bearingY);


		FT_Bitmap_Init(tempContainer.address);
		FT_Bitmap_Copy(library, &(face->glyph->bitmap), tempContainer.address);

		textContainers.push_back(tempContainer);
	}
	
	int16_t charWidth = 4*ceil(((float)(xMax + bxMax - xMin))/4);
	int16_t texWidth = charCount * charWidth;
	int16_t texHeight = 4 * ceil(((float)(yMax - yMin))/4);
	int16_t orgHeight = -yMin;
	int16_t orgWidth = xMin;

	std::vector<uint8_t> texPixels;
	texPixels.resize(texWidth * texHeight);
	int16_t orgX = 0;
	int16_t orgY = 0;
	int16_t rows = 0;
	int16_t cols = 0;
	int16_t penX = 0;
	int16_t penY = 0;



	for (char i = 0; i < charCount; i++) {
		rows = textContainers[i].address->rows;
		cols = textContainers[i].address->width;
		orgX = i * charWidth + orgWidth;
		orgY = orgHeight;
		penX = orgX + textContainers[i].bearingX;
		penY = orgY + textContainers[i].bearingY - rows;


		for (int32_t x = 0; x < cols; x++) {
			for (int32_t y = 0; y < rows; y++) {
				int32_t index = (penY + y) * texWidth + penX + x;
				int16_t index2 = y * (textContainers[i].address->pitch) + x;
				auto value = textContainers[i].address->buffer[y * (textContainers[i].address->pitch) + x];
				texPixels[index] = value;
			}
			
		}
	}
	std::ofstream file;
	file.open("out.csv");

	

	for (int32_t y = 0; y < texHeight; y++) {
		for (int32_t x = 0; x < texWidth; x++) {
			file << uint32_t(texPixels[(y * texWidth) + x]) <<", ";
		}
		file << "\n";
	}

	file.close();
}