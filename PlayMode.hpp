#pragma once

#include "Mode.hpp"
#include "GL.hpp"
#include "BilliardsLogic.hpp"

#include <array>

// local Space Billiards arena
struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &) override;
	virtual void update(float) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	BilliardsLogic game;
	std::array<GLuint, 4> textures{};
	GLuint sprite_vao = 0;
	GLuint sprite_vbo = 0;
};
