#include "MainComponent.h"
#include<algorithm>
#include <pthread.h>


//==============================================================================
MainComponent::MainComponent():
    fftWindow(FFTSIZE, juce::dsp::WindowingFunction<float>::hamming),
    forwardFFT(FFTORDER)
{
    setSize (800, 600);
//    addAndMakeVisible(t);
    t.setButtonText("test");
    t.setColour (juce::Label::backgroundColourId, juce::Colours::black);
    t.setColour (juce::Label::textColourId, juce::Colours::white);
    t.setSize(100, 100);
    t.setCentrePosition(50, 50);
    t.addListener(this);
    
    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired (juce::RuntimePermissions::recordAudio)
        && ! juce::RuntimePermissions::isGranted (juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request (juce::RuntimePermissions::recordAudio,
                                           [&] (bool granted) { setAudioChannels (granted ? 2 : 0, 2); });
    }
    else
    {
        setAudioChannels (8, 8);
    }
}

MainComponent::~MainComponent()
{
    t.removeListener(this);
    shutdownAudio();
}

//==============================================================================
struct clockArgs {
    juce::MidiOutput*   midiOutPtr;
};

int nanoSecondsToSleep = 0;
bool midiClockShouldRun = true;
void* runClock(void* input){
    
    juce::MidiMessage clk = juce::MidiMessage(0xF8);
    clockArgs clkArg = *(clockArgs*)input;
    
    juce::MidiOutput* midiOut = clkArg.midiOutPtr;
    midiOut->startBackgroundThread();
    
    while (true) {
        if(nanoSecondsToSleep > 0)
            break;
    }
//    midiOut->sendMessageNow(juce::MidiMessage(0xFA));
    while (midiClockShouldRun) {
         
//        midiOut->sendMessageNow(clk);
        
//        std::this_thread::sleep_for(std::chrono::nanoseconds(nanoSecondsToSleep));
        
    }
    
    pthread_exit(NULL);
    return nullptr;
}
pthread_t thrd;


void MainComponent::prepareToPlay (int samplesPerBlockExpected, double smpleRate)
{
        sampleRate = smpleRate;
        
    std::cout << "sample rate: "<<sampleRate<<'\n';
    fifoIndex.resize(numBuffers);
    fifo.resize(numBuffers);
    
    hopSize = FFTSIZE / numBuffers;
    for (int i = 0; i < numBuffers; i++) {
        fifoIndex[i] = hopSize * i;
    }
    
    
    
//    FFTsInWindow = fmax(ceil((sampleRate / 1000.f * 200.f) / FFTSIZE),2);
    
    spectralEnergySum.resize(FFTsInWindow);
    guiPeaks.resize(FFTsInWindow);
    
    scopeDataVec.resize(FFTsInWindow);
    filterdFFTData.resize(FFTSCOPESIZE);
    spectralEnergy.resize(FFTsInWindow);
    filterdSpectralEnergy.resize(FFTsInWindow);
    cleanedPeaks.resize(FFTsInWindow);
    
    
    juce::Array<juce::MidiDeviceInfo> midiDevices = juce::MidiOutput::getAvailableDevices();
    
    
    for (int i = 0; i < midiDevices.size(); i++) {
        std::cout<<i<<midiDevices[i].name<<'\n';
    }
    int userOpt = 0;
//    std::cin>>userOpt;
    userOpt = 0; // <- iac
    
    juce::String devId = midiDevices[userOpt].identifier;
    midiOutPtr = juce::MidiOutput::openDevice(devId);
    midiOutPtr->startBackgroundThread();
    
    std::cout<<midiDevices[userOpt].name<<std::endl;
    
//    midiOutPtr->clearAllPendingMessages();
    
    auto* midiOPtr = midiOutPtr.get();
    int timeToSleep = 10;
    
    
//    struct clockArgs clkArgs = {.midiOutPtr = midiOPtr};
//    pthread_create(&thrd, NULL, runClock, (void*)&clkArgs);
    
    for (int i = 0; i < FFTSCOPESIZE; i++) {
        filterdFFTData[i].resize(FFTsInWindow);
    }
    
    for(int i = 0; i < FFTsInWindow; i++){
        scopeDataVec[i].resize(FFTSCOPESIZE);
    }
    
    clk = juce::MidiMessage(0xF8);
    
    midiOut = midiOPtr;
    midiOut->startBackgroundThread();
    
    
    barCol.resize(FFTsInWindow);
    for (int i = 0; i < FFTsInWindow; i++) {
        barCol[i] = 1;
    }
//    nsView.setView(<#void *nsView#>)
    
    

}

