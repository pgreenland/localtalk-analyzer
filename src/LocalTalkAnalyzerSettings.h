#ifndef LOCALTALK_ANALYZER_SETTINGS
#define LOCALTALK_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

enum LocalTalkTolerance
{
    TOL25,
    TOL5,
    TOL05
};

class LocalTalkAnalyzerSettings : public AnalyzerSettings
{
	public:
		LocalTalkAnalyzerSettings();
		virtual ~LocalTalkAnalyzerSettings();

		virtual bool SetSettingsFromInterfaces();			// Get the settings out of the interfaces, validate them, and save them to your local settings vars.
		virtual void LoadSettings(const char* settings);	// Load your settings from a string.
		virtual const char* SaveSettings();					// Save your settings to a string.

		void UpdateInterfacesFromSettings();

		Channel mInputChannel;
		LocalTalkTolerance mTolerance;

	protected:
		std::unique_ptr<AnalyzerSettingInterfaceChannel> mInputChannelInterface;
		std::unique_ptr<AnalyzerSettingInterfaceNumberList> mToleranceInterface;
};

#endif // LOCALTALK_ANALYZER_SETTINGS_SETTINGS
