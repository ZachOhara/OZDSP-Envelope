#include "OZDSP_Envelope.h"
#include "IPlug_include_in_plug_src.h"
#include "resource.h"

const int kNumPrograms = 0;

enum EParams
{
	kAttackShapePid,
	kAttackTimePid,
	kDecayShapePid,
	kDecayTimePid,
	kPeakLevelPid,
	kReleaseShapePid,
	kReleaseTimePid,
	kSustainLevelPid,
	kNumParams
};

std::vector<ParameterInfo> kParameterList =
{
	ParameterInfo()
		.InitParam("Attack Shape", kAttackShapePid, ATTACK_SHAPE_CONTROL_ID, 36, 130)
		.InitLabel(15, -5)
		.MakeEnvelopeShapeParam(),
	ParameterInfo()
		.InitParam("Attack", kAttackTimePid, ATTACK_TIME_CONTROL_ID, 16, 30)
		.InitLabel(17, -5)
		.MakeEnvelopeTimeParam(),
	ParameterInfo()
		.InitParam("Decay Shape", kDecayShapePid, DECAY_SHAPE_CONTROL_ID, 102, 130)
		.InitLabel(15, -5)
		.MakeEnvelopeShapeParam(),
	ParameterInfo()
		.InitParam("Decay", kDecayTimePid, DECAY_TIME_CONTROL_ID, 92, 30)
		.InitLabel(17, -5)
		.MakeEnvelopeTimeParam(),
	ParameterInfo()
		.InitParam("Peak", kPeakLevelPid, PEAK_LEVEL_CONTROL_ID, 168, 130)
		.InitLabel(15, -5)
		.MakePercentageParam(),
	ParameterInfo()
		.InitParam("Release Shape", kReleaseShapePid, RELEASE_SHAPE_CONTROL_ID, 234, 130)
		.InitLabel(15, -5)
		.MakeEnvelopeShapeParam(),
	ParameterInfo()
		.InitParam("Release", kReleaseTimePid, RELEASE_TIME_CONTROL_ID, 244, 30)
		.InitLabel(17, -5)
		.MakeEnvelopeTimeParam(),
	ParameterInfo()
		.InitParam("Sustain", kSustainLevelPid, SUSTAIN_LEVEL_CONTROL_ID, 168, 30)
		.InitLabel(17, -5)
		.MakePercentageParam(),
};

OZDSP_Envelope::OZDSP_Envelope(IPlugInstanceInfo instanceInfo) :
	CorePlugBase(instanceInfo, kNumParams, kNumPrograms,
		MakeGraphics(this, GUI_WIDTH, GUI_HEIGHT),
		COMMONPLUG_CTOR_PARAMS)
{
	SetBackground(BACKGROUND_ID, BACKGROUND_FN);

	RegisterBitmap(ATTACK_SHAPE_CONTROL_ID, ATTACK_SHAPE_CONTROL_FN, ATTACK_SHAPE_CONTROL_FRAMES);
	RegisterBitmap(ATTACK_TIME_CONTROL_ID, ATTACK_TIME_CONTROL_FN, ATTACK_TIME_CONTROL_FRAMES);
	RegisterBitmap(DECAY_SHAPE_CONTROL_ID, DECAY_SHAPE_CONTROL_FN, DECAY_SHAPE_CONTROL_FRAMES);
	RegisterBitmap(DECAY_TIME_CONTROL_ID, DECAY_TIME_CONTROL_FN, DECAY_TIME_CONTROL_FRAMES);
	RegisterBitmap(PEAK_LEVEL_CONTROL_ID, PEAK_LEVEL_CONTROL_FN, PEAK_LEVEL_CONTROL_FRAMES);
	RegisterBitmap(RELEASE_SHAPE_CONTROL_ID, RELEASE_SHAPE_CONTROL_FN, RELEASE_SHAPE_CONTROL_FRAMES);
	RegisterBitmap(RELEASE_TIME_CONTROL_ID, RELEASE_TIME_CONTROL_FN, RELEASE_TIME_CONTROL_FRAMES);
	RegisterBitmap(SUSTAIN_LEVEL_CONTROL_ID, SUSTAIN_LEVEL_CONTROL_FN, SUSTAIN_LEVEL_CONTROL_FRAMES);

	AddParameterList(kParameterList);

	RegisterProcessor(&mEnvelopeProcessor);
	mEnvelopeProcessor.RegisterParameter(kAttackShapePid, EnvelopeProcessor::kAttackShapeParam);
	mEnvelopeProcessor.RegisterParameter(kAttackTimePid, EnvelopeProcessor::kAttackTimeParam);
	mEnvelopeProcessor.RegisterParameter(kDecayShapePid, EnvelopeProcessor::kDecayShapeParam);
	mEnvelopeProcessor.RegisterParameter(kDecayTimePid, EnvelopeProcessor::kDecayTimeParam);
	mEnvelopeProcessor.RegisterParameter(kPeakLevelPid, EnvelopeProcessor::kPeakLevelParam);
	mEnvelopeProcessor.RegisterParameter(kReleaseShapePid, EnvelopeProcessor::kReleaseShapeParam);
	mEnvelopeProcessor.RegisterParameter(kReleaseTimePid, EnvelopeProcessor::kReleaseTimeParam);
	mEnvelopeProcessor.RegisterParameter(kSustainLevelPid, EnvelopeProcessor::kSustainLevelParam);

	RegisterProcessor(&mOscillator);

	RegisterProcessor(&mTuningProcessor);

	mOscillator.SetWaveform(Oscillator::kTriangleWave);

	EnvelopeShapeGraphic* envelopeShape = new EnvelopeShapeGraphic(this, IRECT(15, 200, 305, 320),
		0xFF4CAF50, 0x2200C853); // Material green 500 (theme color) and green accent 700
	GetGraphics()->AttachControl(envelopeShape);
	mEnvelopeProcessor.AttachGraphic(envelopeShape);

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