void MainComponent::pushNextSampleIntoFifo (float sample, int bufferIndex) noexcept
    {
        
        if (fifoIndex[bufferIndex] == FFTSIZE) {
            if (! nextFFTBlockReady) {
                
                juce::zeromem (fftData, sizeof (fftData));
                
                for (int i = 0; i < sizeof(fifo[bufferIndex]) / sizeof(float); i++) {
                    fftData[i] = fifo[bufferIndex][i];
                }
                nextFFTBlockReady = true;
            }
            fifoIndex[bufferIndex] = 0;
        }
        fifo[bufferIndex][ fifoIndex[bufferIndex]++ ] = sample;             // [12]
    }

float MainComponent::autoCorrelation(){
    
    float sum;
    std::vector<float> autoCorrelationData(FFTsInWindow,0);
    for (int i = 0; i < FFTsInWindow; i++) {
        sum = 0.f;
        for (int j = 0; j < FFTsInWindow - i; j++) {
            sum = sum + (spectralEnergy[j + i] * spectralEnergy[j]);
        }
        float val = sum/float(FFTsInWindow);
        autoCorrelationData[i] = val;
    }
    
//    long searchMin = (float)sampleRate/((float)sampleRate/2.f); // <- nyquist
    long searchMin = 8; // <- came from my brain, max bpm of 206
    
    float maxVal = 0;
    long  maxPos = 0;
    //    maxPos = 50; <- use to see max bpm, TODO paramatarize this
    
//    maxVal = *std::max_element(autoCorrelationData.begin() + searchMin, autoCorrelationData.end());
    
    std::vector<int> maxPosVec(1,0);
    int detectedMaxima = 0;
    int maxPosAccum = 0;
    
    
    sum = 0;
    for (int i = 0; i < FFTsInWindow; i++) {
        sum += pow(autoCorrelationData[i],2.f);
    }
    sum /= FFTsInWindow;
    float RMS = sqrt((1.f/FFTsInWindow) * sum);
    
    tempoFound = true;
    while( (maxPosAccum < autoCorrelationData.size()) && tempoFound){
        tempoFound = autoCorrelationData[maxPosAccum] > (0.3 * RMS);
        if(tempoFound){
            maxPos += searchMin;
            maxPos = std::distance(autoCorrelationData.begin() + maxPos, std::max_element(autoCorrelationData.begin() + searchMin + maxPos, autoCorrelationData.end()));
            maxPosVec.push_back(maxPos);
            maxPosAccum += maxPos;
        }
    }
    
    detectedMaxima = maxPosVec.size();
    
    bool firstValIsZero = false;
    if(maxPosVec.size() > 1){
        if(maxPosVec[0] == 0){
            maxPosVec.erase(maxPosVec.begin());
            firstValIsZero = true;
        }
    }
    
    float avgMaxPos = 0;
    for(int i = 0; i < (detectedMaxima - (int)firstValIsZero); i++){avgMaxPos += maxPosVec[i];}
    avgMaxPos /= detectedMaxima;
    
    float medMaxPos = 0;
    int t = 0;
    if(detectedMaxima % 2){
        medMaxPos = (maxPosVec[detectedMaxima / 2] + maxPosVec[(detectedMaxima / 2) + 1]) / 2;
    }else{
        medMaxPos = maxPosVec[detectedMaxima / 2];
    }
    maxVal = autoCorrelationData[medMaxPos];
//    maxPos = maxPosVec[0];
//    tempoFound = true;
    if(tempoFound){
        return (44100.f / float(hopSize)) / medMaxPos * 60.f;
    }else{
        return (44100.f / float(hopSize)) / medMaxPos * 60.f;
    }
    
}

