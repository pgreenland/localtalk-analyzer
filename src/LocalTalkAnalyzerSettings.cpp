#include "LocalTalkAnalyzerSettings.h"

#include <AnalyzerHelpers.h>
#include <sstream>
#include <cstring>

#pragma warning(disable : 4800) // warning C4800: 'U32' : forcing value to bool 'true' or 'false' (performance warning)
#pragma warning(disable : 4996) // warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead.

LocalTalkAnalyzerSettings::LocalTalkAnalyzerSettings()
	: mInputChannel(UNDEFINED_CHANNEL), mTolerance(TOL25)
{
	mInputChannelInterface.reset(new AnalyzerSettingInterfaceChannel());
	mInputChannelInterface->SetTitleAndTooltip("LocalTalk", "LocalTalk");
	mInputChannelInterface->SetChannel(mInputChannel);

	mToleranceInterface.reset(new AnalyzerSettingInterfaceNumberList());
	mToleranceInterface->SetTitleAndTooltip("Tolerance", "Specify the timing tolerance as a percentage of period");
	mToleranceInterface->AddNumber(TOL25, "25% of period (default)", "Maximum allowed tolerance, +- 50% of one half period");
	mToleranceInterface->AddNumber(TOL5, "5% of period", "Required more than 10x over sampling");
	mToleranceInterface->AddNumber(TOL05, "0.5% of period", "Requires more than 200x over sampling");
	mToleranceInterface->SetNumber(mTolerance);

	AddInterface(mInputChannelInterface.get());
	AddInterface(mToleranceInterface.get());

	AddExportOption(0, "Export as text/csv file");
	AddExportExtension(0, "text", "txt");
	AddExportExtension(0, "csv", "csv");

	ClearChannels();
	AddChannel(mInputChannel, "LocalTalk", false);
}

LocalTalkAnalyzerSettings::~LocalTalkAnalyzerSettings()
{
}

bool LocalTalkAnalyzerSettings::SetSettingsFromInterfaces()
{
	mInputChannel = mInputChannelInterface->GetChannel();
	mTolerance = LocalTalkTolerance(U32(mToleranceInterface->GetNumber()));
	ClearChannels();
	AddChannel(mInputChannel, "LocalTalk", true);

	return true;
}
void LocalTalkAnalyzerSettings::LoadSettings(const char* settings)
{
	SimpleArchive text_archive;
	text_archive.SetString(settings);

	const char* name_string; // the first thing in the archive is the name of the protocol analyzer that the data belongs to.
	text_archive >> &name_string;
	if(strcmp(name_string, "LocalTalkAnalyzer") != 0)
		AnalyzerHelpers::Assert("LocalTalkAnalyzer: Provided with a settings string that doesn't belong to us;");

	text_archive >> mInputChannel;

	LocalTalkTolerance tolerance;
	if(text_archive >> *(U32*)&tolerance)
		mTolerance = tolerance;

	ClearChannels();
	AddChannel(mInputChannel, "LocalTalk", true);

	UpdateInterfacesFromSettings();
}

const char* LocalTalkAnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << "LocalTalkAnalyzer";
	text_archive << mInputChannel;
	text_archive << U32(mTolerance);

	return SetReturnString(text_archive.GetString());
}

void LocalTalkAnalyzerSettings::UpdateInterfacesFromSettings()
{
	mInputChannelInterface->SetChannel(mInputChannel);
	mToleranceInterface->SetNumber(mTolerance);
}
