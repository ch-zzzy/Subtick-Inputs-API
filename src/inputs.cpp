#include "inputs.hpp"

#include <SubtickInputs.hpp>
#include <algorithm>
#include <cmath>

#include "SIPlayerObject.hpp"

using namespace subtickinputs;
using namespace subtickinputs::fields;

static bool s_updateJumpCalledP1 = false;
static bool s_updateJumpCalledP2 = false;

namespace subtickinputs::inputs {

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

		double tickStartTimestamp = playLayer->m_timestamp;
		double tickDuration = dt;
		if (tickDuration <= 0.0) return;

		auto& inputQueue = playLayer->m_queuedButtons;

		double tps = 1.0 / dt;
		double scaledDt = 60.0 / tps * 0.9;
		double inputChecksPerTick = config.getInputHz() / tps;

		PlayerState p1{playLayer->m_player1};
		PlayerState p2{playLayer->m_player2};

		if (p1.m_playerObj && !p1.m_playerObj->isVanillaPlayer()) {
			p1.m_playerObj = nullptr;
		}
		if (p2.m_playerObj &&
			(!playLayer->m_gameState.m_isDualMode || !p2.m_playerObj->isVanillaPlayer())) {
			p2.m_playerObj = nullptr;
		}

		if (p1.m_playerObj) {
			p1.m_isWave = p1.m_playerObj->m_isDart;
			p1.m_waveScale = 60.0 / tps * ((p1.m_playerObj->m_vehicleSize != 1.0f) ? 2.0 : 1.0);
		}
		if (p2.m_playerObj) {
			p2.m_isWave = p2.m_playerObj->m_isDart;
			p2.m_waveScale = 60.0 / tps * ((p2.m_playerObj->m_vehicleSize != 1.0f) ? 2.0 : 1.0);
		}

		for (auto& input : inputQueue) {
			PlayerState& playerState = input.m_isPlayer2 ? p2 : p1;
			PlayerObject* player = playerState.m_playerObj;

			double rawRatio =
				std::clamp((input.m_timestamp - tickStartTimestamp) / tickDuration, 0.0, 1.0);

			double ratio = rawRatio;
			if (!config.isInstantInputsEnabled()) {
				if (inputChecksPerTick <= 1.0) {
					ratio = 0.0;
				} else {
					ratio = std::floor(rawRatio * inputChecksPerTick) / inputChecksPerTick;
				}
			}

			if (playerState.m_isWave && player && !player->m_isDashing) {
				auto pendingInput = subtickinputs::fields::PendingWaveInput{
					ratio,
					input.m_isPush,
					static_cast<int>(input.m_button),
				};

				GET_PLAYER_FIELD(player, m_pendingWaveInputs).push_back(pendingInput);
				// this is processed in PlayerObject::update in hooks.cpp
				continue;
			}

			if (!p1.m_simActive && p1.m_playerObj && !p1.m_isWave) {
				p1.m_currentV = p1.m_playerObj->m_yVelocity;
				p1.m_currentA = getGravPerTick(p1.m_playerObj, tps);
				p1.m_totalDisp = 0.0;
				p1.m_lastRatio = 0.0;
				p1.m_simActive = true;
			}
			if (!p2.m_simActive && p2.m_playerObj && !p2.m_isWave) {
				p2.m_currentV = p2.m_playerObj->m_yVelocity;
				p2.m_currentA = getGravPerTick(p2.m_playerObj, tps);
				p2.m_totalDisp = 0.0;
				p2.m_lastRatio = 0.0;
				p2.m_simActive = true;
			}

			p1.advanceToTickRatio(ratio);
			p2.advanceToTickRatio(ratio);

			s_updateJumpCalledP1 = false;
			s_updateJumpCalledP2 = false;

			playLayer->handleButton(
				input.m_isPush, static_cast<int>(input.m_button), !input.m_isPlayer2);

			// vanilla explicitly does updateJump(0) but only for certain gamemodes
			// the checks avoids double calling for those gamemodes
			if (p1.m_playerObj && !s_updateJumpCalledP1) {
				p1.m_playerObj->updateJump(0.0f);
			}
			if (p2.m_playerObj && !s_updateJumpCalledP2) {
				p2.m_playerObj->updateJump(0.0f);
			}

			if (p1.m_simActive) {
				p1.m_currentV = p1.m_playerObj->m_yVelocity;
				p1.m_currentA = getGravPerTick(p1.m_playerObj, tps);
			}
			if (p2.m_simActive) {
				p2.m_currentV = p2.m_playerObj->m_yVelocity;
				p2.m_currentA = getGravPerTick(p2.m_playerObj, tps);
			}

			if (Config::get().isDebugModeEnabled()) {
				if (p1.m_simActive) {
					log::debug("p1 ratio: {}, currentV: {}, currentA: {}, totalDisp: {}", ratio,
						p1.m_currentV, p1.m_currentA, p1.m_totalDisp);
				}
				if (p2.m_simActive) {
					log::debug("p2 ratio: {}, currentV: {}, currentA: {}, totalDisp: {}", ratio,
						p2.m_currentV, p2.m_currentA, p2.m_totalDisp);
				}
			}
		}

		for (PlayerState* ps : {&p1, &p2}) {
			if (!ps->m_playerObj) continue;

			if (ps->m_isWave) {
				GET_PLAYER_FIELD(ps->m_playerObj, m_yDispAdjustment) =
					ps->m_waveAdjustedYVel * ps->m_waveScale;
				continue;
			}

			if (!ps->m_simActive) {
				GET_PLAYER_FIELD(ps->m_playerObj, m_yDispAdjustment) = 0.0;
				continue;
			}

			double lastInputR = ps->m_lastRatio;
			ps->advanceToTickRatio(1.0);

			double vanillaStep = ps->m_currentV + lastInputR * ps->m_currentA;

			GET_PLAYER_FIELD(ps->m_playerObj, m_yDispAdjustment) =
				(ps->m_totalDisp - vanillaStep) * scaledDt;
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