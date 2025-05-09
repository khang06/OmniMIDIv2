/*

	OmniMIDI v15+ (Rewrite) for Win32/Linux

	This file contains the required code to run the driver under both Windows and Linux

*/

// Always include first
#include "Common.hpp"

#ifndef _OFLUIDSYNTH_H
#define _OFLUIDSYNTH_H

#include "SynthModule.hpp"

#include "fluidsynth.h"

#define FLUIDSYNTH_STR	"FluidSynth"

#ifdef _WIN32
#define FLUIDLIB		"libfluidsynth-3"
#define FLUIDSFX		nullptr
#define AUDIODRV		"wasapi"
#else
#define FLUIDLIB		"fluidsynth"
#define FLUIDSFX		".3"
#define AUDIODRV		"alsa"
#endif

namespace OmniMIDI {
	class FluidSettings : public SettingsModule {
	public:
		// Global settings
		uint64_t EvBufSize = 32768;
		uint32_t PeriodSize = 64;
		uint32_t Periods = 2;
		uint32_t ThreadsCount = 1;
		uint32_t MinimumNoteLength = 10;
		bool ExperimentalMultiThreaded = false;
		double OverflowVolume = 10000.0;
		double OverflowPercussion = 10000.0;
		double OverflowReleased = -10000.0;
		double OverflowImportant = 0.0;
		std::string Driver = AUDIODRV;
		std::string SampleFormat = "float";

		FluidSettings(ErrorSystem::Logger* PErr) : SettingsModule(PErr) { }

		void RewriteSynthConfig() {
			nlohmann::json DefConfig = {
					ConfGetVal(SampleRate),
					ConfGetVal(EvBufSize),
					ConfGetVal(VoiceLimit),
					ConfGetVal(PeriodSize),
					ConfGetVal(Periods),
					ConfGetVal(ThreadsCount),
					ConfGetVal(MinimumNoteLength),
					ConfGetVal(OverflowVolume),
					ConfGetVal(OverflowPercussion),
					ConfGetVal(OverflowReleased),
					ConfGetVal(OverflowImportant),
					ConfGetVal(Driver),
					ConfGetVal(SampleFormat),
					ConfGetVal(ExperimentalMultiThreaded)
			};

			if (AppendToConfig(DefConfig))
				WriteConfig();

			CloseConfig();
			InitConfig(false, FLUIDSYNTH_STR, sizeof(FLUIDSYNTH_STR));
		}

		// Here you can load your own JSON, it will be tied to ChangeSetting()
		void LoadSynthConfig() {
			if (InitConfig(false, FLUIDSYNTH_STR)) {
				SynthSetVal(uint32_t, SampleRate);
				SynthSetVal(uint32_t, EvBufSize);
				SynthSetVal(uint32_t, VoiceLimit);
				SynthSetVal(uint32_t, PeriodSize);
				SynthSetVal(uint32_t, Periods);
				SynthSetVal(uint32_t, ThreadsCount);
				SynthSetVal(uint32_t, MinimumNoteLength);
				SynthSetVal(bool, ExperimentalMultiThreaded);
				SynthSetVal(double, OverflowVolume);
				SynthSetVal(double, OverflowPercussion);
				SynthSetVal(double, OverflowReleased);
				SynthSetVal(double, OverflowImportant);
				SynthSetVal(std::string, Driver);
				SynthSetVal(std::string, SampleFormat);
				return;
			}

			if (IsConfigOpen() && !IsSynthConfigValid()) {
				RewriteSynthConfig();
			}
		}
	};

	class FluidSynth : public SynthModule {
	private:
		Lib* FluiLib = nullptr;

		LibImport fLibImp[25] = {
			// BASS
			ImpFunc(new_fluid_synth),
			ImpFunc(new_fluid_settings),
			ImpFunc(delete_fluid_synth),
			ImpFunc(delete_fluid_settings),
			ImpFunc(fluid_synth_all_notes_off),
			ImpFunc(fluid_synth_all_sounds_off),
			ImpFunc(fluid_synth_noteoff),
			ImpFunc(fluid_synth_noteon),
			ImpFunc(fluid_synth_cc),
			ImpFunc(fluid_synth_channel_pressure),
			ImpFunc(fluid_synth_key_pressure),
			ImpFunc(fluid_synth_pitch_bend),
			ImpFunc(fluid_synth_pitch_wheel_sens),
			ImpFunc(fluid_synth_program_change),
			ImpFunc(fluid_synth_program_reset),
			ImpFunc(fluid_synth_program_select),
			ImpFunc(fluid_synth_sysex),
			ImpFunc(fluid_synth_system_reset),
			ImpFunc(fluid_synth_sfunload),
			ImpFunc(fluid_synth_sfload),
			ImpFunc(fluid_settings_setint),
			ImpFunc(fluid_settings_setnum),
			ImpFunc(fluid_settings_setstr),
			ImpFunc(new_fluid_audio_driver),
			ImpFunc(delete_fluid_audio_driver)
		};
		size_t fLibImpLen = sizeof(fLibImp) / sizeof(fLibImp[0]);

		FluidSettings* _fluidConfig = nullptr;
		SoundFontSystem _sfSystem;
		fluid_settings_t* fSet = nullptr;

		std::vector<int> SoundFontIDs;
		fluid_synth_t** AudioStreams = new fluid_synth_t*[16] {0};
		fluid_audio_driver_t** AudioDrivers = new fluid_audio_driver_t*[16] {0};
		size_t AudioStreamSize = 16;

		std::vector<int> SoundFonts;

		void EventsThread();
		bool ProcessEvBuf();

	public:
		FluidSynth(ErrorSystem::Logger* PErr) : SynthModule(PErr) {}
		bool LoadSynthModule() override;
		bool UnloadSynthModule() override;
		bool StartSynthModule() override;
		bool StopSynthModule() override;
		void LoadSoundFonts() override;
		bool SettingsManager(uint32_t setting, bool get, void* var, size_t size) override { return false; }
		uint32_t GetSampleRate() override  { return _fluidConfig->SampleRate; }
		bool IsSynthInitialized() override { return (AudioDrivers[0] != nullptr); }
		uint32_t SynthID() override { return 0x6F704EC6; }

		uint32_t PlayLongEvent(uint8_t* ev, uint32_t size) override;
		uint32_t UPlayLongEvent(uint8_t* ev, uint32_t size) override;

		// Not supported in FluidSynth
		SynthResult TalkToSynthDirectly(uint32_t evt, uint32_t chan, uint32_t param) override { return Ok; }
	};
}

#endif