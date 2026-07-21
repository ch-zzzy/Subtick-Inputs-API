#include <SubtickInputs.hpp>

using namespace subtickinputs;

static bool s_firstFrame = true;

bool useVanilla() {
	// clang-format off
	auto* playLayer = PlayLayer::get();
	return !playLayer
	|| !Config::get().isApiEnabled()
	|| s_firstFrame
	|| playLayer->m_playerDied
	|| playLayer->m_isPlatformer
	|| playLayer->m_useReplay;
	// clang-format on
}

// copied from cbf
#ifdef GEODE_IS_WINDOWS
#include <Geode/modify/CCEGLView.hpp>
#include <winuser.h>
class $modify(CCEGLView) {
	void pollEvents() {
		auto* playLayer = PlayLayer::get();
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
		auto* playLayer = PlayLayer::get();
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