#pragma

namespace ade {

	class AdTimeStep {
	public:
		AdTimeStep(float timeStep) : m_timeStep(timeStep) {}
		float GetSeconds() const { return m_timeStep; }
		float GetMilliseconds() const { return m_timeStep * 1000.0f; }
	private:
		float m_timeStep = 0.0f;
	};
}