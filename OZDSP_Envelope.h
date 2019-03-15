#ifndef __OZDSP_ENVELOPE__
#define __OZDSP_ENVELOPE__

#include "IPlug_include_in_plug_hdr.h"

#include "../OZDSP_Common/CommonPlugBase.h"
#include "../OZDSP_Common/midi/MidiStackReciever.h"
#include "../OZDSP_Common/midi/TuningProcessor.h"
#include "../OZDSP_Common/parameter/ParameterInfo.h"
#include "../OZDSP_Common/processing/EnvelopeProcessor.h"
#include "../OZDSP_Common/processing/Oscillator.h"
#include "../OZDSP_Common/processing/VolumeProcessor.h"

#include <chrono>
#include <iostream>

class OZDSP_Envelope : public CommonPlugBase
{
public:
	OZDSP_Envelope(IPlugInstanceInfo instanceInfo);
	~OZDSP_Envelope();

	void ProcessMidiMsg(IMidiMsg* pMessage) override;
	void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) override;

protected:
	ParameterInfoList BuildParameterInfoList() override;
	ProcessorRegistry BuildProcessorRegistry() override;

private:
	Oscillator mOscillator;
	EnvelopeProcessor mEnvelopeProcessor;

	MidiStackReciever mMidiReciver;
	TuningProcessor mTuningProcessor;

	std::queue<MidiEvent> mMidiEventQueue;

	void HandleMidiEvent(MidiEvent midiEvent);
};

#endif
