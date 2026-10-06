#pragma once

#include <glm/glm.hpp>
#include <array>

struct BilliardsLogic {
	struct Asteroid {
		glm::vec2 position{0.0f};
		glm::vec2 velocity{0.0f};
		float radius = 25.0f;
		bool active = true;
		float mass = 1.0f;
		unsigned points = 10;
	};
	glm::vec2 arena_min{32.0f, 48.0f};
	glm::vec2 arena_max{992.0f, 592.0f};
	glm::vec2 black_hole_center{848.0f, 320.0f};
	float capture_radius = 33.0f;
	static constexpr size_t target_count = 15;
	std::array<Asteroid, target_count + 1> asteroids{};

	static constexpr float max_drag = 120.0f;
	static constexpr float min_drag = 4.0f;
	static constexpr float shot_speed_per_unit = 4.0f;
	static constexpr float fixed_step = 1.0f / 60.0f;
	static constexpr unsigned shot_limit = 25;
	unsigned shots = 0;
	unsigned score = 0;

	void advance(float elapsed);
	void step();
	unsigned remaining() const;
	bool won() const;
	bool lost() const;
	bool ready_to_shoot() const;
	int asteroid_at(glm::vec2 position) const;
	static glm::vec2 shot_velocity(glm::vec2 drag);
	bool shoot(int index, glm::vec2 drag);

	BilliardsLogic();
	void reset();

private:
	double accumulated_time = 0.0;
};
