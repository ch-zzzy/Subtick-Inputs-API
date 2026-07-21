#include <SubtickInputs.hpp>

#include "SIPlayerObject.hpp"
#include "patches.hpp"

using namespace subtickinputs;

static PatchGroup s_velocityUnroundingNops;

// clang-format off

static void toggleVelocityUnroundingPatches(bool enable) {
	if (s_velocityUnroundingNops.appliedPatches.empty()) {
		s_velocityUnroundingNops.init({

			#ifdef GEODE_IS_WINDOWS
			{0x38C329, 0x24, 0x90}, // updateJump yvel rounding
			{0x213EA2, 0x32, 0x90}, // checkCollisions yvel rounding
			{0x38DAC7, 0x38, 0x90}, // postCollision yvel rounding
			{0x39323B, 0x40, 0x90}, // collidedWithObjectInternal yvel rounding
			{0x39FF18, 0x32, 0x90}, // boostPlayer yvel rounding
			#endif

		});
	}

	s_velocityUnroundingNops.toggle(enable);
}

namespace subtickinputs {
	void Config::setVelocityUnroundingEnabled(bool v) {
	#ifndef GEODE_IS_WINDOWS
		if (v) log::warn("Velocity unrounding is currently not supported on this platform.");
		v = false;
	#endif
		if (m_velocityUnroundingEnabled == v) return;
		m_velocityUnroundingEnabled = v;
		toggleVelocityUnroundingPatches(v);
		VelocityUnroundingChangedEvent().send(v);
	}
} // namespace subtickinputs

// clang-format on

void SIPlayerObject::setYVelocity(double velocity, int type) {
	if (Config::get().isVelocityUnroundingEnabled()) {
		this->m_yVelocity = velocity;
		return;
	}
	PlayerObject::setYVelocity(velocity, type);
}