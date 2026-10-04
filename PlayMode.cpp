#include "PlayMode.hpp"
#include "ArenaLayout.hpp"

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
	// read pngs before allocating gl resources
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

bool PlayMode::handle_event(SDL_Event const &event, glm::uvec2 const &window_size) {
	if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
		if (event.key.key == SDLK_R) {
			selected = -1;
			game.reset();
			return true;
		}
		if (event.key.key == SDLK_ESCAPE) {
			selected = -1;
			return true;
		}
	}
	if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST || event.type == SDL_EVENT_WINDOW_RESIZED) {
		selected = -1;
		return false;
	}
	bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT;
	bool up = event.type == SDL_EVENT_MOUSE_BUTTON_UP && event.button.button == SDL_BUTTON_LEFT;
	bool motion = event.type == SDL_EVENT_MOUSE_MOTION;
	if (!down && !up && !motion) return false;
	if (!down && selected < 0) return false;
	glm::vec2 mouse = motion ? glm::vec2(event.motion.x, event.motion.y) : glm::vec2(event.button.x, event.button.y);
	glm::vec2 world;
	if (!ArenaLayout::fit(window_size).mouse_to_world(mouse, window_size, &world, !down)) {
		selected = -1;
		return true;
	}
	if (down) {
		selected = game.ready_to_shoot() ? game.asteroid_at(world) : -1;
		drag_start = drag_cursor = world;
	} else if (up) {
		int index = selected;
		selected = -1;
		game.shoot(index, world - drag_start);
	} else {
		drag_cursor = world;
	}
	return true;
}

void PlayMode::update(float elapsed) {
	game.advance(elapsed);
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

	glm::mat4 projection = ArenaLayout::fit(drawable_size).projection(drawable_size);

	auto const &shader = *color_texture_program;
	glUseProgram(shader.program);
	glUniformMatrix4fv(shader.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(projection));
	glBindVertexArray(sprite_vao);
	// white vertex color preserves sprite colors
	glVertexAttrib4f(shader.Color_vec4, 1.0f, 1.0f, 1.0f, 1.0f);
	glActiveTexture(GL_TEXTURE0);
	auto draw_sprite = [&](GLuint texture, glm::vec2 center) {
		constexpr float half = 32.0f;
		float x = center.x - half, y = center.y - half;
		float side = 2.0f * half;
		float vertices[] = {
			x, y, 0, 0, x + side, y, 1, 0, x + side, y + side, 1, 1,
			x, y, 0, 0, x + side, y + side, 1, 1, x, y + side, 0, 1
		};
		glBindBuffer(GL_ARRAY_BUFFER, sprite_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STREAM_DRAW);
		glBindTexture(GL_TEXTURE_2D, texture);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	};
	draw_sprite(textures[3], game.black_hole_center);
	for (size_t i = 0; i < game.asteroids.size(); ++i) {
		if (game.asteroids[i].active) draw_sprite(textures[i], game.asteroids[i].position);
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	{
		DrawLines lines(projection);
		glm::u8vec4 border(118, 169, 198, 255);
		glm::vec3 lo(game.arena_min, 0.0f), hi(game.arena_max, 0.0f);
		glm::vec3 br(hi.x, lo.y, 0.0f), tl(lo.x, hi.y, 0.0f);
		lines.draw(lo, br, border); lines.draw(br, hi, border);
		lines.draw(hi, tl, border); lines.draw(tl, lo, border);
		if (selected >= 0) {
			auto const &asteroid = game.asteroids[selected];
			glm::u8vec4 gold(255, 195, 94, 255);
			for (int i = 0; i < 32; ++i) {
				float a = float(i) * 6.2831853f / 32.0f;
				float b = float(i + 1) * 6.2831853f / 32.0f;
				float radius = asteroid.radius + 4.0f;
				lines.draw(glm::vec3(asteroid.position + radius * glm::vec2(std::cos(a), std::sin(a)), 0),
					glm::vec3(asteroid.position + radius * glm::vec2(std::cos(b), std::sin(b)), 0), gold);
			}
			glm::vec2 aim = BilliardsLogic::shot_velocity(drag_cursor - drag_start) / BilliardsLogic::shot_speed_per_unit;
			if (aim != glm::vec2(0.0f)) {
				glm::vec2 tip = asteroid.position + aim;
				glm::vec2 direction = glm::normalize(aim);
				glm::vec2 normal(-direction.y, direction.x);
				lines.draw(glm::vec3(asteroid.position, 0), glm::vec3(tip, 0), gold);
				lines.draw(glm::vec3(tip, 0), glm::vec3(tip - direction * 8.0f + normal * 4.0f, 0), gold);
				lines.draw(glm::vec3(tip, 0), glm::vec3(tip - direction * 8.0f - normal * 4.0f, 0), gold);
			}
		}
		float h = 12.0f;
		lines.draw_text("Space Billiards | Shots: " + std::to_string(game.shots), glm::vec3(32, 612, 0),
			glm::vec3(h, 0, 0), glm::vec3(0, h, 0), glm::u8vec4(255, 195, 94, 255));
		lines.draw_text("Drag back and release to shoot | Esc: cancel | R: reset", glm::vec3(32, 20, 0),
			glm::vec3(h, 0, 0), glm::vec3(0, h, 0), glm::u8vec4(200, 210, 220, 255));
	}
	GL_ERRORS();
}
