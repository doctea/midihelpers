#pragma once

#include "bpm.h"
//#include "output.h"

#include "SinTables.h"

#define SUSTAIN_MINIMUM   1   // was 32         // minimum sustain volume to use (below this becomes inaudible, so cut it off)
#define ENV_MAX_ATTACK    (PPQN*2) //48 // maximum attack stage length in ticks
#define ENV_MAX_HOLD      (PPQN*2) //48 // maximum hold stage length
#define ENV_MAX_DECAY     (PPQN*2) //48 // maximum decay stage length
#define ENV_MAX_RELEASE   (PPQN*4) //96 // maximum release stage length

#ifdef ENABLE_SCREEN
#include <LinkedList.h>
#endif

class Menu;

// todo: make EnvelopeBase accept a lambda as callback, instead of the send_envelope_level override

class EnvelopeBase { 
    public:

    const char *label = nullptr;

    EnvelopeBase(const char *label) { 
        //this->randomise();
        this->label = label;
        this->initialise_parameters();
    }

    enum stage : int8_t {
        OFF = 0,
        //DELAY,  // time
        ATTACK,
        HOLD, // time
        DECAY,
        SUSTAIN,
        RELEASE,
        //END = 0
        /*LFO_SYNC_RATIO_HOLD_AND_DECAY,
        LFO_SYNC_RATIO_SUSTAIN_AND_RELEASE,
        ASSIGN_HARMONY_OUTPUT*/
    };

    //#ifndef TEST_LFOS
    int8_t stage = OFF;
    /*#else
    int8_t stage = LFO_SYNC_RATIO;
    #endif*/

    int8_t velocity = 127;         // triggered velocity
    int8_t actual_level = 0;          // right now, the level
    //int8_t stage_start_level = 0;     // level at start of current stage

    unsigned long stage_triggered_at = 0;
    unsigned long triggered_at = 0; 
    unsigned long last_sent_at = 0;

    int trigger_on = 0; // 0->19 = trigger #, 20 = off, 32->51 = trigger #+loop, 64->84 = trigger #+invert, 96->116 = trigger #+loop+invert
    bool loop_mode = false;
    bool invert = false;

    int8_t last_sent_lvl; // value but not inverted
    int8_t last_sent_actual_lvl;  // actual midi value sent

    /*void send_envelope_level(int8_t level) {
        output_wrapper->sendControlChange(midi_cc, level, channel);
    }*/
    virtual void send_envelope_level(uint8_t level) = 0;

    virtual void randomise() = 0;
    virtual void initialise_parameters() = 0;

    virtual void kill() {
        this->stage = OFF;
        //this->stage_start_level = (int8_t)0;
        this->last_state.stage = OFF;
        this->last_state.lvl_start = 0;
        this->last_state.lvl_now = 0;
        send_envelope_level(0);
    }

    // received a message that the state of the envelope should change (note on/note off etc)
    virtual void update_state (int8_t velocity, bool state, uint32_t now = ticks) = 0;

    struct envelope_state_t {
        uint8_t stage = OFF;
        uint8_t lvl_start = 0;
        uint8_t lvl_now = 0;
        uint32_t elapsed = 0;
    };
    envelope_state_t calculate_envelope_level(uint8_t stage, uint8_t stage_elapsed, uint8_t level_start, uint8_t velocity = 127) = 0;

    struct graph_t {
        int_least8_t value = 0;
        char stage = -1;
    };
    graph_t graph[240];

    void calculate_graph() {
        envelope_state_t graph_state = {
            .stage = ATTACK,
            .lvl_start = 0,
            .lvl_now = 0
        };
        int stage_elapsed = 0;
        for (int i = 0 ; i < 240 ; i++) {
            envelope_state_t result = calculate_envelope_level(graph_state.stage, stage_elapsed, graph_state.lvl_start, velocity);
            if (result.stage != graph_state.stage) {
                graph_state.lvl_start = result.lvl_now;
                stage_elapsed = 0;
            } else {
                stage_elapsed++;
            }
            graph[i].value = result.lvl_now;
            graph[i].stage = result.stage;
            graph_state.stage = result.stage;
            if (result.stage==SUSTAIN && stage_elapsed >= PPQN) {
                stage_elapsed = 0;
                graph_state.stage = RELEASE;    // move to release after 1 beat, if we are calculating the graph
            }
        }
    }

