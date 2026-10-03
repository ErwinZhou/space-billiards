#include "BilliardsLogic.hpp"

#include <algorithm>
#include <cmath>

BilliardsLogic::BilliardsLogic() {
	reset();
}

void BilliardsLogic::reset() {
	shots = 0;
	arena_min = {32.0f, 48.0f};
	arena_max = {992.0f, 592.0f};
	black_hole_center = {768.0f, 320.0f};
	capture_radius = 11.0f;
	asteroids = {{
		{{256.0f, 320.0f}, {0.0f, 0.0f}, 25.0f, true},
		{{448.0f, 416.0f}, {0.0f, 0.0f}, 25.0f, true},
		{{448.0f, 224.0f}, {0.0f, 0.0f}, 25.0f, true}
	}};
}

bool BilliardsLogic::ready_to_shoot() const {
	bool remaining = false;
	for (auto const &asteroid : asteroids) {
		if (!asteroid.active) continue;
		remaining = true;
		if (asteroid.velocity != glm::vec2(0.0f)) return false;
	}
	return remaining;
}

int BilliardsLogic::asteroid_at(glm::vec2 position) const {
	if (position.x < arena_min.x || position.x > arena_max.x ||
		position.y < arena_min.y || position.y > arena_max.y) return -1;
	for (size_t i = 0; i < asteroids.size(); ++i) {
		auto const &asteroid = asteroids[i];
		if (asteroid.active && glm::distance(position, asteroid.position) <= asteroid.radius) return int(i);
	}
	return -1;
}

glm::vec2 BilliardsLogic::shot_velocity(glm::vec2 drag) {
	float distance = glm::length(drag);
	if (!std::isfinite(distance) || distance < min_drag) return glm::vec2(0.0f);
	return -drag * (std::min(distance, max_drag) * shot_speed_per_unit / distance);
}

bool BilliardsLogic::shoot(int index, glm::vec2 drag) {
	if (index < 0 || size_t(index) >= asteroids.size() || !asteroids[index].active || !ready_to_shoot()) return false;
	glm::vec2 velocity = shot_velocity(drag);
	if (velocity == glm::vec2(0.0f)) return false;
	asteroids[index].velocity = velocity;
	++shots;
	return true;
}
