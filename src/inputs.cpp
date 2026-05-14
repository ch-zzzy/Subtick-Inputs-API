#include <ContinuousPhysics.hpp>

using namespace continuousphysics::physics;

namespace continuousphysics::input {

	void processInputs(PlayerObject* player, double tickEnd) {
		PlayLayer* playLayer = PlayLayer::get();
		auto& physicsState = ContinuousPhysicsState::get();
		auto& config = Config::get();
		auto* playerState = physicsState.tryGetPlayerState(player);
		if (!playLayer || !playerState) return;

		double& lastEventTimestamp = playerState->m_lastEventTimestamp;

		double inputCheckInterval = 1.0 / config.getInputHz();
		double nextInputCheck = playLayer->m_timestamp;

		auto& inputQueue = playLayer->m_queuedButtons;
		bool isPlayer1 = player->isPlayer1();
		int inputIdx = 0;

		while (nextInputCheck <= tickEnd) {
			while (inputIdx < static_cast<int>(inputQueue.size())) {
				auto& input = inputQueue[inputIdx];

				if ((!input.m_isPlayer2) != isPlayer1) {
					inputIdx++;
					continue;
				}

				if (input.m_timestamp >= nextInputCheck) break;

				advancePlayerToTimestamp(
					player, input.m_timestamp, lastEventTimestamp);

				playLayer->handleButton(input.m_isPush,
					static_cast<int>(input.m_button), isPlayer1);

				player->updateJump(0.0f);

				inputIdx++;
			}

			nextInputCheck += inputCheckInterval;
		}

		advancePlayerToTimestamp(player, tickEnd, lastEventTimestamp);
	}
} // namespace continuousphysics::input