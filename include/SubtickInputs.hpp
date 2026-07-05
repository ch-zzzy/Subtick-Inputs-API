#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// clang-format off
#ifdef GEODE_IS_WINDOWS
	#ifdef SubtickInputsAPI_EXPORTING
		#define SI_API __declspec(dllexport)
	#else
		#define SI_API __declspec(dllimport)
	#endif
#else
	#define SI_API __attribute__((visibility("default")))
#endif
// clang-format on

namespace subtickinputs {
	struct InputHzChangedEvent final : public Event<InputHzChangedEvent, void(float)> {
		using Event::Event;
	};

	struct InstantInputsChangedEvent final : public Event<InstantInputsChangedEvent, void(bool)> {
		using Event::Event;
	};

	struct VelocityUnroundingChangedEvent final
		: public Event<VelocityUnroundingChangedEvent, void(bool)> {
		using Event::Event;
	};

	/// @brief whether to skip custom logic and use vanilla behavior
	/// @return true if playLayer is null, api is disabled, first frame after
	/// pause/death/init, player died, platformer mode, or robtop's replay mode thing
	bool SI_API useVanilla();

	class SI_API Config {
		public:
		static Config& get();

		bool isApiDisabled() const;

		float getInputHz() const {
			return m_inputHz;
		}
		void setInputHz(float v);

		bool isInstantInputsEnabled() const {
			return m_instantInputsEnabled;
		}
		void setInstantInputsEnabled(bool v);

		bool isVelocityUnroundingEnabled() const {
			return m_velocityUnroundingEnabled;
		}
		void setVelocityUnroundingEnabled(bool v);

		Config(const Config&) = delete;
		Config& operator=(const Config&) = delete;

		private:
		Config() = default;

		float m_inputHz = 240.0f;
		bool m_instantInputsEnabled = false;
		bool m_velocityUnroundingEnabled = false;
	};

	template <class Callback>
	ListenerHandle* listenForInputHzChanges(Callback&& cb) {
		return InputHzChangedEvent().listen(std::forward<Callback>(cb)).leak();
	}

	template <class Callback>
	ListenerHandle* listenForInstantInputsChanges(Callback&& cb) {
		return InstantInputsChangedEvent().listen(std::forward<Callback>(cb)).leak();
	}

	template <class Callback>
	ListenerHandle* listenForVelocityUnroundingChanges(Callback&& cb) {
		return VelocityUnroundingChangedEvent().listen(std::forward<Callback>(cb)).leak();
	}

	namespace physics {

		/// @brief this function (and namespace) is deprecated and will be removed in v1.0.0
		[[deprecated("use useVanilla() instead")]]
		SI_API bool useVanillaPhysics();

	} // namespace physics

	namespace inputs {

		/// @brief this function is deprecated and will be removed in v1.0.0
		[[deprecated("use processInputs(float dt) instead")]]
		SI_API void processInputs(PlayerObject* player, float dt);

		/// @brief processes this player's inputs from PlayLayer.m_queuedButtons
		/// for the current tick: dispatches each via handleButton + updateJump(0)
		/// and accumulates the sub-tick Y displacement adjustment
		/// (impulse + accel terms) into m_yDispAdjustment for SIPlayerObject::update to apply
		/// @param dt the tick duration (the dt passed to processQueuedButtons)
		SI_API void processInputs(float dt);

	} // namespace inputs

	namespace prelude {
		using namespace subtickinputs;
		using namespace subtickinputs::inputs;
		using namespace subtickinputs::physics;
	} // namespace prelude

} // namespace subtickinputs
