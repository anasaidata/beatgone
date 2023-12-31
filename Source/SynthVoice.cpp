/*
  ==============================================================================

    SynthVoice.cpp
    Created: 10 Dec 2020 1:55:41pm
    Author:  Joshua Hodge

  ==============================================================================
*/

#include "SynthVoice.h"


bool SynthVoice::canPlaySound (juce::SynthesiserSound* sound)
{
    return dynamic_cast<juce::SynthesiserSound*>(sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound *sound, int currentPitchWheelPosition)
{
    osc.setWaveFrequency (midiNoteNumber);
    adsr.noteOn();
    filterAdsr.noteOn();
}

void SynthVoice::stopNote (float velocity, bool allowTailOff)
{
    adsr.noteOff();
    filterAdsr.noteOff();
    
    if (! allowTailOff || ! adsr.isActive())
        clearCurrentNote();
}

void SynthVoice::controllerMoved (int controllerNumber, int newControllerValue)
{
    
}

void SynthVoice::pitchWheelMoved (int newPitchWheelValue)
{
    
}

void SynthVoice::prepareToPlay (double sampleRate, int samplesPerBlock, int outputChannels)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.sampleRate = sampleRate;
    spec.numChannels = outputChannels;
    
    osc.prepareToPlay (spec);
    filterAdsr.setSampleRate (sampleRate);
    filter.prepareToPlay (sampleRate, samplesPerBlock, outputChannels);
    adsr.setSampleRate (sampleRate);
    gain.prepare (spec);
    
    gain.setGainLinear (0.3f);
    
    isPrepared = true;
}

void SynthVoice::renderNextBlock (juce::AudioBuffer< float > &outputBuffer, int startSample, int numSamples)
{
    
    jassert (isPrepared);
    
    if (! isVoiceActive())
        return;
    
    synthBuffer.clear();
    synthBuffer.setSize (outputBuffer.getNumChannels(), numSamples, false, false, true);
    
    juce::AudioBuffer<float> synthesisBufferProxy (synthBuffer.getArrayOfWritePointers(), outputBuffer.getNumChannels(), startSample, numSamples);
    juce::dsp::AudioBlock<float> audioBlock{ synthesisBufferProxy };
    
    //juce::dsp::AudioBlock<float> audioBlock { synthBuffer };
    osc.getNextAudioBlock (audioBlock);
    gain.process (juce::dsp::ProcessContextReplacing<float> (audioBlock));
    filter.process (synthesisBufferProxy);
    adsr.applyEnvelopeToBuffer (synthesisBufferProxy, 0, numSamples);
    filterAdsr.applyEnvelopeToBuffer (synthBuffer, 0, numSamples);

    for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
    {
        outputBuffer.addFrom (channel, startSample, synthesisBufferProxy, channel, 0, numSamples, 1.0f);

        if (! adsr.isActive())
            clearCurrentNote();
    }
}

void SynthVoice::updateFilter (const int filterType, const float frequency, const float resonance)
{
    auto modulator = filterAdsr.getNextSample();
    filter.updateParameters (modulator, filterType, frequency, resonance);
}
