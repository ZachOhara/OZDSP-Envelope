#ifndef __OZDSP_ENVELOPE__
#define __OZDSP_ENVELOPE__

#include "IPlug_include_in_plug_hdr.h"

#include "../OZDSP_Core/CorePlugBase.h"
#include "../OZDSP_Core/audio/EnvelopeProcessor.h"
#include "../OZDSP_Core/audio/Oscillator.h"
#include "../OZDSP_Core/audio/VolumeProcessor.h"
#include "../OZDSP_Core/midi/MidiStackReciever.h"
#include "../OZDSP_Core/midi/TuningProcessor.h"
#include "../OZDSP_Core/parameter/ParameterInfo.h"

#include <chrono>
#include <iostream>

class OZDSP_Envelope : public CorePlugBase
{
public:
	OZDSP_Envelope(IPlugInstanceInfo instanceInfo);
	~OZDSP_Envelope();

	void ProcessMidiMsg(IMidiMsg* pMessage) override;
	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;

private:
	Oscillator mOscillator;
	EnvelopeProcessor mEnvelopeProcessor;
	TuningProcessor mTuningProcessor;

	MidiStackReciever mMidiReciver;

	std::queue<MidiEvent> mMidiEventQueue;

	void HandleMidiEvent(MidiEvent midiEvent);
};

#endif
