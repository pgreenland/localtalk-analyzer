#ifndef LOCALTALK_SIMULATION_DATA_GENERATOR
#define LOCALTALK_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>

class LocalTalkAnalyzerSettings;

class LocalTalkSimulationDataGenerator
{
	public:
		LocalTalkSimulationDataGenerator();
		~LocalTalkSimulationDataGenerator();

		void Initialize(U32 simulation_sample_rate, LocalTalkAnalyzerSettings* settings);
		U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);

	protected:
		U64 UsToSamples(double us);

		void SimWriteByte(U8 value, bool pack);
		void SimWriteBit(U8 bit, bool pack);

		/* Shared settings and simulation sample rate */
		LocalTalkAnalyzerSettings* mSettings;
		U32 mSimulationSampleRateHz;

		/* Demo frame to send */
		static const U8 mSimData[];

		/* Number of samples for half-period */
		U64 mT;

		/* Number of consecutive ones output */
		U8 mConsecutiveOnes;

		/* Channel description */
		SimulationChannelDescriptor mLocalTalkSimData;
};

#endif // LOCALTALK_SIMULATION_DATA_GENERATOR