    envelope_state_t last_state = {
        .stage = OFF,
        .lvl_start = 0,
        .lvl_now = 0,
        .elapsed = 0
    };
    void process_envelope(unsigned long now = millis()) {
        //now = ticks;
        unsigned long elapsed = now - this->stage_triggered_at;
        //unsigned long real_elapsed = elapsed;    // elapsed is currently the number of REAL ticks that have passed

        envelope_state_t new_state = calculate_envelope_level(last_state.stage, elapsed, last_state.lvl_start, velocity);
        if (new_state.stage!=last_state.stage) {
            //Serial.printf("process_envelope(now=%-3i) began at stage %i, changed to stage %i (elapsed is %-3i)\n", now, last_state.stage, new_state.stage, elapsed);
            this->stage_triggered_at = now;
        }

        if (this->last_sent_actual_lvl != new_state.lvl_now) {
            send_envelope_level(new_state.lvl_now);
            last_sent_actual_lvl = new_state.lvl_now;
        }

        last_state.elapsed = elapsed;
        last_state.stage = new_state.stage;
        last_state.lvl_start = new_state.lvl_start;
        last_state.lvl_now = new_state.lvl_now;
    }


    
    #ifdef ENABLE_SCREEN
        virtual void make_menu_items(Menu *menu, int index);
    #endif

};



class RegularEnvelope : public EnvelopeBase {
    public:

    RegularEnvelope(const char *label) : EnvelopeBase(label) {}

    // TODO: int delay_length = 5;                    // D - delay before atack starts
    unsigned int  attack_length   = 0;                // A - attack  - length of stage
    unsigned int  hold_length     = (PPQN / 4) - 1;   // H - hold    - length to hold at end of attack before decay
    unsigned int  decay_length    = (PPQN / 2) - 1;   // D - decay   - length of stage
    float         sustain_ratio   = 0.90f;            // S - sustain - level to drop to after decay phase
    unsigned int  release_length  = (PPQN / 2) - 1;   // R - release - length (time to drop to 0)

    int8_t lfo_sync_ratio_hold_and_decay = 0;
    int8_t lfo_sync_ratio_sustain_and_release = 0;

    int8_t cc_value_sync_modifier = 12;

    virtual void randomise() {
        //this->lfo_sync_ratio_hold_and_decay = (int8_t)random(0,127);
        //this->lfo_sync_ratio_sustain_and_release = (int8_t)random(0,127);
        this->set_attack((int8_t)random(0,64));
        this->set_hold((int8_t)random(0,127));
        this->set_decay((int8_t)random(0,127));
        this->set_sustain(random(64,127));
        this->set_release((int8_t)random(0,127));
        this->invert = (int8_t)random(0,10) < 2;
    }

    virtual void initialise_parameters() {
        //this->lfo_sync_ratio_hold_and_decay = (int8_t)random(0,127);
        //this->lfo_sync_ratio_sustain_and_release = (int8_t)random(0,127);
        this->set_attack(0);
        this->set_hold((int8_t)random(0,127));
        this->set_decay((int8_t)random(0,127));
        this->set_sustain(random(64,127));
        this->set_release((int8_t)random(0,127));
        this->invert = (int8_t)random(0,10) < 2;
    }

    // received a message that the state of the envelope should change (note on/note off etc)
    virtual void update_state (int8_t velocity, bool state, uint32_t now = ticks) {
        //unsigned long now = ticks; //clock_millis(); 
        //unsigned long env_time = millis();
        if (state == true) { //&& this->stage==OFF) {  // envelope told to be in 'on' state by note on
            this->velocity = velocity;
            this->actual_level = velocity; // TODO: start this at 0 so it can ramp / offset level feature
            //this->stage_start_level = velocity; // TODO: start this at 0 so it can ramp / offset level feature
            this->stage = ATTACK;
            last_state.stage = ATTACK;
            last_state.lvl_start = velocity;
            this->triggered_at = now;
            this->stage_triggered_at = now;
            this->last_sent_at = 0;  // trigger immediate sending

            //NUMBER_DEBUG(7, this->stage, this->attack_length);
        } else if (state == false && this->stage != OFF) { // envelope told to be in 'off' state by note off
            // note ended - fastforward to next envelope stage...?
            // if attack/decay/sustain, go straight to release at the current volume...?
            switch (this->last_state.stage) {
            case RELEASE:
                // received note off while already releasing -- cut note short
                //this->stage_start_level = 0; 
                /*this->stage = last_state.stage = OFF;
                last_state.lvl_start = last_state.lvl_now;
                this->stage_triggered_at = now;*/
                return;
            case OFF:
                // don't do anything if we're in this stage and we receive note off, since we're already meant to be stopping by now
                /*NUMBER_DEBUG(15, 15, 15);
                NUMBER_DEBUG(15, 15, 15);
                NUMBER_DEBUG(15, 15, 15);
                NUMBER_DEBUG(15, 15, 15);*/
                return;

            // if in any other stage, jump straight to RELEASE stage at current volume
            case ATTACK:
            case HOLD:
                // TODO: continue to HOLD , but leap straight to RELEASE when reached end?
            case DECAY:
            case SUSTAIN:
            default:
                //NOISY_DEBUG(500, 2);
                //NUMBER_DEBUG(13, 13, 13);
                //Serial.printf("update_state moving to RELEASE because gate ended!  lvl is %-3i, now is %-3i\n", last_state.lvl_now, now);
                this->stage = last_state.stage = RELEASE;
                //this->stage_start_level = this->actual_level;
                last_state.lvl_start = last_state.lvl_now;
                this->stage_triggered_at = now;
                this->last_sent_at = 0;  // trigger immediate sending
                break;
            }
        }
    }

