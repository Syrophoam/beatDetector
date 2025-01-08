#pragma once

#include <JuceHeader.h>
#include <vector>

#define FFTORDER 11
#define FFTSIZE (1 << FFTORDER) // <- inefficiant and bad
//#define FFTSIZE 1024 // <- not programatic
#define FFTSCOPESIZE (2048)

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

class MainComponent  :  public juce::AudioAppComponent, public juce::Button::Listener
{
public:
    
    //==============================================================================
    MainComponent();
    ~MainComponent() override;
    
    //==============================================================================
    
    void pushNextSampleIntoFifo (float sample, int bufferIndex) noexcept;
    void performFFT();
    void performFFTonTrain();
    float autoCorrelation();
    void cleanPeaks();
//    void* runClock(void* input);
    void buttonClicked (juce::Button* button) override{ if(button == &t)  {buttonPressed = true;} } // use for comparison
    
    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    void drawFrame (juce::Graphics& g)
    {
        // make ur own fft viewer without all this stupid math
//        feniuewe /\urejh
        auto mindB = -100.0f;
        auto maxdB =    0.0f;
        
        for (int i = 1; i < FFTSCOPESIZE; ++i)
        {
            auto width  = getLocalBounds().getWidth();
            auto height = getLocalBounds().getHeight();
            
            
            auto skewedProportionX = 1.0f - std::exp (std::log (1.0f - (float) i / (float) FFTSCOPESIZE) * 0.2f);
            
            auto fftDataIndex = juce::jlimit (0, FFTSIZE / 2, (int) (skewedProportionX * (float) FFTSIZE * 0.5f));
            auto level = juce::jmap (juce::jlimit (mindB, maxdB, juce::Decibels::gainToDecibels (fftData[fftDataIndex])
                                                               - juce::Decibels::gainToDecibels ((float) FFTSIZE)),
                                     mindB, maxdB, 0.0f, 1.0f);
            scopeData[i] = level;                                  

            g.drawLine ({ (float) juce::jmap (i - 1, 0, FFTSCOPESIZE - 1, 0, width),
                                  juce::jmap (scopeData[i - 1], 0.0f, 1.0f, (float) height, 0.0f),
                          (float) juce::jmap (i,     0, FFTSCOPESIZE - 1, 0, width),
                                  juce::jmap (scopeData[i],     0.0f, 1.0f, (float) height, 0.0f) });
            
        }
    }
    
    float sigmoid(float x, float strength, float offset){
        return 1.f/(1.f+std::exp(-strength * (x - offset)));
    }
    
    
    
    void drawSpectEnergy(juce::Graphics& g){
        
        auto width  = getLocalBounds().getWidth();
        auto height = getLocalBounds().getHeight();
        
        int barLen = width / FFTsInWindow;
        
        
        for(int i = 0; i < FFTsInWindow; i++){
            int barHeight = guiPeaks[i] * (float)height;
            barCol[i] *= 0.995;
            if((i < (scopeDataIndex + 3)) && ((i > scopeDataIndex - 3))){ barCol[i] = 1;}
            
            g.setColour(juce::Colour::fromFloatRGBA(0., barCol[i], 0., 1.));
            
            g.fillRect(i * barLen, 0, barLen, barHeight);
            
            g.setColour(juce::Colour::fromFloatRGBA(1., 0.f, 0., .7));
            
            g.fillRect(i * barLen, 0, barLen, int(cleanedPeaks[i] * (float)height));
            
        }
        g.setColour(juce::Colour::fromFloatRGBA(0.f,1.f,0.f,1.f));
    }
    
    void drawRMS(juce::Graphics& g){
        
        auto height = getLocalBounds().getHeight();
        auto width  = getLocalBounds().getWidth();
        
        float normalizer = pow(rollingMaxPeak,-1.f);
        int rmsPos = RMSofSpectEnergy * height * normalizer;
        
        g.drawLine(0, rmsPos, width, rmsPos);
    }

    
    void flash(juce::Graphics& g){
        
        auto height = getLocalBounds().getHeight();
        auto width  = getLocalBounds().getWidth();
        int size = 100;
        
        if( spectralEnergy[scopeDataIndex] > RMSofSpectEnergy ){
//            g.fillAll(juce::Colours::white);
        }
        g.drawText(std::to_string((int)tempo1), (width/2)-size, (height/2)-size, size*10, size*10, juce::Justification::centred);
        
    }
    
    void drawTempo(juce::Graphics& g){
        
        auto height = getLocalBounds().getHeight();
        auto width  = getLocalBounds().getWidth();
        
        
//        g.setFont(200.f);
        
        juce::FontOptions fontOpt;
        
        fontOpt = fontOpt.withHeight(350);
        
        g.setFont(fontOpt);
        std::string tempoStr = std::to_string((int)tempo1);
        
        int x = (width/2)-400, y = (height/2)-200, w = 800, h = 400;
        
        g.drawRect(x, y, w, h);
        
        
        juce::Colour grn = juce::Colour::fromRGB(0  , 255, 0);
        juce::Colour red = juce::Colour::fromRGB(255, 0, 0);
        tempoFound ? g.setColour(grn) : g.setColour(red);
            
        g.drawText(tempoStr, x, y, w, h, juce::Justification::horizontallyCentred);
        
    }
    

private:
    //==============================================================================
    juce::dsp::WindowingFunction<float>     fftWindow;
    juce::dsp::FFT                          forwardFFT;
    
    std::vector<std::array<float, 2 * FFTSIZE>> fifo;
    
    std::vector<int>         fifoIndex;
    int                      numBuffers = 4;
    
    float   fftData [2 * FFTSIZE];
    std::vector<std::vector<float>> filterdFFTData;
    std::vector<double>             spectralEnergySum; // <- use FFTSInWindow
    
    bool    nextFFTBlockReady = false;
    
    float   scopeData [FFTSCOPESIZE];   // for display from tutorial, might be all i care about
    std::vector<std::vector<float>> scopeDataVec;
    
    std::vector<float>              spectralEnergy;                                  //                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    std::vector<float>              filterdSpectralEnergy;                           //                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    std::vector<float>              cleanedPeaks;                                    //                  @@@@@                                  @@@@@
    std::vector<float>              guiPeaks;                                        //                  @@@@@                                  @@@@@
    double                          RMSofSpectEnergy;                                //                  @@@@@     !   CLEAN UP VARIABLES   !   @@@@@
    double                          medianOfSpectEnergy;                             //            @     @@@@@                                  @@@@@
    double                          sampleRate;                                      //         @@@@     @@@@@                                  @@@@@
    float                           avgDistanceOfPeaks;                              //      @@@@@@@     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    float                           rollingMaxPeak = 0;                              //  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    float                           normalizer;                                      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                                                                                     //  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                                                                                     //      @@@@@@@     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    float                           tempo1;                                          //         @@@@     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    int                             samplesPerBeat = 0;                              //            @     @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    int                             clkSampleCntr = 0;                               //                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                                                                                     //                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                                                                                     //                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    int     scopeDataIndex = 2;                                                      //                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    int     FFTsInWindow = 32 * numBuffers;                                         //                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                                                                                     //                  @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    int hopSize;
    bool                            tempoFound = false;
    
    std::unique_ptr<juce::MidiOutput> midiOutPtr = nullptr;
    juce::MidiMessage clk;
    juce::MidiOutput* midiOut;
    unsigned long phase = 0;
    
    
    
    juce::TextButton t;
    bool buttonPressed = false;
    std::vector<float> barCol;
    
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
