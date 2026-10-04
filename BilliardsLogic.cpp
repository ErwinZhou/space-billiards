#include "BilliardsLogic.hpp"

#include <algorithm>
#include <cmath>

BilliardsLogic::BilliardsLogic() {
	reset();
}

void BilliardsLogic::reset() {
	shots = 0;
	accumulated_time = 0.0;
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

unsigned BilliardsLogic::remaining() const {
	unsigned count = 0;
	for (auto const &asteroid : asteroids) if (asteroid.active) ++count;
	return count;
}

bool BilliardsLogic::won() const {
	return remaining() == 0;
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

void BilliardsLogic::advance(float elapsed) {
	if (won() || !std::isfinite(elapsed) || elapsed <= 0.0f) return;
	accumulated_time += std::min(elapsed, 0.25f);
	while (accumulated_time >= double(fixed_step)) {
		step();
		accumulated_time -= double(fixed_step);
	}
}

void BilliardsLogic::step() {
	if (won()) return;
	constexpr float restitution = 0.95f;
	auto capture = [&](Asteroid &a, glm::vec2 start) {
		glm::vec2 segment = a.position - start;
		float length_squared = glm::dot(segment, segment);
		float t = length_squared > 0.0f ? std::clamp(glm::dot(black_hole_center - start, segment) / length_squared, 0.0f, 1.0f) : 0.0f;
		glm::vec2 nearest = start + t * segment - black_hole_center;
		if (glm::dot(nearest, nearest) <= capture_radius * capture_radius) {
			a.active = false;
			a.velocity = glm::vec2(0.0f);
		}
	};
	auto contain = [&](Asteroid &a) {
		for (int axis = 0; axis < 2; ++axis) {
			float low = arena_min[axis] + a.radius;
			float high = arena_max[axis] - a.radius;
			if (a.position[axis] < low) {
				a.position[axis] = low;
				if (a.velocity[axis] < 0) a.velocity[axis] *= -restitution;
			} else if (a.position[axis] > high) {
				a.position[axis] = high;
				if (a.velocity[axis] > 0) a.velocity[axis] *= -restitution;
			}
		}
	};
	// small substeps reduce missed grazing contacts at maximum shot speed
	for (int substep = 0; substep < 4; ++substep) {
		for (auto &a : asteroids) {
			if (!a.active) continue;
			glm::vec2 start = a.position;
			a.position += a.velocity * (fixed_step / 4.0f);
			capture(a, start);
			if (a.active) contain(a);
		}
		// repeated passes resolve contacts pushed into walls or other asteroids
		for (int pass = 0; pass < 4; ++pass) {
			for (size_t i = 0; i < asteroids.size(); ++i) {
				auto &a = asteroids[i];
				if (!a.active) continue;
				for (size_t j = i + 1; j < asteroids.size(); ++j) {
					auto &b = asteroids[j];
					if (!b.active) continue;
					glm::vec2 delta = b.position - a.position;
					float distance = glm::length(delta);
					float radius = a.radius + b.radius;
					if (distance > radius) continue;
					glm::vec2 normal = distance > 0.0001f ? delta / distance : glm::vec2(1.0f, 0.0f);
					glm::vec2 correction = normal * (0.5f * (radius - distance));
					a.position -= correction;
					b.position += correction;
					float closing = glm::dot(b.velocity - a.velocity, normal);
					if (closing < 0.0f) {
						glm::vec2 impulse = normal * (-0.5f * (1.0f + restitution) * closing);
						a.velocity -= impulse;
						b.velocity += impulse;
					}
				}
			}
			for (auto &a : asteroids) {
				if (!a.active) continue;
				contain(a);
				capture(a, a.position);
			}
		}
	}
	float damping = std::exp(-0.7f * fixed_step);
	for (auto &a : asteroids) {
		if (!a.active) continue;
		a.velocity *= damping;
		if (glm::length(a.velocity) < 3.0f) a.velocity = glm::vec2(0.0f);
	}
}
