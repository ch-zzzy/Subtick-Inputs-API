#include <Geode/modify/PlayerObject.hpp>

#include "internal.hpp"

using namespace subtickinputs::internal;

class $modify(SIPlayerObject, PlayerObject) {
	struct Fields {
		double m_yDispAdjustment = 0.0;
		std::vector<PendingWaveInput> m_pendingWaveInputs;
	};
};

namespace subtickinputs::internal {

	double& getYDispField(PlayerObject* player) {
		return static_cast<SIPlayerObject*>(player)->m_fields->m_yDispAdjustment;
	}

	std::vector<PendingWaveInput>& getPendingWaveField(PlayerObject* player) {
		return static_cast<SIPlayerObject*>(player)->m_fields->m_pendingWaveInputs;
	}

} // namespace subtickinputs::internal