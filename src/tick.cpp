#include <ContinuousPhysics.hpp>

namespace continuousphysics::tick {

	void preTick(PlayerObject* player) {
		/*
			pre-compensate gravity: add what vanilla will subtract
			so vanilla's gravity application nets to zero
			the formula handles gravity interpolation instead
		*/
		if (!player->m_isDart && !player->m_isDashing) {
			// gravity is part of robot hold mechanic
			// let vanilla handle it without pre-compensation
			if (!(player->m_isRobot && player->m_jumpBuffered)) {
				float tps = Config::get().getTPS();
				float gravPerTick =
					physics::getGravityAcceleration(player, tps) / tps;
				int dir = player->flipMod();
				player->m_yVelocity += static_cast<double>(dir * gravPerTick);
			}
		}
	}

	void postTick(PlayerObject* player, float dt) {
		auto* playLayer = PlayLayer::get();
		if (!playLayer) return;

		double tickEnd = playLayer->m_timestamp + static_cast<double>(dt);

		auto* playerState =
			ContinuousPhysicsState::get().tryGetPlayerState(player);
		if (!playerState) return;

		physics::advancePlayerToTimestamp(
			player, tickEnd, playerState->m_lastEventTimestamp);
	}
} // namespace continuousphysics::tick