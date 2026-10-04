#pragma once

#include <glm/glm.hpp>
#include <algorithm>

// mapping for the 1024x640 world including border and hud margins
// screen coordinates use the lower-left origin and mouse input uses the top-left
struct ArenaLayout {
	float scale = 0.0f;
	glm::vec2 offset{0.0f};

	static ArenaLayout fit(glm::uvec2 size) {
		ArenaLayout layout;
		layout.scale = std::min(float(size.x) / 1024.0f, float(size.y) / 640.0f);
		layout.offset = (glm::vec2(size) - layout.scale * glm::vec2(1024, 640)) * 0.5f;
		return layout;
	}
	glm::mat4 projection(glm::uvec2 size) const {
		glm::mat4 result(1.0f);
		if (size.x == 0 || size.y == 0) return result;
		result[0][0] = 2.0f * scale / size.x;
		result[1][1] = 2.0f * scale / size.y;
		result[3][0] = 2.0f * offset.x / size.x - 1.0f;
		result[3][1] = 2.0f * offset.y / size.y - 1.0f;
		return result;
	}
	// logical window dimensions for mouse input and drawable dimensions for rendering
	bool mouse_to_world(glm::vec2 mouse, glm::uvec2 window_size, glm::vec2 *world, bool allow_outside = false) const {
		if (scale <= 0.0f) return false;
		glm::vec2 point = (glm::vec2(mouse.x, float(window_size.y) - mouse.y) - offset) / scale;
		if (!allow_outside && (point.x < 0 || point.x > 1024 || point.y < 0 || point.y > 640)) return false;
		*world = point;
		return true;
	}
};
