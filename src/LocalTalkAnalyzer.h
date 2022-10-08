#ifndef LOCALTALK_ANALYSER
#define LOCALTALK_ANALYSER

#ifdef WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#define __cdecl
#define __stdcall
#define __fastcall
#endif

#include "Analyzer.h"
#include "LocalTalkAnalyzerResults.h"
#include "LocalTalkSimulationDataGenerator.h"

/* mType bit values */
#define DATA_BYTE_FLAG ( 1 << 0 )

class LocalTalkAnalyzerSettings;

class LocalTalkAnalyzer : public Analyzer2
{
	public:
		LocalTalkAnalyzer();
		virtual ~LocalTalkAnalyzer();
		virtual void SetupResults();
		virtual void WorkerThread();
		virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
		virtual U32 GetMinimumSampleRateHz();
		virtual bool NeedsRerun();
		virtual const char* GetAnalyzerName() const;

#pragma warning(push)
#pragma warning(disable : 4251)	// warning C4251: 'LocalTalkAnalyzer::<...>' : class <...> needs to have dll-interface to be used by
								// clients of class
	protected:
	/* Localtalk bit rate */
	const U32 mLocalTalkBitRate = 230400; /* bps */

	/* Shared settings and results */
	std::unique_ptr<LocalTalkAnalyzerSettings> mSettings;
	std::unique_ptr<LocalTalkAnalyzerResults> mResults;

	/* Source channel */
	AnalyzerChannelData* mLocalTalk;

	/* De-serializer */
	bool mSynchronized;
	U8 mConsecutiveOneCount;
	U8 mTempByte;
	U8 mTempByteBitCount;
	U16 mTempPacketByteCount;
	U64 mDataStartLocation;
	void ResetDeserializer(void);
	void DeserializeBit(U8 bitValue, U64 curr_edge_location, U64 next_edge_location);

	/* Output packet and reset buffer */
	bool ChecksumOutputResetPacket(void);

	/* Output packet buffer */
	bool ChecksumOutputPacket(void);

	/* Perform bit-reversal */
	uint8_t BitReverse(uint8_t uiVal);

	/* Reset packet buffer */
	void ResetPacket(void);

	/* Tuples of start / end sample number and data byte value for each byte in packet */
	std::vector<std::tuple<U64, U64, U8>> mPacketBytes;

	/* Packet id/index */
	U64 mPacketID;

	/* Simulation state */
	bool mSimulationInitialised;
	LocalTalkSimulationDataGenerator mSimulationDataGenerator;
#pragma warning(pop)
};
extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);
#endif // LOCALTALK_ANALYSER