    envelope_state_t calculate_envelope_level(uint8_t stage, uint8_t stage_elapsed, uint8_t level_start, uint8_t velocity = 127) {
        float ratio = (float)PPQN / (float)cc_value_sync_modifier;  // calculate ratio of real ticks : pseudoticks
        unsigned long elapsed = (float)stage_elapsed * ratio;   // convert real elapsed to pseudoelapsed
        //unsigned long elapsed = stage_elapsed;

        uint8_t lvl;
        envelope_state_t return_state = {
            stage,
            level_start,
            level_start
        };

        if (stage == ATTACK) {
            if (attack_length==0)
                lvl = velocity;
            else
                lvl = (int8_t) ((float)velocity * ((float)elapsed / ((float)this->attack_length )));

            return_state.lvl_now = lvl;
            if (elapsed >= this->attack_length) {
                //Serial.printf("calculate_envelope_level in ATTACK, moving to HOLD because elapsed %-3i >= attack_length %3i\n", elapsed, attack_length);
                return_state = { .stage = ++stage, .lvl_start = lvl, .lvl_now = lvl };
            }
        } else if (stage == HOLD && this->hold_length>0) {
            lvl = velocity;
            return_state.lvl_now = lvl;
            if (elapsed >= hold_length) {
                //Serial.printf("calculate_envelope_level in HOLD, moving to SUSTAIN because elapsed %-3i >= hold_length %3i\n", elapsed, hold_length);
                return_state = { .stage = ++stage, .lvl_start = lvl, .lvl_now = lvl };
            }
        } else if (stage == HOLD || stage == DECAY) {
            float f_sustain_level = sustain_ratio * velocity; //SUSTAIN_MINIMUM + (this->sustain_ratio * (float)level_start);
            float f_original_level = level_start;

            if (stage==HOLD)
                return_state.stage = DECAY;

            if (this->decay_length>0) {
                float decay_position = ((float)elapsed / (float)(this->decay_length));
        
                // we start at stage_start_level
                float diff = (f_original_level - (f_sustain_level));
                // and over decay_length time
                // we want to scale down to f_sustain_level at minimum
        
                lvl = f_original_level - (diff * decay_position);
            } else {
                // if there's no decay stage then set level to the sustain level
                lvl = f_sustain_level; 
            }
            return_state.lvl_now = lvl;
            if (elapsed >= this->decay_length) {
                //Serial.printf("calculate_envelope_level in DECAY, moving to SUSTAIN because elapsed %-3i >= decay_length %3i\n", elapsed, decay_length);
                return_state = { .stage = SUSTAIN, .lvl_start = lvl, .lvl_now = lvl };
            }
        } else if (stage == SUSTAIN) {
            //return_state.lvl_now = (unsigned char)sustain_value;
            return_state.lvl_now = sustain_ratio * velocity;
            if (sustain_ratio==0.0 || this->loop_mode) {
                //Serial.printf("calculate_envelope_level in SUSTAIN, moving to RELEASE because sustain_ratio or loop_mode!\n");
                return_state = { .stage = RELEASE, .lvl_start = return_state.lvl_now, .lvl_now = return_state.lvl_now };
            }
        } else if (stage == RELEASE) {
            if (this->sustain_ratio == 0.0f) {
                // start level = velocity
            }
            if (this->release_length>0) {
                float eR = (float)elapsed / (float)(this->release_length); 
                eR = constrain(eR, 0.0f, 1.0f);
        
                //NUMBER_DEBUG(8, this->stage, this->stage_start_level);
                //Serial.printf("in RELEASE stage, release_length is %u, elapsed is %u, eR is %3.3f, lvl is %i ....", this->release_length, elapsed, eR, lvl);
                lvl = (int8_t)((float)level_start * (1.0f-eR));                
            } else {
                lvl = 0;
            }
            return_state.lvl_now = lvl;
            if (elapsed > this->release_length || this->release_length==0 || lvl==0) {
                //Serial.printf("calculate_envelope_level in RELEASE, moving to OFF because either elapsed %-3i > release_length %-3i, or lvl==0 (lvl is actually %3i)!\n", elapsed, release_length, lvl);
                return_state = { .stage = OFF, .lvl_start = lvl, .lvl_now = lvl };
            }
        } else if (stage == OFF) {
            return_state = { .stage = OFF, .lvl_start = 0, .lvl_now = 0 };
        }

        if (stage!=OFF) {
            int8_t lvl = return_state.lvl_now;
            int sync = (stage==DECAY || stage==HOLD) 
                            ?
                            this->lfo_sync_ratio_hold_and_decay
                            :
                        (stage==SUSTAIN || stage==RELEASE)
                            ?
                            this->lfo_sync_ratio_sustain_and_release : 
                        -1;
            if (sync>=0) {            
                sync *= 4; // multiply sync value so that it gives us possibility to modulate much faster

                float mod_amp = (float)lvl/4.0f; //32.0; // modulation amplitude is a quarter of the current level

                float mod_result = mod_amp * isin((float)elapsed * PPQN * ((float)sync/127.0f));
                //Serial.printf("mod_result is %3.1f, elapsed is %i, sync is %i\r\n", mod_result, elapsed, sync);
                
                lvl = constrain(
                    lvl + mod_result,
                    0,
                    127
                );
                //Serial.printf("sync of %i resulted in lvl %i\r\n", sync, lvl);
                return_state.lvl_now = lvl;
            }           
        } else {
            if (this->loop_mode)
                return_state.stage = ATTACK;
        }

        return_state.lvl_now = this->invert ? 127-return_state.lvl_now : return_state.lvl_now;

        return return_state;
    }

