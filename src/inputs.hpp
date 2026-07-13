#include <SubtickInputs.hpp>

using namespace subtickinputs;

float getBaseGravity(PlayerObject* player) {
	if (player->isInBasicMode()) {
		return player->m_gravity * player->m_gravityMod;
	} else {
		return 0.958199f * player->m_gravityMod;
	}
}

// tried my best to recreate the conditions from ghidra
// i wouldn't be surprised if something's wrong
float getGravityCoefficient(PlayerObject* player) {
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

double getGravPerTick(PlayerObject* player, float tps) {
	if (player->m_isDart) {
		return 0.0;
	}

	if (player->m_isOnGround) {
		// contemplating removing or reworking this
		return 0.0;
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

struct PlayerState {
	PlayerObject* m_playerObj = nullptr;
	bool m_isWave = false;
	double m_waveScale = 0.0;
	double m_waveAdjustedYVel = 0.0;

	double m_currentV = 0.0;
	double m_currentA = 0.0;
	double m_totalDisp = 0.0;
	double m_lastRatio = 0.0;
	bool m_simActive = false;

	void advanceToTickRatio(double currentRatio) {
		if (!m_simActive) return;

		double ratioSlice = currentRatio - m_lastRatio;

		if (ratioSlice > 0.0) {
			m_totalDisp += ratioSlice * m_currentV + ratioSlice * m_currentA;
			m_currentV += ratioSlice * m_currentA;
			m_lastRatio = currentRatio;
		}
	}
};