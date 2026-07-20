// there's a couple hooks at the bottom of inputs.cpp as well

#include <SubtickInputs.hpp>
#include <algorithm>
#include <limits>

#include "SIPlayerObject.hpp"

using namespace subtickinputs;
using namespace subtickinputs::physics;
using namespace subtickinputs::fields;

static bool s_firstFrame = true;

namespace subtickinputs {
	bool useVanilla() {
		// clang-format off
		PlayLayer* playLayer = PlayLayer::get();
		return !playLayer
		|| Config::get().isApiDisabled()
		|| s_firstFrame
		|| playLayer->m_playerDied
		|| playLayer->m_isPlatformer
		|| playLayer->m_useReplay;
		// clang-format on
	}
} // namespace subtickinputs

// copied from cbf
#ifdef GEODE_IS_WINDOWS
#include <Geode/modify/CCEGLView.hpp>
#include <winuser.h>
class $modify(CCEGLView) {
	void pollEvents() {
		PlayLayer* playLayer = PlayLayer::get();
		CCNode* parent = playLayer ? playLayer->getParent() : nullptr;

		// clang-format off
		if (!GetFocus()
			|| !playLayer
			|| !parent
			|| parent->getChildByType<PauseLayer>(0)
			|| playLayer->getChildByType<EndLevelLayer>(0)
			|| playLayer->m_playerDied)
		{
			s_firstFrame = true;
		}
		// clang-format on

		CCEGLView::pollEvents();
	}
};
#else
#include <Geode/modify/CCScheduler.hpp>
class $modify(CCScheduler) {
	void update(float dt) {
		PlayLayer* playLayer = PlayLayer::get();
		CCNode* parent = playLayer ? playLayer->getParent() : nullptr;

		// clang-format off
		if (!playLayer
			|| !parent
			|| parent->getChildByType<PauseLayer>(0)
			|| playLayer->getChildByType<EndLevelLayer>(0)
			|| playLayer->m_playerDied)
		{
			s_firstFrame = true;
		}
		// clang-format on

		CCScheduler::update(dt);
	}
};
#endif

#include <Geode/modify/GJBaseGameLayer.hpp>
class $modify(GJBaseGameLayer) {
	void update(float dt) {
		if (PlayLayer::get() && PlayLayer::get()->m_playerDied) {
			s_firstFrame = true;
		}

		GJBaseGameLayer::update(dt);

		if (s_firstFrame) {
			s_firstFrame = false;
		}
	}
};

// split like cbf for wave
// doesn't cause gravity issues since wave velocity is always constant
void SIPlayerObject::update(float dt) {
	if (useVanilla()) {
		GET_PLAYER_FIELD(this, m_didWaveSplit) = false;
		PlayerObject::update(dt);
		return;
	}

	auto& pendingWaveInputs = GET_PLAYER_FIELD(this, m_pendingWaveInputs);

	if (pendingWaveInputs.empty() || !this->m_isDart || this->m_isDashing) {
		pendingWaveInputs.clear();
		GET_PLAYER_FIELD(this, m_didWaveSplit) = false;
		processPlayerTick(dt);
		return;
	}

	constexpr double SMALLEST_FLOAT = std::numeric_limits<float>::min();

	PlayLayer* playLayer = PlayLayer::get();
	double lastRatio = 0.0;

	CCPoint preTickPosition = this->getPosition();
	bool firstLoop = true;
	bool startedOnGround = this->m_isOnGround;

	for (auto& input : pendingWaveInputs) {
		double segment = std::clamp(input.m_ratio - lastRatio, SMALLEST_FLOAT, 1.0);

		PlayerObject::update(segment * dt);

		if (firstLoop && ((this->m_yVelocity < 0) ^ this->m_isUpsideDown)) {
			this->m_isOnGround = startedOnGround;
		}

		playLayer->checkCollisions(this, 0.0f, true);
		PlayerObject::updateRotation(segment * dt);
		this->resetCollisionLog(false);

		playLayer->handleButton(input.m_isPush, input.m_button, this->isPlayer1());

		lastRatio = std::max(lastRatio, input.m_ratio);
		firstLoop = false;
	}
	pendingWaveInputs.clear();

	float remainingDelta = static_cast<float>((1.0 - lastRatio) * dt);
	PlayerObject::update(remainingDelta);

	GET_PLAYER_FIELD(this, m_rotationDelta) = remainingDelta;
	GET_PLAYER_FIELD(this, m_preTickPosition) = preTickPosition;
	GET_PLAYER_FIELD(this, m_didWaveSplit) = true;
}

void SIPlayerObject::updateRotation(float dt) {
	auto& didWaveSplit = GET_PLAYER_FIELD(this, m_didWaveSplit);

	if (!didWaveSplit) {
		PlayerObject::updateRotation(dt);
		return;
	}

	didWaveSplit = false;

	PlayerObject::updateRotation(GET_PLAYER_FIELD(this, m_rotationDelta));
	this->m_lastPosition = GET_PLAYER_FIELD(this, m_preTickPosition);
}

void SIPlayerObject::setYVelocity(double velocity, int type) {
	if (Config::get().isVelocityUnroundingEnabled()) {
		this->m_yVelocity = velocity;
		return;
	}
	PlayerObject::setYVelocity(velocity, type);
}