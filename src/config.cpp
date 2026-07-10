#include <SubtickInputs.hpp>

static bool s_apiDisabled = false;
static bool s_debugModeEnabled = false;

namespace subtickinputs {

	Config& Config::get() {
		static Config instance;
		return instance;
	}

	bool Config::isApiDisabled() const {
		return s_apiDisabled;
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

$on_mod(Loaded) {
	auto* mod = Mod::get();

	s_apiDisabled = mod->getSettingValue<bool>("api-disabled");
	listenForSettingChanges<bool>("api-disabled", +[](bool val) { s_apiDisabled = val; });

	s_debugModeEnabled = mod->getSettingValue<bool>("debug-mode");
	listenForSettingChanges<bool>("debug-mode", +[](bool val) { s_debugModeEnabled = val; });
}