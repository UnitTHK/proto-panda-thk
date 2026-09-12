#pragma once
#include <esp_adc/adc_continuous.h>
#include "Arduino.h"

class FFT {
  public:
    FFT():m_adc_handle(nullptr),m_managed(true),m_running(false),m_started(false),m_samples(0),m_samplingFreq(0),m_noiseThreshold(0),m_bandCount(0),m_adcBufSize(0),m_fftBuf(nullptr),m_window(nullptr),m_binToBand(nullptr),m_bandValues(nullptr),m_adcBuf(nullptr){}


    bool begin(int gpio=1, int samples = 512, int samplingFreq = 44100, int noiseThreshold = 2000, int bandCount = 16);
    bool start();
    void stop();
    void deinit();
    void update();
    bool isManaged(){
        return m_managed;
    };
    void setManaged(bool b){
        m_managed = b;
    }
    void setNoiseThreshold(int n){
      m_noiseThreshold = n;
    }

    inline int  getBandCount(){
      return m_bandCount;
    };
    bool isRunning();
    int  getBandValue(int i);

  private:
    adc_continuous_handle_t m_adc_handle;
    bool     m_running, m_started, m_managed;
    int      m_samples;
    int      m_samplingFreq;
    int      m_noiseThreshold;
    int      m_bandCount;
    int      m_adcBufSize;
    float*   m_fftBuf;
    float*   m_window;
    int*     m_binToBand;
    int*     m_bandValues;
    uint8_t* m_adcBuf;

    void readAndAnalyse();
    void freeBuffers();
};


extern FFT g_fft;