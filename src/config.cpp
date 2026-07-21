#include <Geode/modify/PlayLayer.hpp>
#include <SubtickInputs.hpp>
#include <string>

static bool s_apiEnabled = false;
// cbs AND cos are both force disabled when this is true, i just kept the name concise
static bool s_cbsForceDisabled = true;
static bool s_debugModeEnabled = false;

namespace subtickinputs {
	Config& Config::get() {
		static Config instance;
		return instance;
	}

	bool Config::isApiEnabled() const {
		return s_apiEnabled && s_cbsForceDisabled;
	}

	bool Config::isApiDisabled() const {
		return !isApiEnabled();
	}

	bool Config::isDebugModeEnabled() const {
		return s_debugModeEnabled;
	}

	void Config::setInputHz(float v) {
		if (m_inputHz == v) return;
		m_inputHz = v;
		InputHzChangedEvent().send(v);
	}

	void Config::setInstantInputsEnabled(bool v) {
		if (m_instantInputsEnabled == v) return;
		m_instantInputsEnabled = v;
		InstantInputsChangedEvent().send(v);
	}

} // namespace subtickinputs

template <typename T>
static void localSettingInit(T& staticVar, const std::string& settingKey, auto listenerCallback) {
	staticVar = Mod::get()->getSettingValue<T>(settingKey);
	listenForSettingChanges<T>(settingKey, listenerCallback);
}

static void disableApiIfCbsUnforced() {
	if (!s_cbsForceDisabled) {
		Mod::get()->setSettingValue<bool>("api-enabled", false);
	}
}

$on_mod(Loaded) {
	localSettingInit(s_apiEnabled, "api-enabled", [](bool val) { s_apiEnabled = val; });

	localSettingInit(s_debugModeEnabled, "debug-mode", [](bool val) { s_debugModeEnabled = val; });

	localSettingInit(s_cbsForceDisabled, "cbs-disabled", [](bool val) {
		s_cbsForceDisabled = val;
		disableApiIfCbsUnforced();

		auto* playLayer = PlayLayer::get();
		if (playLayer) {
			if (!s_cbsForceDisabled) {
				auto* gameManager = GameManager::get();
				playLayer->m_clickBetweenSteps = gameManager->getGameVariable("0177");
				playLayer->m_clickOnSteps = gameManager->getGameVariable("0176");
			} else {
				playLayer->m_clickBetweenSteps = false;
				playLayer->m_clickOnSteps = false;
			}
		}
	});

	disableApiIfCbsUnforced();
}

class $modify(PlayLayer) {
	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
		if (s_cbsForceDisabled) {
			this->m_clickBetweenSteps = false;
			this->m_clickOnSteps = false;
		}
		return true;
	}

	void resetLevel() {
		PlayLayer::resetLevel();
		if (s_cbsForceDisabled) {
			this->m_clickBetweenSteps = false;
			this->m_clickOnSteps = false;
		}
	}
};