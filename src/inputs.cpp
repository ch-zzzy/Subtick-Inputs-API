#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <SubtickInputs.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>

#include "SIPlayerObject.hpp"

using namespace subtickinputs;
using namespace subtickinputs::fields;

static bool s_isLastTickOfFrame = true;
static bool s_updateJumpCalledP1 = false;
static bool s_updateJumpCalledP2 = false;

static float getBaseGravity(PlayerObject* player) {
	if (player->isInBasicMode()) {
		return static_cast<float>(player->m_gravity) * player->m_gravityMod;
	} else {
		return 0.958199f * player->m_gravityMod;
	}
}

// tried my best to recreate the conditions from ghidra
// i wouldn't be surprised if something's wrong
static float getGravityCoefficient(PlayerObject* player) {
	if (player->m_isShip) {
		double yVel = player->m_yVelocity;
		bool upsideDown = player->m_isUpsideDown;
		bool holding = player->m_jumpBuffered;
		bool wrongDir = (!upsideDown && yVel < 0) || (upsideDown && yVel > 0);
		bool fbugged = player->playerIsFallingBugged();
		bool accel = player->m_isAccelerating; // this field name sucks
		// im pretty sure m_isAccelerating is just true whenever yvel
		// is NOT within (-6.4, 8.0) and false otherwise
		// it's also like one tick stale but oh well

		float coeff = 0.8f;
		if (holding) {
			if (!accel || wrongDir) coeff = -1.0f;
		} else {
			if (accel && wrongDir) coeff = -1.0f;
			if (!fbugged) coeff = 1.2f;
		}
		float randomScaleThing = (holding && fbugged) ? 0.5f : 0.4f;
		return coeff * randomScaleThing;
	}

	if (player->m_isBird) {
		return player->playerIsFallingBugged() ? 0.4f : 0.6f;
	}

	if (player->m_isBall) return 0.6f;

	if (player->m_isDart) return 0.0f;

	if (player->m_isRobot) return 0.9f;

	if (player->m_isSpider) return 0.6f;

	if (player->m_isSwing) {
		if (player->m_vehicleSize != 1.0f)
			return 0.6f;
		else
			return 0.4f;
	}

	return 1.0f;
}

static double getGravPerTick(PlayerObject* player, double scaledDt) {
	if (player->m_isDart) {
		return 0.0;
	}

	if (player->m_isOnGround) {
		return 0.0;
	}

	if (player->m_isRobot && player->m_maybeIsBoosted && player->m_jumpBuffered &&
		!player->m_touchedPad && player->m_accelerationOrSpeed < 1.5f) {
		return 0.0;
	}

	double gravPerTick = getBaseGravity(player) * getGravityCoefficient(player) * scaledDt;

	if (player->isFlying()) {
		double sizeScale = (player->m_vehicleSize != 1.0f) ? 0.85 : 1.0;
		gravPerTick /= sizeScale;
	}

	// vanilla doesn't round the gravity itself but it rounds velocity instead
	// so ideally i'd do that but i can't be bothered rn
	if (!Config::get().isVelocityUnroundingEnabled()) {
		gravPerTick = std::round(gravPerTick * 1000.0) / 1000.0;
	}

	return -1 * player->flipMod() * gravPerTick;
}

namespace subtickinputs::inputs {

	void processInputs(PlayerObject* player, float dt) {
		log::warn("update Superb Input Precision, a deprecated function has been called");

		if (player->isPlayer1()) {
			processInputs(dt);
		}
	}

