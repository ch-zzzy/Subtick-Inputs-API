#include <SubtickInputs.hpp>
#include <algorithm>
#include <cmath>

#include "SIPlayerObject.hpp"

using namespace subtickinputs;
using namespace subtickinputs::fields;

static bool s_updateJumpCalledP1 = false;
static bool s_updateJumpCalledP2 = false;

static float getBaseGravity(PlayerObject* player) {
	if (player->isInBasicMode()) {
		return player->m_gravity * player->m_gravityMod;
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

static double getGravPerTick(PlayerObject* player, float tps) {
	if (player->m_isDart) {
		return 0.0;
	}

	if (player->m_isOnGround) {
		// return 0.0;
		// not too sure why i added this
		// i hope removing it doesn't backfire
	}

	if (player->m_isRobot && player->m_maybeIsBoosted && player->m_jumpBuffered &&
		!player->m_touchedPad && player->m_accelerationOrSpeed < 1.5f) {
		return 0.0;
	}

	double scaledDt = 60.0 / tps * 0.9;

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
				GET_PLAYER_FIELD(player, m_pendingWaveInputs)
					.push_back({
						ratio,
						input.m_isPush,
						static_cast<int>(input.m_button),
					});
				// this is processed in PlayerObject::update in hooks.cpp
				continue;
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

				if (Config::get().isDebugModeEnabled()) {
					log::debug(
						"p1 preVel: {}, postVel: {}, preDv: {}, postDv: {}, ratio: {}, "
						"adjustedYVel: {}",
						preVel1, postVel, preDv1, postDv, ratio, p1.adjustedYVel);
				}
			}
			if (p2.playerObj) {
				double postVel = p2.playerObj->m_yVelocity;
				double postDv = getGravPerTick(p2.playerObj, tps);

				p2.adjustedYVel += ratio * ((preVel2 - postVel) + (preDv2 - postDv));

				if (Config::get().isDebugModeEnabled()) {
					log::debug(
						"p2 preVel: {}, postVel: {}, preDv: {}, postDv: {}, ratio: {}, "
						"adjustedYVel: {}",
						preVel2, postVel, preDv2, postDv, ratio, p2.adjustedYVel);
				}
			}
		}

		if (p1.playerObj) {
			GET_PLAYER_FIELD(p1.playerObj, m_yDispAdjustment) =
				p1.adjustedYVel * (p1.isWave ? p1.waveScale : scaledDt);
		}
		if (p2.playerObj) {
			GET_PLAYER_FIELD(p2.playerObj, m_yDispAdjustment) =
				p2.adjustedYVel * (p2.isWave ? p2.waveScale : scaledDt);
		}

		inputQueue.clear();
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
};