#pragma once

#include <Geode/Geode.hpp>

using namespace geode::prelude;

// clang-format off
#ifdef GEODE_IS_WINDOWS
	#ifdef ContinuousPhysicsAPI_EXPORTING
		#define CP_API __declspec(dllexport)
	#else
		#define CP_API __declspec(dllimport)
	#endif
#else
	#define CP_API __attribute__((visibility("default")))
#endif
// clang-format on

namespace continuousphysics {

	class CP_API ContinuousPhysicsState {
		public:
		struct PlayerState {
			double m_lastEventTimestamp = 0.0;
		};

		static ContinuousPhysicsState& get();

		/// @brief tries to get the corresponding PlayerState for a given PlayerObject
		/// @return pointer to &g_physicsState.m_player1 or &g_physicsState.m_player2, or nullptr if player is null
		PlayerState* tryGetPlayerState(PlayerObject* player) {
			if (!player) return nullptr;
			auto& physicsState = ContinuousPhysicsState::get();
			return player->isPlayer1() ? &physicsState.m_player1
									   : &physicsState.m_player2;
		}

		bool m_firstFrame = true;
		PlayerState m_player1;
		PlayerState m_player2;

		ContinuousPhysicsState(const ContinuousPhysicsState&) = delete;
		ContinuousPhysicsState& operator=(
			const ContinuousPhysicsState&) = delete;

		private:
		ContinuousPhysicsState() = default;
	};

	class CP_API Config {
		public:
		static Config& get();

		float getTPS() const {
			return m_tps;
		}
		void setTPS(float v) {
			m_tps = v;
		}

		float getInputHz() const {
			return m_inputHz;
		}
		void setInputHz(float v) {
			m_inputHz = v;
		}

		bool isModActive() const {
			return m_modActive;
		}
		void setModActive(bool v) {
			m_modActive = v;
		}

		bool isVelocityUnroundingEnabled() const {
			return m_velocityUnroundingEnabled;
		}
		void setVelocityUnroundingEnabled(bool v) {
			m_velocityUnroundingEnabled = v;
		}

		Config(const Config&) = delete;
		Config& operator=(const Config&) = delete;

		private:
		Config() = default;

		float m_tps = 240.0f;
		float m_inputHz = 240.0f;
		bool m_modActive = false;
		bool m_velocityUnroundingEnabled = false;
	};

	namespace physics {
		/// @brief whether to skip continuous physics logic and use vanilla behavior
		/// @return true if playLayer is null, mod is disabled, first frame after
		/// pause/death/init, player died, platformer mode, or robtop's replay mode
		CP_API bool useVanillaPhysics();

		/// @brief updates the player's position to where it should be at the given timestamp
		/// based on the time since the last event
		/// @param timestamp the new timestamp to advance the player to
		/// @param lastEventTimestamp the timestamp of the last "event" (input check, tick, collision, frame, etc.)
		CP_API void advancePlayerToTimestamp(
			PlayerObject* player, double timestamp, double& lastEventTimestamp);

		/// @brief calculates the gravity acceleration a player would have at a given tps
		/// @return the gravity acceleration in yvels/sec where 1 yvel is 54 units/sec
		/// and 1 unit is 1/30th of a block
		CP_API float getGravityAcceleration(PlayerObject* player, float tps);

	} // namespace physics

	namespace input {

		/// @brief processes all inputs from PlayLayer.m_queuedButtons for a player at a tick,
		/// calling advancePlayerToTimestamp and handleInput for each input event
		CP_API void processInputs(PlayerObject* player, double tickEnd);

	} // namespace input

	namespace tick {

		/// @brief called before PlayerObject::update(dt) to pre-compensate gravity
		CP_API void preTick(PlayerObject* player);

		/// @brief called after PlayerObject::update(dt) to update physics state
		CP_API void postTick(PlayerObject* player, float dt);

	} // namespace tick

	namespace patches {

		/// @brief sets whether to apply nop patches to remove velocity rounding in PlayerObject::update
		CP_API void toggleVelocityUnroundingPatches(bool enable);

	} // namespace patches

	namespace prelude {
		using namespace continuousphysics;
		using namespace continuousphysics::input;
		using namespace continuousphysics::patches;
		using namespace continuousphysics::physics;
		using namespace continuousphysics::tick;
	} // namespace prelude

} // namespace continuousphysics