	void processInputs(float dt) {
		auto* playLayer = PlayLayer::get();
		if (!playLayer) return;

		if (dt <= 0.0f) return;

		auto& inputQueue = playLayer->m_queuedButtons;

		auto& config = Config::get();

		double tps = 1.0 / dt;
		double scaledDt = 60.0 / tps * 0.9;
		double inputChecksPerTick = config.getInputHz() / tps;

		PlayerObject* p1 = playLayer->m_player1;
		PlayerObject* p2 = playLayer->m_player2;
		if (p1 && !p1->isVanillaPlayer()) {
			p1 = nullptr;
		}
		if (p2 && (!playLayer->m_gameState.m_isDualMode || !p2->isVanillaPlayer())) {
			p2 = nullptr;
		}

		double adjustedYVel1 = 0.0;
		double adjustedYVel2 = 0.0;

		double tickDuration = dt;
		double tickStartTime = playLayer->m_timestamp;
		double tickEndTime = tickStartTime + tickDuration;

		bool processEntireQueue = s_isLastTickOfFrame;

		size_t processedCount = 0;

		for (auto& input : inputQueue) {
			if (!processEntireQueue && input.m_timestamp >= tickEndTime) {
				break;
			}
			processedCount++;

			PlayerObject* player = input.m_isPlayer2 ? p2 : p1;

			double currentTime = input.m_timestamp;
			double ratio = (currentTime - tickStartTime) / tickDuration;
			ratio = std::clamp(ratio, 0.0, 1.0);

			if (!config.isInstantInputsEnabled()) {
				if (inputChecksPerTick > 1.0) {
					ratio = std::floor(ratio * inputChecksPerTick) / inputChecksPerTick;
				} else {
					ratio = 0.0;
				}
			}

			if (player && player->m_isDart && !player->m_isDashing) {
				GET_PLAYER_FIELD(player, m_pendingWaveInputs)
					.push_back({
						ratio,
						input.m_isPush,
						static_cast<int>(input.m_button),
					});
				// this is processed in PlayerObject::update in hooks.cpp
				continue;
			}

			double preVel1 = p1 ? p1->m_yVelocity : 0.0;
			double preDv1 = p1 ? getGravPerTick(p1, scaledDt) : 0.0;

			double preVel2 = p2 ? p2->m_yVelocity : 0.0;
			double preDv2 = p2 ? getGravPerTick(p2, scaledDt) : 0.0;

			s_updateJumpCalledP1 = false;
			s_updateJumpCalledP2 = false;

			playLayer->handleButton(
				input.m_isPush, static_cast<int>(input.m_button), !input.m_isPlayer2);

			if (p1) {
				// vanilla explicitly does updateJump(0) but only for certain gamemodes
				// the checks avoid double calling for those gamemodes
				if (!s_updateJumpCalledP1) p1->updateJump(0.0f);

				double postVel = p1->m_yVelocity;
				double postDv = getGravPerTick(p1, scaledDt);

				adjustedYVel1 += ratio * ((preVel1 - postVel) + (preDv1 - postDv));

				if (Config::get().isDebugModeEnabled()) {
					log::debug(
						"p1 preVel: {}, postVel: {}, preDv: {}, postDv: {}, ratio: {}, "
						"adjustedYVel: {}",
						preVel1, postVel, preDv1, postDv, ratio, adjustedYVel1);
				}
			}

			if (p2) {
				if (!s_updateJumpCalledP2) p2->updateJump(0.0f);

				double postVel = p2->m_yVelocity;
				double postDv = getGravPerTick(p2, scaledDt);

				adjustedYVel2 += ratio * ((preVel2 - postVel) + (preDv2 - postDv));

				if (Config::get().isDebugModeEnabled()) {
					log::debug(
						"p2 preVel: {}, postVel: {}, preDv: {}, postDv: {}, ratio: {}, "
						"adjustedYVel: {}",
						preVel2, postVel, preDv2, postDv, ratio, adjustedYVel2);
				}
			}
			// the handleButton + updateJump method to "dispatch" an input could be wrong
			// i'll let the players figure that one out 😛
		}

		for (PlayerObject* player : {p1, p2}) {
			if (!player) continue;
			auto& adjustedYVel = (player == p1) ? adjustedYVel1 : adjustedYVel2;

			if (player->m_isDart) {
				double waveDt = 60.0 / tps * ((player->m_vehicleSize != 1.0f) ? 2.0 : 1.0);
				GET_PLAYER_FIELD(player, m_yDispAdjustment) = adjustedYVel * waveDt;
			} else {
				GET_PLAYER_FIELD(player, m_yDispAdjustment) = adjustedYVel * scaledDt;
			}
		}

		inputQueue.erase(inputQueue.begin(), inputQueue.begin() + processedCount);
	}

} // namespace subtickinputs::inputs

class $modify(GJBaseGameLayer) {
	void processCommands(float dt, bool isHalfTick, bool isLastTick) {
		s_isLastTickOfFrame = isLastTick;
		GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
	}
};

class $modify(PlayerObject) {
	void updateJump(float dt) {
		auto* playLayer = PlayLayer::get();
		if (playLayer) {
			if (this == playLayer->m_player1)
				s_updateJumpCalledP1 = true;
			else if (this == playLayer->m_player2)
				s_updateJumpCalledP2 = true;
		}
		PlayerObject::updateJump(dt);
	}
};