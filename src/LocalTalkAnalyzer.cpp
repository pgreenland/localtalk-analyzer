#pragma warning(push, 0)
#include <sstream>
#include <ios>
#include <algorithm>
#pragma warning(pop)

#include "LocalTalkAnalyzer.h"
#include "LocalTalkAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

LocalTalkAnalyzer::LocalTalkAnalyzer() : mSettings(new LocalTalkAnalyzerSettings()), Analyzer2(), mSimulationInitialised(false)
{
	SetAnalyzerSettings(mSettings.get());
	UseFrameV2();
}

LocalTalkAnalyzer::~LocalTalkAnalyzer()
{
	KillThread();
}

void LocalTalkAnalyzer::SetupResults()
{
	mResults.reset(new LocalTalkAnalyzerResults(this, mSettings.get()));
	SetAnalyzerResults(mResults.get());
	mResults->AddChannelBubblesWillAppearOn(mSettings->mInputChannel);
}

void LocalTalkAnalyzer::WorkerThread()
{
	/*  Retrieve input channel and sample rate */
	mLocalTalk = GetAnalyzerChannelData(mSettings->mInputChannel);

	/* Calculate half period, in us */
	double half_period = (1.0 / double(mLocalTalkBitRate * 2)) * 1000000.0;

	/* Calculate number of samples per half-period */
	U32 mT = U32((this->GetSampleRate() * half_period) / 1000000.0);

	/* Calculate number of samples taking tolerance into account */
	U32 mTError;
	switch (mSettings->mTolerance)
	{
		case TOL25:
		{
			/* 25% tolerance, calculate error (+/- 25% = 50%)*/
			mTError = mT / 2;
			break;
		}
		case TOL5:
		{
			/* 5% tolerance, calculate error (+/- 5% = 10%) */
			mTError = mT / 10;
			break;
		}
		case TOL05:
		{
			/* 0.55% tolerance, calculate error (+/- 0.5% = 1%) */
			mTError = mT / 100;
			break;
		}
	}

	/* Reset deserializer */
	ResetDeserializer();

	/* Advance to first edge */
	mLocalTalk->AdvanceToNextEdge();

	for (;;)
	{
		/* Capture location of current edge and next edge */
		U64 curr_edge_location = mLocalTalk->GetSampleNumber();
		mLocalTalk->AdvanceToNextEdge();
		U64 next_edge_location = mLocalTalk->GetSampleNumber();

		/* Calculate samples between edges */
		U64 edge_distance = next_edge_location - curr_edge_location;

		/* Decide what sort of distance we're looking at */
		U8 bitValue;
		bool bitInSpec = false;
		if ((edge_distance > (mT - mTError)) && (edge_distance < (mT + mTError)))
		{
			/* Bit appears to be a zero, look for next transition */
			mLocalTalk->AdvanceToNextEdge();
			U64 next_next_edge_location = mLocalTalk->GetSampleNumber();

			/* Calculate samples between edges */
			edge_distance = next_next_edge_location - next_edge_location;

			/* Check next edge distance */
			bitInSpec = ((edge_distance > (mT - mTError)) && (edge_distance < (mT + mTError)));
			if (bitInSpec)
			{
				/* Bit is a zero */
				bitValue = 0;

				/* Mark first edge */
				mResults->AddMarker(curr_edge_location, AnalyzerResults::Zero, mSettings->mInputChannel);

				/* Mark second edge */
				mResults->AddMarker(next_edge_location, AnalyzerResults::X, mSettings->mInputChannel);
			}

			/* Update next edge location */
			next_edge_location = next_next_edge_location;
		}
		else if ((edge_distance > ((2 * mT) - mTError)) && (edge_distance < ((2 * mT) + mTError)))
		{
			/* Bit is a one */
			bitValue = 1;

			/* Flag bit is within spec */
			bitInSpec = true;

			/* Mark first edge */
			mResults->AddMarker(curr_edge_location, AnalyzerResults::One, mSettings->mInputChannel);
		}

		if (bitInSpec)
		{
			/* Bit is within timing limits, process it */
			DeserializeBit(bitValue, curr_edge_location, next_edge_location);
		}
		else
		{
			/* Current bit out of spec, look to end last frame */
			if (mTempPacketByteCount > 0)
			{
				/* End packet */
				ResetPacket();
			}

			/* Reset deserializer state */
			ResetDeserializer();
		}

		/* Report how far we've got through processing samples */
		ReportProgress(mLocalTalk->GetSampleNumber());

		/* Check if this glorious game should come to an end? */
		CheckIfThreadShouldExit();
	}
}