void MainComponent::performFFT(){ // break this into more functions
    
    fftWindow.multiplyWithWindowingTable(fftData, FFTSIZE);
    
    forwardFFT.performFrequencyOnlyForwardTransform(fftData);
    
    nextFFTBlockReady = false;
   
    for (int i = 0; i < FFTSIZE; ++i){
        scopeDataVec[scopeDataIndex][i] = fftData[i] * 2.f / double(forwardFFT.getSize()); // <- mabye dont want it normalized?
    }
    
    float sum = 0;
    float maxSum = FFTSIZE;
    
    for (int i = 0; i < FFTSIZE; i++) {
        float binFlux = 0;
        float binFluxB = 0;
        
        float binA = pow(scopeDataVec[scopeDataIndex - 1][i],2.f); // <- doesnnt wrap around to last scope data
        float binB = pow(scopeDataVec[scopeDataIndex]    [i],2.f);
        
        float binAA = pow(scopeDataVec[scopeDataIndex - 2][i],2.f); // <- doesnnt wrap around to last scope data
        float binBB = pow(scopeDataVec[scopeDataIndex - 1][i],2.f);
        
        binFlux = pow(abs(binA - binB),2.f);
        binFluxB = pow(abs(binAA - binBB),2.f);
        sum += (binFlux + binFluxB)/2;
    }
    spectralEnergy[scopeDataIndex] = sum/maxSum;
    
    float maxPeak = fmax(*std::max_element(spectralEnergy.begin() + (spectralEnergy.size()/2), spectralEnergy.end()),0.00000001);
    
    if( !((rollingMaxPeak > (maxPeak - (maxPeak * 0.2))) && (rollingMaxPeak < (maxPeak + (maxPeak * 0.2))))) {
        
        if(rollingMaxPeak < maxPeak){
            rollingMaxPeak += (maxPeak * 0.02);
        }
        else{
            rollingMaxPeak *= 0.98f;
        }
    }
    
    normalizer = pow(rollingMaxPeak,-1.f);
    
    sum = 0;
    for (int i = 0; i < FFTsInWindow; i++) {
        sum += pow(spectralEnergy[i],2.f);
    }
    sum /= FFTsInWindow;
    RMSofSpectEnergy = sqrt((1.f/FFTsInWindow) * sum) * 16.f;
    RMSofSpectEnergy = 0.5 / normalizer;
    
    for (int i = 0; i < FFTsInWindow; i++) {
        (spectralEnergy[i] > RMSofSpectEnergy) ? cleanedPeaks[i] = 1 : cleanedPeaks[i] = 0;
    }
    
    for(int i = 0; i < FFTsInWindow; i++){
        guiPeaks[i] = spectralEnergy[i] * normalizer;
    }
    
    if( scopeDataIndex++ == (FFTsInWindow - 1) ) {
        scopeDataIndex = 2;
        
        nanoSecondsToSleep = ((60.f/120.f) * 1e9)/24.f; // <- Testing
        
        tempo1 = autoCorrelation();
        
        float beatsPerSec = tempo1 / 60.f;
        samplesPerBeat = (pow(beatsPerSec,-1.f) * sampleRate); //< - super wrong
    }
}

void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
    
    for (auto i = 0; i < bufferToFill.numSamples; ++i){
        for(int j = 0; j < numBuffers; j++){
            pushNextSampleIntoFifo (channelData[i],j);
        }
    }
    
    if(nextFFTBlockReady){
        performFFT();
        
        const juce::MessageManagerLock mmLock;
        MainComponent::repaint(); 
    }
    
    juce::MidiBuffer clkBuff;
    bufferToFill.clearActiveBufferRegion();
//    const auto* bufferWr[2] = bufferToFill.buffer->getReadPointer(0);
    
    
    for (int i = 0; i < bufferToFill.numSamples; i++) {
        if (samplesPerBeat){
            if (clkSampleCntr++ == samplesPerBeat) {
                clkSampleCntr = 0;
                clkBuff.addEvent(clk, i);
                bufferToFill.buffer->getWritePointer(2)[i] = 1; /* < - still not consistent */
            }
        }
        
        float piPhase = (float(phase++) / sampleRate) * M_2_PI * 220.f;
        float sine = sin(piPhase) / 4.f;
        bufferToFill.buffer->getWritePointer(3)[i] = float(i)/float(bufferToFill.numSamples);
        
        
    }
//    midiOut->sendBlockOfMessagesNow(clkBuff);
    midiOut->sendBlockOfMessages(clkBuff, juce::Time::getMillisecondCounterHiRes(), sampleRate);
    
    
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    g.fillAll (juce::Colours::black);
    
    g.setOpacity (1.0f);
    g.setColour (juce::Colours::white);
    drawFrame(g);
    g.setColour(juce::Colours::green);
    drawSpectEnergy(g);
    drawRMS(g);
    flash(g);
    drawTempo(g);
}


void MainComponent::resized(){
    
    std::cout<<getLocalBounds().getHeight()<<'\n';
    std::cout<<getLocalBounds().getWidth()<<'\n';
    
}
