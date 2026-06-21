#include <Geode/modify/PlayerObject.hpp>
#include <SubtickInputs.hpp>

#include "internal.hpp"

using namespace subtickinputs::physics;
using namespace subtickinputs::internal;

static bool s_updateJumpCalledP1 = false;
static bool s_updateJumpCalledP2 = false;

namespace subtickinputs::inputs {

	struct PlayerState {
		PlayerObject* playerObj = nullptr;
		bool isPlayer1 = false;
		bool isWave = false;
		double waveScale = 0.0;
		double adjustedYVel = 0.0;
	};

	void processInputs(PlayerObject* player, float dt) {
		log::warn("update Superb Input Precision, a deprecated function has been called");

		if (player->isPlayer1()) {
			processInputs(dt);
		}
	}

	void processInputs(float dt) {
		PlayLayer* playLayer = PlayLayer::get();
		auto& config = Config::get();
		if (!playLayer) return;

		double tickStart = playLayer->m_timestamp;
		double tickDuration = dt;
		if (tickDuration <= 0.0) return;

		auto& inputQueue = playLayer->m_queuedButtons;

		double tps = 1.0 / dt;
		double scaledDt = 60.0 / tps * 0.9;
		double inputChecksPerTick = config.getInputHz() / tps;

		PlayerState p1{playLayer->m_player1, true};
		PlayerState p2{playLayer->m_player2, false};

		if (p1.playerObj && !p1.playerObj->isVanillaPlayer()) {
			p1.playerObj = nullptr;
		}
		if (p2.playerObj &&
			(!playLayer->m_gameState.m_isDualMode || !p2.playerObj->isVanillaPlayer())) {
			p2.playerObj = nullptr;
		}

		if (p1.playerObj) {
			p1.isWave = p1.playerObj->m_isDart;
			p1.waveScale = 60.0 / tps * ((p1.playerObj->m_vehicleSize != 1.0f) ? 2.0 : 1.0);
		}
		if (p2.playerObj) {
			p2.isWave = p2.playerObj->m_isDart;
			p2.waveScale = 60.0 / tps * ((p2.playerObj->m_vehicleSize != 1.0f) ? 2.0 : 1.0);
		}

		for (auto& input : inputQueue) {
			PlayerState& playerState = input.m_isPlayer2 ? p2 : p1;
			PlayerObject* player = playerState.playerObj;

			double rawRatio = std::clamp((input.m_timestamp - tickStart) / tickDuration, 0.0, 1.0);

			double ratio = rawRatio;
			if (!config.isInstantInputsEnabled()) {
				ratio = inputChecksPerTick <= 1.0
					? 0.0
					: std::floor(rawRatio * inputChecksPerTick) / inputChecksPerTick;
			}

			if (playerState.isWave && player && !player->m_isDashing) {
				// clang-format off
				bool ringPending =
				input.m_isPush &&
				player->m_touchingRings &&
				player->m_touchingRings->count() > 0;
				// clang-format on

				if (!ringPending) {
					getPendingWaveField(player).push_back({
						ratio,
						input.m_isPush,
						static_cast<int>(input.m_button),
					});
					continue;
				}
			}

			double preVel1 = p1.playerObj ? p1.playerObj->m_yVelocity : 0.0;
			double preDv1 = p1.playerObj ? getGravPerTick(p1.playerObj, tps) : 0.0;

			double preVel2 = p2.playerObj ? p2.playerObj->m_yVelocity : 0.0;
			double preDv2 = p2.playerObj ? getGravPerTick(p2.playerObj, tps) : 0.0;

			s_updateJumpCalledP1 = false;
			s_updateJumpCalledP2 = false;

			playLayer->handleButton(
				input.m_isPush, static_cast<int>(input.m_button), !input.m_isPlayer2);

			// vanilla explicitly does updateJump(0) but only for certain gamemodes
			// the checks avoids double calling for those gamemodes
			if (p1.playerObj && !s_updateJumpCalledP1) {
				p1.playerObj->updateJump(0.0f);
			}
			if (p2.playerObj && !s_updateJumpCalledP2) {
				p2.playerObj->updateJump(0.0f);
			}

			// the handleButton + updateJump method to "dispatch" an input could be wrong
			// i'll let the players figure that one out 😛

			if (p1.playerObj) {
				double postVel = p1.playerObj->m_yVelocity;
				double postDv = getGravPerTick(p1.playerObj, tps);

				p1.adjustedYVel += ratio * ((preVel1 - postVel) + (preDv1 - postDv));
			}
			if (p2.playerObj) {
				double postVel = p2.playerObj->m_yVelocity;
				double postDv = getGravPerTick(p2.playerObj, tps);

				p2.adjustedYVel += ratio * ((preVel2 - postVel) + (preDv2 - postDv));
			}
		}

		if (p1.playerObj) {
			getYDispField(p1.playerObj) = p1.adjustedYVel * (p1.isWave ? p1.waveScale : scaledDt);
		}
		if (p2.playerObj) {
			getYDispField(p2.playerObj) = p2.adjustedYVel * (p2.isWave ? p2.waveScale : scaledDt);
		}
	}

} // namespace subtickinputs::inputs

class $modify(PlayerObject) {
	void updateJump(float dt) {
		PlayLayer* playLayer = PlayLayer::get();
		if (playLayer) {
			if (this == playLayer->m_player1)
				s_updateJumpCalledP1 = true;
			else if (this == playLayer->m_player2)
				s_updateJumpCalledP2 = true;
		}
		PlayerObject::updateJump(dt);
	}

	// split like cbf for wave
	// doesn't cause gravity issues since wave velocity is always constant
	void update(float dt) {
		auto& pendingWaveInputs = getPendingWaveField(this);

		if (useVanillaPhysics() || pendingWaveInputs.empty() || !this->m_isDart ||
			this->m_isDashing) {
			pendingWaveInputs.clear();
			PlayerObject::update(dt);
			return;
		}

		PlayLayer* playLayer = PlayLayer::get();
		bool isPlayer1 = this->isPlayer1();
		double lastRatio = 0.0;

		CCPoint preTickPosition = this->getPosition();

		for (auto& input : pendingWaveInputs) {
			double segment = std::max(0.0, input.m_ratio - lastRatio);
			if (segment > 0.0) {
				PlayerObject::update(segment * dt);
			}
			if (playLayer) {
				playLayer->handleButton(input.m_isPush, input.m_button, isPlayer1);
			}
			lastRatio = std::max(lastRatio, input.m_ratio);
		}
		pendingWaveInputs.clear();

		PlayerObject::update((1.0 - lastRatio) * dt);

		this->m_lastPosition = preTickPosition;
	}
};