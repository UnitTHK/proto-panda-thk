/*
   s3servo.cpp - Library for control servo motor on esp32s3.
   Created by HA.S, October 10, 2022.
   Released into the public domain.
   mit라이선스라고하고싶은데 arduino-esp32 라이선스 따라가야겠지? 아직 라이선스 잘모르니 그냥 arduino-esp32 이거 사용"했으니 이거 따름
*/
#include "tools/s3servo.hpp"
#include "tools/logger.hpp"

#if defined(ESP_IDF_VERSION_MAJOR)
  #if ESP_IDF_VERSION_MAJOR >= 5
    #define USE_NEW_LEDC_API 1  // Core 3.x with IDF 5.x
  #else
    #define USE_NEW_LEDC_API 0  // Core 2.x with IDF 4.x
  #endif
#else
  // If ESP_IDF_VERSION_MAJOR is not defined, assume old API
  #define USE_NEW_LEDC_API 0
#endif

ToneESP32::ToneESP32(int pin, int channel) {	
  this->pin = pin;
  this->channel = channel;
  
  #if USE_NEW_LEDC_API
    // New API (Core 3.x): attach with default frequency
    // Don't set frequency yet - will be set in tone()
    #ifdef PWM_Res
      ledcAttach(pin, 5000, PWM_Res);
    #else
      ledcAttach(pin, 5000, 8);  // Default resolution 8-bit
    #endif
  #else
    // Old API (Core 2.x): just attach pin, frequency will be set in tone()
    ledcAttachPin(pin, channel);
  #endif
}

void ToneESP32::tone(int note) {
  #if USE_NEW_LEDC_API
    // New API (Core 3.x): detach and reattach with new frequency
    ledcDetach(pin);
    #ifdef PWM_Res
      ledcAttach(pin, note, PWM_Res);
    #else
      ledcAttach(pin, note, 8);
    #endif
    ledcWrite(pin, 127);
  #else
    // Old API (Core 2.x): use channel-based setup
    #ifdef PWM_Res
      ledcSetup(channel, note, PWM_Res);
    #else
      ledcSetup(channel, note, 8);
    #endif
    ledcWrite(channel, 127);
  #endif
}

void ToneESP32::noTone() {
  #if USE_NEW_LEDC_API
    ledcWrite(pin, 0);
  #else
    ledcWrite(channel, 0);
  #endif
}


s3servo::McpwmTimerSlot s3servo::_timers[s3servo::MCPWM_GROUPS][s3servo::MCPWM_TIMERS_PER_GROUP];
 
s3servo::s3servo() :
    _frequency(50),
    _pin(-1),
    _minAngle(0),
    _maxAngle(180),
    _minPulseWidth(400),
    _maxPulseWidth(2090),
    _attached(false),
    _paused(false),
    _lastDuty(0),
    _usingMcpwm(false),
    _mcpwmGroup(0),
    _mcpwmTimerIdx(0),
    _comparator(nullptr),
    _generator(nullptr)
{
}
 
s3servo::~s3servo() {
    _fullyRelease();
}
 
void s3servo::_setAngleRange(int min, int max) {
    _minAngle = min;
    _maxAngle = max;
}
 
void s3servo::_setPulseRange(int min, int max) {
    _minPulseWidth = min;
    _maxPulseWidth = max;
}
 
bool s3servo::_allocateMcpwm() {
    for (int g = 0; g < MCPWM_GROUPS; g++) {
        for (int t = 0; t < MCPWM_TIMERS_PER_GROUP; t++) {
            if (_timers[g][t].operatorCount < MCPWM_GENS_PER_OPERATOR) {
                _mcpwmGroup    = g;
                _mcpwmTimerIdx = t;
                return true;
            }
        }
    }
    return false; 
}
 
void s3servo::_releaseMcpwm() {

    McpwmTimerSlot &slot = _timers[_mcpwmGroup][_mcpwmTimerIdx];
 
    if (_generator) {
        mcpwm_del_generator(_generator);
        _generator = nullptr;
    }
    if (_comparator) {
        mcpwm_del_comparator(_comparator);
        _comparator = nullptr;
    }
 
    for (int i = 0; i < MCPWM_GENS_PER_OPERATOR; i++) {
        if (slot.owners[i] == this) slot.owners[i] = nullptr;
    }

    if (slot.operatorCount > 0){
      slot.operatorCount--;
    }
}
 