    int attack_value, hold_value, decay_value, sustain_value, release_value;

    virtual void set_attack(int8_t attack) {
        this->attack_value = attack;
        this->attack_length = (ENV_MAX_ATTACK) * ((float)attack/127.0f);
        calculate_graph();
    }
    virtual int8_t get_attack() {
        return this->attack_value;
    }
    virtual void set_hold(int8_t hold) {
        this->hold_value = hold;
        this->hold_length = (ENV_MAX_HOLD) * ((float)hold/127.0f);
        calculate_graph();
    }
    virtual int8_t get_hold() {
        return this->hold_value;
    }
    virtual void set_decay(int8_t decay) {
        this->decay_value = decay;
        decay_length   = (ENV_MAX_DECAY) * ((float)decay/127.0f);
        calculate_graph();
    }
    virtual int8_t get_decay() {
        return this->decay_value;
    }
    virtual void set_sustain(int8_t sustain) {
        this->sustain_value = sustain; //(((float)value/127.0f) * (float)(128-SUSTAIN_MINIMUM)) / 127.0f;
        //sustain_ratio = (((float)sustain/127.0f) * (float)(128-SUSTAIN_MINIMUM)) / 127.0f;
        float sustain_normal = ((float)sustain)/127.0f;
        this->sustain_ratio = sustain_normal;
        calculate_graph();
    }
    virtual int8_t get_sustain() {
        return this->sustain_value;
    }
    virtual void set_release(int8_t release) {
        this->release_value = release;
        release_length = (ENV_MAX_RELEASE) * ((float)release/127.0f);
        calculate_graph();
    }
    virtual int8_t get_release() {
        return this->release_value;
    }
    virtual bool is_invert() {
        return invert;
    }
    virtual void set_invert(bool i) {
        this->invert = i;
        calculate_graph();
    }
    virtual bool is_loop() {
        return loop_mode;
    }
    virtual void set_loop(bool i) {
        this->loop_mode = i;
        calculate_graph();
    }
    virtual int8_t get_stage() {
        return this->stage;
    }
    virtual void set_mod_hd(int8_t hd) {
        this->lfo_sync_ratio_hold_and_decay = hd;
        //this->attack_length = (ENV_MAX_ATTACK) * ((float)attack/127.0f);
        calculate_graph();
    }
    virtual int8_t get_mod_hd() {
        return this->lfo_sync_ratio_hold_and_decay;
    }
    virtual void set_mod_sr(int8_t sr) {
        this->lfo_sync_ratio_sustain_and_release = sr;
        //this->attack_length = (ENV_MAX_ATTACK) * ((float)attack/127.0f);
        calculate_graph();
    }
    virtual int8_t get_mod_sr() {
        return this->lfo_sync_ratio_sustain_and_release;
    }
    virtual void set_cc_value_sync_modifier(uint8_t sync) {
        this->cc_value_sync_modifier = sync;
    }
    virtual uint8_t get_cc_value_sync_modifier() {
        return this->cc_value_sync_modifier;
    }

};