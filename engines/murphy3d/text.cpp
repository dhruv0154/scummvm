#include "murphy3d/text.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/math_utils.h"

namespace Murphy3d {

Text::Text(Font *font) : _font(font), _vbo(0), _lines(0), _width(0.0f) {
	_colour1 = 0xFFFFFFFF;
	_colour2 = 0xFFFFFFFF;
	_colour3 = 0xFFFFFFFF;
	_colour4 = 0xFFFFFFFF;
}

Text::~Text() {
	if (_vbo && g_engine->_renderer) {
		g_engine->_renderer->deleteBuffer(_vbo);
		_vbo = 0;
	}
}

void Text::setColours(uint32 c1, uint32 c2, uint32 c3, uint32 c4) {
	_colour1 = c1;
	_colour2 = c2;
	_colour3 = c3;
	_colour4 = c4;
}

void Text::extractRGB(uint32 color, float &r, float &g, float &b) {
	r = ((color >> 16) & 0xFF) / 255.0f;
	g = ((color >> 8) & 0xFF) / 255.0f;
	b = (color & 0xFF) / 255.0f;
}

void Text::setText(const Common::String &text, const Rect &rect, TextAlignment alignment, float scale) {
	_vertices.clear();
	if (text.empty() || !_font)
		return;

	Common::Array<Word> words;
	int start = 0;
	float pixels = 0.0f;
	uint32 currentCol = _colour1;

	for (uint i = 0; i <= text.size(); i++) {
		char c = (i == text.size()) ? 0 : text[i];

		if (c == 0 || c == ' ' || c == '<' || c == '~' || c == '>' || c == '@' || c == '\n') {
			int len = i - start;
			if (len > 0) {
				Word w;
				w.color = currentCol;
				w.pixels = pixels;
				w.text = text.substr(start, len);
				words.push_back(w);

				start = i + 1;
				pixels = 0.0f;

				if (c == '<')
					currentCol = _colour2;
				else if (c == '~')
					currentCol = _colour3;
				else if (c == '>' || c == '@')
					currentCol = _colour1;
				else if (c == '\n') {
					Word nl;
					nl.text = "\n";
					nl.pixels = 0.0f;
					nl.color = currentCol;
					words.push_back(nl);
				}
			}
		} else if (c == '^' && i + 1 < text.size()) {
			char cix = text[++i];
			if (cix == 0x2d)
				currentCol = _colour1;
			else if (cix == 0x2e)
				currentCol = _colour2;
			else if (cix == 0x3f)
				currentCol = _colour3;
			start = i + 1;
		} else if ((unsigned char)c > 0x20 && (unsigned char)c <= 0x7f) {
			pixels += _font->getWidthArray((unsigned char)c - 0x20) * scale;
		}
	}

	if (words.empty())
		return;

	float maxw = rect.right - rect.left;
	float spaceWidth = _font->getWidthArray(0) * scale;
	float fh = _font->getHeight() * scale;
	float fcw = 1.0f / 224.0f;
	float y1 = _font->getY1();
	float y2 = _font->getY2();

	_lines = 0;
	float sx = rect.left;
	float sy = 0.0f;

	uint wordIndex = 0;
	while (wordIndex < words.size()) {
		float linePixels = 0.0f;
		int wordsInLine = 0;
		uint lineStartIndex = wordIndex;

		while (wordIndex < words.size() && words[wordIndex].text != "\n") {
			float nextWidth = words[wordIndex].pixels + (wordsInLine > 0 ? spaceWidth : 0.0);
			if (wordsInLine > 0 && linePixels + nextWidth > maxw)
				break;
			linePixels += nextWidth;
			wordsInLine++;
			wordIndex++;
		}

		if (wordIndex < words.size() && words[wordIndex].text == "\n")
			wordIndex++;
		_lines++;
		float pixelsLeft = maxw - linePixels;

		sx = rect.left;
		float justifyAdjust = 0.0f;

		if (alignment == kAlignCenter) {
			sx += pixelsLeft / 2.0f;
		} else if (alignment == kAlignRight) {
			sx += pixelsLeft;
		} else if (alignment == kAlignJustify && wordsInLine > 1 && linePixels > 0) {
			justifyAdjust = pixelsLeft / (wordsInLine - 1);
		}

		for (int i = 0; i < wordsInLine; i++) {
			Word &w = words[lineStartIndex + i];
			float r, g, b;
			extractRGB(w.color, r, g, b);
			sx = floor(sx);

			for (uint c = 0; c < w.text.size(); c++) {
				char ch = w.text[c];
				if (ch >= 0x20 && (unsigned char)ch <= 0x7f) {
					ch -= 0x20;
					float fx = _font->getWidthArray(ch);
					float u1 = fcw * (float)ch;
					float u2 = u1 + fx / 3584.0f;
					float scaledFx = fx * scale;

					MULTICOLOURED_FONT_VERTEX v[6] = {
						{sx, sy, -1.5f, u1, y1, r, g, b, 1.0f},
						{sx + scaledFx, sy, -1.5f, u2, y1, r, g, b, 1.0f},
						{sx + scaledFx, sy - fh, -1.5f, u2, y2, r, g, b, 1.0f},

						{sx, sy, -1.5f, u1, y1, r, g, b, 1.0f},
						{sx + scaledFx, sy - fh, -1.5f, u2, y2, r, g, b, 1.0f},
						{sx, sy - fh, -1.5f, u1, y2, r, g, b, 1.0f}};

					for (int k = 0; k < 6; k++)
						_vertices.push_back(v[k]);
					sx += scaledFx;
				}
			}
			sx += spaceWidth + justifyAdjust;
		}
		sy -= fh;
	}

	if (_vertices.size() > 0 && g_engine && g_engine->_renderer) {
		if (_vbo == 0) {
			_vbo = g_engine->_renderer->createVertexBuffer(_vertices.data(), _vertices.size() * sizeof(MULTICOLOURED_FONT_VERTEX), true);
		} else {
			g_engine->_renderer->updateBufferData(_vbo, _vertices.data(), _vertices.size() * sizeof(MULTICOLOURED_FONT_VERTEX), 0);
		}
	}
}

void Text::render(Renderer *renderer, float x, float y, float z) {
	if (_vertices.empty() || !_vbo || !_font->getTexture())
		return;

	Math::Matrix4 worldMat = MathUtils::translation(floor(x), -floor(y), z);
	renderer->drawUI(_vbo, _vertices.size(), _font->getTexture()->getId(), worldMat);
}

}; // End of namespace Murphy3d
