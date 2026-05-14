#include <ContinuousPhysics.hpp>

using namespace continuousphysics;

namespace continuousphysics::physics {

	bool useVanillaPhysics() {
		// clang-format off
		PlayLayer* playLayer = PlayLayer::get();
		return !playLayer 
		|| !Config::get().isModActive() 
		|| ContinuousPhysicsState::get().m_firstFrame 
		|| playLayer->m_playerDied 
		|| playLayer->m_isPlatformer 
		|| playLayer->m_useReplay;
		// clang-format on
	}

	double quantizeYVelocity(double velocity) {
		velocity = std::clamp(velocity, -1000.0, 1000.0);

		if (Config::get().isVelocityUnroundingEnabled()) {
			return velocity;
		}

		double wholePart = static_cast<double>(static_cast<int>(velocity));
		if (velocity != wholePart) {
			double frac = std::round((velocity - wholePart) * 1000.0);
			return frac / 1000.0 + wholePart;
		}

		return velocity;
	}

	float getBaseGravity(PlayerObject* player) {
		if (player->isInBasicMode()) {
			return static_cast<float>(player->m_gravity);
		} else {
			return 0.958199f;
		}
	}

	float getGravityCoefficient(PlayerObject* player) {
		bool isMini = std::abs(player->m_vehicleSize - 0.6f) < 0.01f;
		float divisor = 1.0f;

		if (isMini) {
			if (player->m_isShip || player->m_isBird || player->m_isSwing) {
				divisor = 0.85f;
			} else if (player->m_isDart) {
				divisor = 1.0f;
			} else {
				divisor = 0.8f;
			}
		}

		if (player->m_isShip) {
			float threshold = static_cast<float>(player->m_gravity) * 2.0f;
			double yVel = player->m_yVelocity;
			bool upsideDown = player->m_isUpsideDown;
			bool holding = player->m_jumpBuffered;
			bool wrongDir =
				(!upsideDown && yVel < 0) || (upsideDown && yVel > 0);
			bool inDeadzone = (yVel > -6.4f && yVel < 8.0f);
			bool belowThreshold = (yVel < threshold);

			float coeff;
			if (holding) {
				if (wrongDir)
					coeff = -0.40f;
				else if (belowThreshold)
					coeff = 0.50f;
				else
					coeff = 0.40f;
			} else {
				if (inDeadzone)
					coeff = -0.40f;
				else if (belowThreshold)
					coeff = 0.40f;
				else
					coeff = 0.48f;
			}
			return coeff / divisor;
		}

		if (player->m_isBird) {
			float threshold = static_cast<float>(player->m_gravity) * 2.0f;
			float coeff = (player->m_yVelocity < threshold) ? 0.4f : 0.6f;
			return coeff / divisor;
		}

		if (player->m_isBall) return 0.6f / divisor;
		if (player->m_isDart) return 0.0f;
		if (player->m_isRobot) return 0.9f / divisor;
		if (player->m_isSpider) return 0.6f / divisor;
		if (player->m_isSwing) {
			if (isMini) return 0.6f / divisor;
			return 0.4f / divisor;
		}

		return 1.0f / divisor;
	}

	float getGravityAcceleration(PlayerObject* player, float tps) {
		if (player->m_isDart) return 0.0f;

		float scaledDt = 60.0f / tps * 0.9f;
		float gravPerTick =
			getBaseGravity(player) * getGravityCoefficient(player) * scaledDt;
		gravPerTick = quantizeYVelocity(gravPerTick);
		return gravPerTick * tps;
	}

	float evalYPosition(PlayerObject* player, double t) {
		float yPos = player->getPositionY();
		double yVel = player->m_yVelocity;
		float tps = Config::get().getTPS();

		if (player->m_isDashing) {
			double xSpeed = player->getCurrentXVelocity();
			return yPos +
				static_cast<float>(
					player->m_dashY * std::abs(xSpeed) * t * 60.0);
		}

		if (player->m_isDart) {
			bool isMini = std::abs(player->m_vehicleSize - 1.0f) > 0.01f;
			if (isMini) yVel *= 2.0;
			return yPos + static_cast<float>(yVel * t * 60.0);
		}

		float g = getGravityAcceleration(player, tps);
		return yPos +
			static_cast<float>(
				(yVel * t - 0.5 * g * t * (t + 1.0 / tps)) * 54.0);
	}

	float evalXPosition(PlayerObject* player, double t) {
		float xPos = player->getPositionX();
		double xSpeed = player->getCurrentXVelocity();
		int dir = player->reverseMod();

		return xPos + static_cast<float>(xSpeed * dir * t * 60.0);
	}

	void advancePlayerToTimestamp(
		PlayerObject* player, double timestamp, double& lastEventTimestamp) {
		double secondsSinceLastEvent = timestamp - lastEventTimestamp;
		if (secondsSinceLastEvent <= 0.0) return;

		float newX, newY;
		if (!player->m_isSideways) {
			newX = evalXPosition(player, secondsSinceLastEvent);
			newY = evalYPosition(player, secondsSinceLastEvent);
		} else {
			newX = evalYPosition(player, secondsSinceLastEvent);
			newY = evalXPosition(player, secondsSinceLastEvent);
		}

		player->setPosition({newX, newY});
		lastEventTimestamp = timestamp;
	}

} // namespace continuousphysics::physics