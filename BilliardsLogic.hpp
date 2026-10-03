#pragma once

#include <glm/glm.hpp>
#include <array>

struct BilliardsLogic {
	struct Asteroid {
		glm::vec2 position{0.0f};
		glm::vec2 velocity{0.0f};
		float radius = 25.0f;
		bool active = true;
	};
	glm::vec2 arena_min{32.0f, 48.0f};
	glm::vec2 arena_max{992.0f, 592.0f};
	glm::vec2 black_hole_center{768.0f, 320.0f};
	float capture_radius = 11.0f;
	std::array<Asteroid, 3> asteroids{};

	BilliardsLogic();
	void reset();
};
