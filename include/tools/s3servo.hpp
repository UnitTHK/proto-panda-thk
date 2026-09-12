/*
   s3servo.h - Library for control servo motor on esp32s3.
   Created by HA.S, October 10, 2022.
   Released into the public domain.
   mit라이선스라고하고싶은데 arduino-esp32 라이선스 따라가야겠지? 아직 라이선스 잘모르니 그냥 arduino-esp32 이거 사용했으니 이거 따름
*/

#ifndef S3SERVO_H
#define S3SERVO_H

#include "Arduino.h"
#include "driver/mcpwm_prelude.h"

#if defined (CONFIG_IDF_TARGET_ESP32S3)
#define CHANNEL_MAX_NUM  7
#define MAX_BIT_NUM  14
#else 
#define CHANNEL_MAX_NUM  15
#define MAX_BIT_NUM  16
#endif

#define PWM_Res   10
#define TIMER_RES_HZ 1000000

class ToneESP32 { 
  private:
    int pin; 
    int channel;
  public:    
    ToneESP32(int pin, int channel);      
    void tone(int note);
    void noTone();    
};

class s3servo {
 
public:
 
    s3servo();
    ~s3servo();
 
    int8_t attach(int pin, int frequency, int min_angle, int max_angle, int min_pulse, int max_pulse); // emax es9251II
    void detach();
    void reattach();
 
    void write(float angle);
    void writeDuty(int duty);
 
    bool isAttached() { return _attached; }
    bool isPaused()   { return _paused; }
    bool usingMcpwm() { return _usingMcpwm; }
    int  getPin()     { return _pin; }
 
private:
    float mapf(float x, float in_min, float in_max, float out_min, float out_max);
    void  _setAngleRange(int min, int max);
    void  _setPulseRange(int min, int max);
  
    // ESP32-S3 MCPWM peripheral shape: 2 groups x 3 timers, each operator drives 2 generators (A/B)
    static const int MCPWM_GROUPS            = 2;
    static const int MCPWM_TIMERS_PER_GROUP  = 3;
    static const int MCPWM_GENS_PER_OPERATOR = 2;
 
    // One timer+operator is shared by up to 2 servos (one per generator/GPIO).
    // Shared across ALL s3servo instances since these are global hardware resources.
    // NOTE: once created, a slot's timer+operator are intentionally never deleted
    // (see _releaseMcpwm) — deleting a running MCPWM timer safely requires waiting
    // for an async stop-at-TEZ event, which is racy to do on every detach. Instead
    // the timer/operator are created once and reused for the life of the program;
    // only the per-servo comparator+generator are created/destroyed on attach/detach.

    struct McpwmTimerSlot {
        uint8_t              operatorCount = 0;
        bool                 initialized   = false;
        mcpwm_timer_handle_t timer         = nullptr;
        mcpwm_oper_handle_t  oper          = nullptr;
        s3servo*             owners[MCPWM_GENS_PER_OPERATOR] = { nullptr, nullptr };
    };

    static McpwmTimerSlot _timers[MCPWM_GROUPS][MCPWM_TIMERS_PER_GROUP];
 
    bool _allocateMcpwm();  
    void _releaseMcpwm(); 
    void _fullyRelease();
 
    int _frequency;
    int  _pin;
    int  _minAngle;
    int  _maxAngle;
    int  _minPulseWidth;
    int  _maxPulseWidth;
    bool _attached;
    bool _paused;  
    int  _lastDuty;
 
    bool _usingMcpwm;
    int  _mcpwmGroup;
    int  _mcpwmTimerIdx;
    mcpwm_cmpr_handle_t _comparator;
    mcpwm_gen_handle_t  _generator;
};


#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS  455
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GSH  830
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

#endif

