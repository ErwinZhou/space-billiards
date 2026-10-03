#include "PlayMode.hpp"

#include "ColorTextureProgram.hpp"
#include "DrawLines.hpp"
#include "data_path.hpp"
#include "load_save_png.hpp"
#include "gl_errors.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr std::array<char const *, 4> asset_names = {
	"asteroid-a", "asteroid-b", "asteroid-c", "black-hole"
};
}

PlayMode::PlayMode() {
	// Read everything before allocating GL resources, so a missing PNG fails cleanly.
	std::array<std::vector<glm::u8vec4>, 4> pixels;
	for (size_t i = 0; i < asset_names.size(); ++i) {
		glm::uvec2 size;
		load_png(data_path(std::string(asset_names[i]) + ".png"), &size, &pixels[i], LowerLeftOrigin);
		if (size != glm::uvec2(64, 64)) {
			throw std::runtime_error(std::string(asset_names[i]) + ": expected a 64x64 PNG");
		}
	}
	glActiveTexture(GL_TEXTURE0);
	glGenTextures(GLsizei(textures.size()), textures.data());
	for (size_t i = 0; i < textures.size(); ++i) {
		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, pixels[i].data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	glGenVertexArrays(1, &sprite_vao);
	glGenBuffers(1, &sprite_vbo);
	glBindVertexArray(sprite_vao);
	glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
	auto const &shader = *color_texture_program;
	glVertexAttribPointer(shader.Position_vec4, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	glEnableVertexAttribArray(shader.Position_vec4);
	glVertexAttribPointer(shader.TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
		reinterpret_cast<void *>(2 * sizeof(float)));
	glEnableVertexAttribArray(shader.TexCoord_vec2);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	GL_ERRORS();
}

PlayMode::~PlayMode() {
	glDeleteTextures(GLsizei(textures.size()), textures.data());
	glDeleteBuffers(1, &sprite_vbo);
	glDeleteVertexArrays(1, &sprite_vao);
}

bool PlayMode::handle_event(SDL_Event const &, glm::uvec2 const &) {
	return false;
}

void PlayMode::update(float) {
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	if (drawable_size.x == 0 || drawable_size.y == 0) return;
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glClearColor(0.039f, 0.047f, 0.094f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Fit a 2x2 preview, enlarging by whole pixels whenever there is room.
	int scale = std::max(1, std::min(int(drawable_size.x) / 192, int(drawable_size.y) / 224));
	float side = 64.0f * scale;
	float gap = 16.0f * scale;
	float width = 2.0f * side + gap;
	float left = std::floor((float(drawable_size.x) - width) * 0.5f);
	float bottom = std::floor((float(drawable_size.y) - width) * 0.5f);
	glm::mat4 projection(1.0f);
	projection[0][0] = 2.0f / drawable_size.x;
	projection[1][1] = 2.0f / drawable_size.y;
	projection[3][0] = -1.0f;
	projection[3][1] = -1.0f;

	auto const &shader = *color_texture_program;
	glUseProgram(shader.program);
	glUniformMatrix4fv(shader.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(sprite_vao);
	// The shader multiplies by vertex color; use solid white for every sprite.
	glVertexAttrib4f(shader.Color_vec4, 1.0f, 1.0f, 1.0f, 1.0f);
	glActiveTexture(GL_TEXTURE0);
	for (size_t i = 0; i < textures.size(); ++i) {
		float x = left + float(i % 2) * (side + gap);
		float y = bottom + float(1 - i / 2) * (side + gap);
		float vertices[] = {
			x, y, 0, 0, x + side, y, 1, 0, x + side, y + side, 1, 1,
			x, y, 0, 0, x + side, y + side, 1, 1, x, y + side, 0, 1
		};
		glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
		glBindTexture(GL_TEXTURE_2D, textures[i]);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	{
		DrawLines lines(projection);
		float h = 12.0f;
		lines.draw_text("Space Billiards - asset preview", glm::vec3(16, drawable_size.y - 24.0f, 0),
			glm::vec3(h, 0, 0), glm::vec3(0, h, 0), glm::u8vec4(255, 195, 94, 255));
	}
	GL_ERRORS();
}
