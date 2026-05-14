#include <ContinuousPhysics.hpp>
#include <Geode/modify/CCEGLView.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace continuousphysics;
using namespace continuousphysics::physics;

class $modify(CCEGLView) {
	void pollEvents() {
		PlayLayer* playLayer = PlayLayer::get();
		CCNode* parent;

		// clang-format off
		if (!GetFocus() 
			|| !playLayer
			|| !(parent = playLayer->getParent())
			|| parent->getChildByType<PauseLayer>(0)
			|| playLayer->getChildByType<EndLevelLayer>(0)
			|| playLayer->m_playerDied)
		{
			ContinuousPhysicsState::get().m_firstFrame = true;
		}
		// clang-format on

		CCEGLView::pollEvents();
	}
};

class $modify(PlayLayer) {
	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		bool result = PlayLayer::init(level, useReplay, dontCreateObjects);
		if (!result) return false;

		if (!Config::get().isModActive()) {
			this->m_clickBetweenSteps = false;
			this->m_clickOnSteps = false;
		}

		auto& state = ContinuousPhysicsState::get();
		state.m_firstFrame = true;
		state.m_player1.m_lastEventTimestamp = this->m_timestamp;
		state.m_player2.m_lastEventTimestamp = this->m_timestamp;

		return true;
	}

	void resetLevel() {
		PlayLayer::resetLevel();

		if (Config::get().isModActive()) {
			this->m_clickBetweenSteps = false;
			this->m_clickOnSteps = false;
		}

		auto& state = ContinuousPhysicsState::get();
		state.m_firstFrame = true;
		state.m_player1.m_lastEventTimestamp = this->m_timestamp;
		state.m_player2.m_lastEventTimestamp = this->m_timestamp;
	}
};

class $modify(GJBaseGameLayer) {
	int checkCollisions(PlayerObject* object, float dt, bool ignoreDamage) {
		int result = GJBaseGameLayer::checkCollisions(object, dt, ignoreDamage);

		if (!useVanillaPhysics()) {
			auto* playerState =
				ContinuousPhysicsState::get().tryGetPlayerState(object);
			if (playerState) {
				playerState->m_lastEventTimestamp = this->m_timestamp;
			}
		}

		return result;
	}

	void update(float dt) {
		auto& state = ContinuousPhysicsState::get();

		if (PlayLayer::get() && PlayLayer::get()->m_playerDied) {
			state.m_firstFrame = true;
		}

		GJBaseGameLayer::update(dt);

		if (useVanillaPhysics()) {
			state.m_player1.m_lastEventTimestamp = this->m_timestamp;
			state.m_player2.m_lastEventTimestamp = this->m_timestamp;
			if (state.m_firstFrame) {
				state.m_firstFrame = false;
			}
			return;
		}

		PlayerObject* p1 = this->m_player1;
		PlayerObject* p2 =
			this->m_gameState.m_isDualMode ? this->m_player2 : nullptr;

		advancePlayerToTimestamp(
			p1, this->m_timestamp, state.m_player1.m_lastEventTimestamp);
		if (p2) {
			advancePlayerToTimestamp(
				p2, this->m_timestamp, state.m_player2.m_lastEventTimestamp);
		}
	}
};

class $modify(PlayerObject) {
	void setYVelocity(double velocity, int type) {
		if (Config::get().isVelocityUnroundingEnabled()) {
			this->m_yVelocity = velocity;
			return;
		}
		PlayerObject::setYVelocity(velocity, type);
	}
};