int8_t s3servo::attach(int pin, int frequency, int min_angle, int max_angle, int min_pulse, int max_pulse)
{

    if (_attached) {
        _fullyRelease();
    }
    
    _frequency = frequency;
    _pin = pin;
    _setAngleRange(min_angle, max_angle);
    _setPulseRange(min_pulse, max_pulse);
 
    if (_allocateMcpwm()) {
        _usingMcpwm = true;
        McpwmTimerSlot &slot = _timers[_mcpwmGroup][_mcpwmTimerIdx];
 
        if (!slot.initialized) {
            mcpwm_timer_config_t timer_config = {};
            timer_config.group_id      = _mcpwmGroup;
            timer_config.clk_src       = MCPWM_TIMER_CLK_SRC_DEFAULT;
            timer_config.resolution_hz = TIMER_RES_HZ;
            timer_config.count_mode    = MCPWM_TIMER_COUNT_MODE_UP;
            timer_config.period_ticks  = (TIMER_RES_HZ / _frequency);
            mcpwm_new_timer(&timer_config, &slot.timer);
 
            mcpwm_operator_config_t operator_config = {};
            operator_config.group_id = _mcpwmGroup;
            mcpwm_new_operator(&operator_config, &slot.oper);
 
            mcpwm_operator_connect_timer(slot.oper, slot.timer);
 
            mcpwm_timer_enable(slot.timer);
            mcpwm_timer_start_stop(slot.timer, MCPWM_TIMER_START_NO_STOP);
 
            slot.initialized = true;
        }
 
        mcpwm_comparator_config_t comparator_config = {};
        comparator_config.flags.update_cmp_on_tez = true; // glitch-free duty updates
        mcpwm_new_comparator(slot.oper, &comparator_config, &_comparator);
 
        mcpwm_generator_config_t generator_config = {};
        generator_config.gen_gpio_num = pin;
        mcpwm_new_generator(slot.oper, &generator_config, &_generator);
 
        mcpwm_generator_set_action_on_timer_event(_generator,
            MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
        mcpwm_generator_set_action_on_compare_event(_generator,
            MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, _comparator, MCPWM_GEN_ACTION_LOW));
 
        mcpwm_comparator_set_compare_value(_comparator, 0);
 
        int genIndex = slot.operatorCount;
        slot.owners[genIndex] = this;
        slot.operatorCount++;
 
    } else {
        // All MCPWM generator slots are taken — fall back to LEDC.
        // Core 3.x's ledcAttach() manages the channel internally.
        _usingMcpwm = false;
        ledcAttach(pin, _frequency, SERVO_RESOLUTION_BITS);
    }
 
    _attached = true;
    _paused = false;
    return 0;
}
 
void s3servo::detach() {
    // "Detach" just pauses output: write a 0 compare value so the pulse
    // effectively disappears, without freeing any hardware. This goes through
    // the comparator's own glitch-free update path (update_cmp_on_tez), so the
    // change only takes effect cleanly at the next period boundary - unlike
    // mcpwm_generator_set_force_level(), which overrides the pin immediately
    // and can catch the shared timer's internal HIGH/LOW state mid-cycle,
    // producing a random glitch pulse.
    if (!_attached || _paused) return;
 
    if (_usingMcpwm) {
        if (_comparator) mcpwm_comparator_set_compare_value(_comparator, 0);
    } else {
        ledcWrite(_pin, 0);
    }
 
    _paused = true;
}
 
void s3servo::reattach() {
    if (!_attached || !_paused){
      return;
    } 
 
    _paused = false;
    writeDuty(_lastDuty);
}
 
void s3servo::_fullyRelease() {
    if (!_attached) return;
 
    if (_usingMcpwm) {
        _releaseMcpwm();
    } else {
        ledcDetach(_pin);
    }
 
    _usingMcpwm = false;
    _attached = false;
    _paused = false;
}
 
float s3servo::mapf(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
 
void s3servo::write(float angle) {
    int duty = (int)mapf(angle, _minAngle, _maxAngle, _minPulseWidth, _maxPulseWidth);
 
    if (duty < _minPulseWidth) duty = _minPulseWidth;
    if (duty > _maxPulseWidth) duty = _maxPulseWidth;
 
    writeDuty(duty);
}
 
void s3servo::writeDuty(int duty) {
    if (duty < 0) duty = 0;
    if (duty > (1 << SERVO_RESOLUTION_BITS) - 1) duty = (1 << SERVO_RESOLUTION_BITS) - 1;
 
    _lastDuty = duty;
 
    if (!_attached || _paused){
      return;
    }
 
    if (_usingMcpwm) {
        if (!_comparator) return;
 

        uint32_t compareTicks = (uint32_t)((float)duty / (float)(1 << SERVO_RESOLUTION_BITS) * (TIMER_RES_HZ / _frequency));
        if (compareTicks >= (TIMER_RES_HZ / _frequency)) compareTicks = (TIMER_RES_HZ / _frequency) - 1;
        //this beeetch weill avoid a servo jitter when reattaching
        mcpwm_comparator_set_compare_value(_comparator, compareTicks);
    } else {
        ledcWrite(_pin, duty);
    }
}

