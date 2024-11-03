/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"


//==============================================================================
EQAudioProcessor::EQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

EQAudioProcessor::~EQAudioProcessor()
{
}

//==============================================================================
const juce::String EQAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EQAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EQAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EQAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EQAudioProcessor::getProgramName (int index)
{
    return {};
}

void EQAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    DBG("reached prep2play");

    
    //prepare channels by passing a process spec object to the chains which will pass it to each link in the chain
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1; //mono chain can only handle 1 channel
    spec.sampleRate = sampleRate;
    
    //pass it to each chain to prepare them for processing
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    
    DBG("about to get chain settings from ");
    
    auto chainSettings = getChainSettings(apvts);
    DBG("about to get make pfcoeffs");
    //makepeakfilter makes our filter coefficients from our parameters. convert decibles to gain
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                                                chainSettings.peakFreq,
                                                                                chainSettings.peakQuality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));
    DBG("about to pass chain settings to chains");
    //get the peak filter from the chain using the enum we defd in the header. assign its coefficents to what we just defined above
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    
    
}

void EQAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    DBG("reached processblock");
    
    //gets a buffer. extract right and left channels. 0 and 1
    //make a processing context for the chain. needs an audio block
    
    
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    //update param values before we process audio
    auto chainSettings = getChainSettings(apvts);
    //makepeakfilter makes our filter coefficients from our parameters. convert decibles to gain
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                                chainSettings.peakFreq,
                                                                                chainSettings.peakQuality,
                                                                                juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibles));
    
    //get the peak filter from the chain using the enum we defd in the header. assign its coefficents to what we just defined above
    *leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

    
    
    juce::dsp::AudioBlock<float> block(buffer); //make a block. wraps the buffer. use it to extract channels
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    
    //now we have audio blocks representing each channel. now create processing contexts that wrap each block
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    
    //now we have blocks and contexts for each cahnnel. pass contexts to the mono filter chains
    leftChain.process(leftContext);
    rightChain.process(rightContext);
    
    
       
    
    
}

//==============================================================================
bool EQAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EQAudioProcessor::createEditor()
{
    //return new EQAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
    
}

//==============================================================================
void EQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void EQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//get parameters from apvts in form of the ChainSettings struct we declared in header
ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts){
    ChainSettings settings;
    
    
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load(); //raw gets it in our original range not normalizes
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibles = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = apvts.getRawParameterValue("LowCut Slope")->load();
    settings.highCutSlope = apvts.getRawParameterValue("HighCut Slope")->load();
    
    return settings;
}

juce::AudioProcessorValueTreeState::ParameterLayout EQAudioProcessor::createParamterLayout(){
    
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add(std::make_unique<juce::AudioParameterFloat>( juce::ParameterID { "LowCut Freq", 1 }, //id and version hint
                                                          "LowCut Freq", //param name
                                                          juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), //range start end step and skew
                                                          20.f)); //default value
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "HighCut Freq", 1 },
                                                          "HighCut Freq",
                                                          juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                          20000.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "Peak Freq", 1 },
                                                          "Peak Freq",
                                                          juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
                                                          750.f));
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "Peak Gain", 1 },
                                                          "Peak Gain",
                                                          juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                          0.0f));
    
    //how tight. not really a unit, just an abstract range here. we'll say 0.1 to 10
    layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "Peak Quality", 1 },
                                                          "Peak Quality",
                                                          juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                          1.f));
    
    //for choices of slope steepness. takes all multiples of 12.
    juce::StringArray stringArray;
    //populate it
    for(int i=0; i<4; ++i){
        juce::String str;
        str << (12 + i*12);
        str << " db/Octave";
        stringArray.add(str);
    }
    
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID {"LowCut Slope", 1}, //id and version hint
                                                           "LowCut Slope", //name
                                                           stringArray, //choices
                                                            0)); //default value
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID {"HighCut Slope", 1}, "HighCut Slope", stringArray,0));
    

    
    
    
    return layout;
    
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQAudioProcessor();
}


 