void LocalTalkAnalyzer::ResetDeserializer(void)
{
	/* Reset deserializer variables */
	mSynchronized = false;
	mConsecutiveOneCount = 0;
	mTempByte = 0;
	mTempByteBitCount = 0;
	mTempPacketByteCount = 0;
	mDataStartLocation = 0;
}

void LocalTalkAnalyzer::DeserializeBit(U8 bitValue, U64 curr_edge_location, U64 next_edge_location)
{
	const U8 CONS_ONES_BEFORE_STUFF = 5;
	const U8 FLAG_VALUE = 0x7E;

	/* Check for consecutive ones */
	if (bitValue)
	{
		/* One, increment consecutive ones */
		if (mConsecutiveOneCount < UINT8_MAX) mConsecutiveOneCount++;
	}
	else
	{
		/* Zero, snapshot and reset counter having encountered a zero */
		U8 consecutiveOnesTmp = mConsecutiveOneCount;
		mConsecutiveOneCount = 0;

		/* Check consecutive ones count */
		if (consecutiveOnesTmp < CONS_ONES_BEFORE_STUFF)
		{
			/* Just data, prevent comparisons below */
		}
		else if (CONS_ONES_BEFORE_STUFF == consecutiveOnesTmp)
		{
			/* Found stuff bit, skip if synchronized */
			if (mSynchronized)
			{
				return;
			}
		}
		else if (	 ((CONS_ONES_BEFORE_STUFF + 1) == consecutiveOnesTmp)
				  && ((FLAG_VALUE << 1) == (mTempByte & 0xfe))
				  && (	  (!mSynchronized && mTempByteBitCount >= 7)
					   || (mSynchronized && mTempByteBitCount >= 6)
					 )
				)
		{
			/*
			** We've seen 6 consecutive ones
			** Shifting in the current zero bit will make the flag value 0x7e
			** Either:
			** 	We're not in sync and have seen 7 or more bits (with this zero, we've seen the 8 required for a flag)
			** Or:
			** 	We are in sync, meaning the first zero of this flag byte was the last zero of the previous and the previous flag was a false one
			*/

			/* As a flag has just been encountered, if there was data waiting end packet */
			if (mTempPacketByteCount > 0)
			{
				/* End flag seen, checksum packet  */
				if (ChecksumOutputPacket())
				{
					/* Commit packet and start a new one */
					mResults->CommitPacketAndStartNewPacket();

					/* Reset packet buffer */
					mPacketBytes.clear();
				}
				else
				{
					/* Invalid checksum or bad packet length, skip packet */
					ResetPacket();
				}

				/* Clear synced flag */
				mSynchronized = false;

				/* Skip this bit */
				return;
			}

			/* Flag that we're now in sync */
			mSynchronized = true;

			/* Reset temporary bit counter now we're in sync */
			mTempByteBitCount = 0;

			/* Reset data byte count */
			mTempPacketByteCount = 0;

			/* Skip this bit */
			return;
		}
		else
		{
			/* More than five consecutive ones but didn't detect a flag, drop out of sync */
			mSynchronized = false;

			/* Cancel packet in progress */
			ResetPacket();
		}
	}

	/* Capture data start location */
	if (0 == mTempByteBitCount)
	{
		mDataStartLocation = curr_edge_location;
	}

	/* Shift new bit into output */
	mTempByte = (mTempByte >> 1) | (bitValue ? 0x80 : 0x00);
	if (mTempByteBitCount < 8) mTempByteBitCount++;

	if ((8 == mTempByteBitCount) && mSynchronized)
	{
		/* Commit complete byte */
		mPacketBytes.push_back(std::tuple<U64, U64, U8>(mDataStartLocation, next_edge_location, mTempByte));

		/* Increment byte counter */
		mTempPacketByteCount++;

		/* Reset bit counter */
		mTempByteBitCount = 0;
	}
}

