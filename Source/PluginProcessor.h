/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>



//==============================================================================
/**
*/
class EQAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    EQAudioProcessor();
    ~EQAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;


    //function to provide list of params as an apvts parameter layout
    static juce::AudioProcessorValueTreeState::ParameterLayout createParamterLayout();
    
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParamterLayout()};
    

private:
    //aliases
    using Filter = juce::dsp::IIR::Filter<float>;
    //define a chain. pass in a processing context which will run through each element of the chain. put 4 filters through. can configure as different types of filters peak, shelf, notch etc.
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    //define a chain to rep the whole mono signal path
    //we'll have 2 instances of it to do stereo processing
    using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    MonoChain leftChain, rightChain;
    
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQAudioProcessor)
};
