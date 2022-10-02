#include "LocalTalkSimulationDataGenerator.h"
#include "LocalTalkAnalyzerSettings.h"

/* Sample data frame */
const U8 LocalTalkSimulationDataGenerator::mSimData[] = {0xFF, 0x01, 0x00, 0x4C, 0x4F, 0x43, 0x41, 0x4C, 0x54, 0x41, 0x4C, 0x4B, 0x3D, 0x3F};

LocalTalkSimulationDataGenerator::LocalTalkSimulationDataGenerator()
{
}

LocalTalkSimulationDataGenerator::~LocalTalkSimulationDataGenerator()
{
}

void LocalTalkSimulationDataGenerator::Initialize(U32 simulation_sample_rate, LocalTalkAnalyzerSettings* settings)
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	/* Set channel for simulation and sample rate */
	mLocalTalkSimData.SetChannel(mSettings->mInputChannel);
	mLocalTalkSimData.SetSampleRate(simulation_sample_rate);

	mLocalTalkSimData.SetInitialBitState(BIT_HIGH);

	/* Calculate half-period in us */
	double half_period = (1.0 / double(230400 * 2)) * 1000000.0;

	/* Convert half-period to sample count */
	mT = UsToSamples(half_period);

	/* Delay before first output */
	mLocalTalkSimData.Advance(U32(mT * 2 * 32));
}

U32 LocalTalkSimulationDataGenerator::GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate,
															 SimulationChannelDescriptor** simulation_channels)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(newest_sample_requested, sample_rate, mSimulationSampleRateHz);

	while(mLocalTalkSimData.GetCurrentSampleNumber() < adjusted_largest_sample_requested)
	{
		/* Start flags */
		SimWriteByte(0x7E, false);
		SimWriteByte(0x7E, false);
		SimWriteByte(0x7E, false);

		/* Data */
		for (U32 i = 0; i < sizeof(mSimData) / sizeof(mSimData[0]); ++i)
		{
			SimWriteByte(mSimData[i], true);
		}

		/* End flag */
		SimWriteByte(0x7E, false);

		/* Abort sequence */
		SimWriteByte(0xFF, false);
		SimWriteByte(0xFF, false);

		/* Delay before next output */
		mLocalTalkSimData.Advance(U32(mT * 2 * 32));
	}

	*simulation_channels = &mLocalTalkSimData;

	return 1;
}

U64 LocalTalkSimulationDataGenerator::UsToSamples(double us)
{
	return U64((mSimulationSampleRateHz * us) / 1000000.0);
}

void LocalTalkSimulationDataGenerator::SimWriteByte(U8 value, bool pack)
{
	U32 bits_per_xfer = 8;

	for (U32 i = 0; i < bits_per_xfer; ++i)
	{
		SimWriteBit((value >> i) & 0x1, pack);
	}
}

void LocalTalkSimulationDataGenerator::SimWriteBit(U8 bit, bool pack)
{
	BitState start_bit_state = mLocalTalkSimData.GetCurrentBitState();

	/* Perform bit packing if required */
	if (bit)
	{
		/* One, check if packing enabled */
		if (5 == mConsecutiveOnes)
		{
			/* Five consecutive ones seen, pack with a zero if enabled */
			if (pack)
			{
				mLocalTalkSimData.Transition();
				mLocalTalkSimData.Advance(U32(mT));
				mLocalTalkSimData.Transition();
				mLocalTalkSimData.Advance(U32(mT));
			}

			/* Reset count */
			mConsecutiveOnes = 0;
		}
		else
		{
			/* Inc count */
			mConsecutiveOnes++;
		}
	}
	else
	{
		/* Bit is a zero, reset consecutive one counter */
		mConsecutiveOnes = 0;
	}

	/* Output bit */
	mLocalTalkSimData.Transition();
	mLocalTalkSimData.Advance(U32(mT));
	if (bit == 0) mLocalTalkSimData.Transition();
	mLocalTalkSimData.Advance(U32(mT));
}