bool LocalTalkAnalyzer::ChecksumOutputPacket(void)
{
	bool crc_valid;

	/* Reject packet if it doesn't contain at least a single data byte + checksum */
	if (mPacketBytes.size() < 3) return false;

	/* Checksum packet */
	uint16_t act_crc = 0xFFFF;
	for (int i = 0; i < mPacketBytes.size() - 2; i++)
	{
		/* Xor byte into upper byte of CRC, having reflected byte */
		act_crc ^= (uint16_t)BitReverse(std::get<2>(mPacketBytes[i])) << 8;

		for (int j = 0; j < 8; j++)
		{
			/* Update CRC bit by bit */
			if (act_crc & 0x8000)
			{
				act_crc = (act_crc << 1) ^ 0x1021;
			}
			else
			{
				act_crc = (act_crc << 1);
			}
		}

	}
	act_crc ^= 0xFFFF;

	/* Reflect CRC */
	act_crc = (uint16_t)BitReverse(act_crc >> 8) << 8 | BitReverse(act_crc);

	/* Extract expected CRC */
	uint16_t exp_crc = ((U16)std::get<2>(mPacketBytes[mPacketBytes.size() - 2]) << 8) | std::get<2>(mPacketBytes[mPacketBytes.size() - 1]);

	/* Compare checksums */
	crc_valid = (act_crc == exp_crc);

	/* Prepare byte array for frame v2 */
	U8 localPacketBytes[mPacketBytes.size() - 2];

	/* Increment ID */
	mPacketID++;

	/* Output bytes for display on timeline / export */
	for (int i = 0; i < mPacketBytes.size(); i++)
	{
		/* Break apart tuple */
		U64 location_start;
		U64 location_end;
		U8 data;
		std::tie(location_start, location_end, data) = mPacketBytes[i];

		/* Decide if data or checksum */
		bool is_data = (i < (mPacketBytes.size() - 2));

		/* Output frame */
		Frame frame;
		frame.mStartingSampleInclusive = location_start;
		frame.mEndingSampleInclusive = location_end;
		frame.mData1 = data; /* data byte */
		frame.mData2 = mPacketID; /* index of packet it goes with */
		frame.mFlags = 0;
		if (!crc_valid) frame.mFlags |= DISPLAY_AS_WARNING_FLAG;
		if (is_data) frame.mFlags |= DATA_BYTE_FLAG;
		mResults->AddFrame(frame);

		/* Add byte to array for frame v2 */
		if (is_data) localPacketBytes[i] = data;
	}

	/* Output bytes for export / table display */
	FrameV2 frame_v2;
	frame_v2.AddByteArray("data", localPacketBytes, sizeof(localPacketBytes));
	frame_v2.AddBoolean("crc_valid", crc_valid);
	mResults->AddFrameV2(frame_v2, "localtalk", std::get<0>(mPacketBytes[0]), std::get<1>(mPacketBytes[mPacketBytes.size() - 1]));
	mResults->CommitResults();

	/* All good, we output the data with bad CRC's flagged */
	return true;
}

uint8_t LocalTalkAnalyzer::BitReverse(uint8_t uiVal)
{
	static const uint8_t auiBitReverseLookup[] =
	{
		0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
		0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf
	};

	/* Reverse the top and bottom nibble then swap them */
	return (auiBitReverseLookup[uiVal & 0x0f] << 4) | auiBitReverseLookup[uiVal >> 4];
}

void LocalTalkAnalyzer::ResetPacket(void)
{
	mResults->CancelPacketAndStartNewPacket();
	mPacketBytes.clear();
}

U32 LocalTalkAnalyzer::GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate,
											  SimulationChannelDescriptor** simulation_channels)
{
	if (!mSimulationInitialised)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), mSettings.get());
		mSimulationInitialised = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(newest_sample_requested, sample_rate, simulation_channels);
}

U32 LocalTalkAnalyzer::GetMinimumSampleRateHz()
{
	return mLocalTalkBitRate * 8;
}

bool LocalTalkAnalyzer::NeedsRerun()
{
	return false;
}
const char gAnalyzerName[] = "LocalTalk"; // your analyzer must have a unique name

const char* LocalTalkAnalyzer::GetAnalyzerName() const
{
	return gAnalyzerName;
}

const char* GetAnalyzerName()
{
	return gAnalyzerName;
}

Analyzer* CreateAnalyzer()
{
	return new LocalTalkAnalyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}
