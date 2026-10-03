#include "BilliardsLogic.hpp"

BilliardsLogic::BilliardsLogic() {
	reset();
}

void BilliardsLogic::reset() {
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
