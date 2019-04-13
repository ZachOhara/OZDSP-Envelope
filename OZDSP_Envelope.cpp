#include "OZDSP_Envelope.h"
#include "IPlug_include_in_plug_src.h"
#include "resource.h"

const int kNumPrograms = 0;

enum EParameters
{
	kAttackTimePid,
	kDecayTimePid,
	kSustainLevelPid,
	kReleaseTimePid,
	kAttackShapePid,
	kDecayShapePid,
	kReleaseShapePid,
	kNumParams
};

std::vector<ParameterInfo> kParameterInfoList = {
	ParameterInfo()
		.InitParam("Attack", kAttackTimePid, KNOB_80_ID, 10, 30)
		.InitLabel()
		.MakeEnvelopeTimeParam(),
	ParameterInfo()
		.InitParam("Decay", kDecayTimePid, KNOB_80_ID, 110, 30)
		.InitLabel()
		.MakeEnvelopeTimeParam(),
	ParameterInfo()
		.InitParam("Sustain", kSustainLevelPid, KNOB_80_ID, 210, 30)
		.InitLabel()
		.MakePercentageParam(),
	ParameterInfo()
		.InitParam("Release", kReleaseTimePid, KNOB_80_ID, 310, 30)
		.InitLabel()
		.MakeEnvelopeTimeParam(),
	ParameterInfo()
		.InitParam("Attack Shape", kAttackShapePid, KNOB_80_ID, 10, 150)
		.InitLabel()
		.MakeEnvelopeShapeParam(),
	ParameterInfo()
		.InitParam("Decay Shape", kDecayShapePid, KNOB_80_ID, 110, 150)
		.InitLabel()
		.MakeEnvelopeShapeParam(),
	ParameterInfo()
		.InitParam("Release Shape", kReleaseShapePid, KNOB_80_ID, 310, 150)
		.InitLabel()
		.MakeEnvelopeShapeParam(),
};

OZDSP_Envelope::OZDSP_Envelope(IPlugInstanceInfo instanceInfo) :
	CorePlugBase(instanceInfo, kNumParams, kNumPrograms,
		MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT),
		COMMONPLUG_CTOR_PARAMS)
{
	SetBackground(BACKGROUND_ID, BACKGROUND_FN);
	RegisterBitmap(KNOB_80_ID, KNOB_80_FN, KNOB_FRAMES);

	AddParameterList(kParameterInfoList);

	RegisterProcessorList({
		&mOscillator,
		&mEnvelopeProcessor,
		&mTuningProcessor
	});

	mEnvelopeProcessor.RegisterParameterList({
		{ kAttackTimePid, EnvelopeProcessor::kAttackTimeParam },
		{ kDecayTimePid, EnvelopeProcessor::kDecayTimeParam },
		{ kSustainLevelPid, EnvelopeProcessor::kSustainLevelParam },
		{ kReleaseTimePid, EnvelopeProcessor::kReleaseTimeParam },
		{ kAttackShapePid, EnvelopeProcessor::kAttackShapeParam },
		{ kDecayShapePid, EnvelopeProcessor::kDecayShapeParam },
		{ kReleaseShapePid, EnvelopeProcessor::kReleaseShapeParam }
	});

	mOscillator.SetWaveform(Oscillator::kTriangleWave);

	EnvelopeShapeGraphic* envelopeShape = new EnvelopeShapeGraphic(this, &mEnvelopeProcessor, IRECT(20, 240, 380, 370));
	GetGraphics()->AttachControl(envelopeShape);

	FinishConstruction();
}

OZDSP_Envelope::~OZDSP_Envelope()
{
}


void OZDSP_Envelope::ProcessMidiMsg(IMidiMsg* pMessage)
{
	mMidiReciver.RecieveMessage(pMessage);
}

void OZDSP_Envelope::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
	auto startTime = std::chrono::high_resolution_clock::now();

	// Mutex is already locked for us.
	const int nChannels = 2;

	for (int i = 0; i < nFrames; i++)
	{
		mMidiReciver.AdvanceSample(&mMidiEventQueue);

		while (!mMidiEventQueue.empty())
		{
			MidiEvent midiEvent = mMidiEventQueue.front();
			mMidiEventQueue.pop();
			HandleMidiEvent(midiEvent);
		}

		double out = mOscillator.GetNextSample();
		out = mEnvelopeProcessor.GetAdjustedSample(out);

		if (mEnvelopeProcessor.IsNoteSilent())
		{
			mOscillator.SetFrequency(0);
		}

		for (int j = 0; j < nChannels; j++)
		{
			outputs[j][i] = out;
		}
	}

	mMidiReciver.FlushBlock(nFrames);

	auto endTime = std::chrono::high_resolution_clock::now();
	long long blockTime = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
	static long long avgBlockTime;
	avgBlockTime += blockTime;
	static int blockCount;
	blockCount++;
	if (blockCount > 10)
	{
		std::cout << "Block time: " << avgBlockTime / 10 << "\n";
		std::cout.flush();
		avgBlockTime = 0;
		blockCount = 0;
	}

}

void OZDSP_Envelope::HandleMidiEvent(MidiEvent midiEvent)
{
	switch (midiEvent.eventType)
	{
	case MidiEvent::kNoteBegin:
		mOscillator.SetFrequency(mTuningProcessor.GetFrequencyOfNote(midiEvent.noteId));
		mEnvelopeProcessor.TriggerNoteAttack();
		break;
	case MidiEvent::kNoteChange:
		mOscillator.SetFrequency(mTuningProcessor.GetFrequencyOfNote(midiEvent.newNoteId));
		break;
	case MidiEvent::kNoteEnd:
		mEnvelopeProcessor.TriggerNoteRelease();
		break;
	}
}
