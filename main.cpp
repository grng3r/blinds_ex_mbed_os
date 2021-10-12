/* mbed Microcontroller Library
 * Copyright (c) 2021 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
/* 1. Usage:
*   - 2 modes: switch from winter to summer on long button press, switch back on reset press
*   - 4 states: indicated by leds on the board and terminal output
*   
* 2. Improvments: Need to use a signaling mechanism instead of convinient timings to schedule threads*/

typedef struct{
    int al;
} amb_l;

unsigned short th = 65535;//tresholld
bool b = false;
InterruptIn sw(PB_2);

MemoryPool <amb_l, 10> mp;
Queue  <amb_l, 10> q;
Thread t;

//print mode
void  print_mode(bool b){
    b ? printf("Summer\n") : printf("Winter\n");
}

//print state
void print_state(unsigned short i){
    i == 0 ? printf("Up\n") : i == 1 ? printf("Down\n") : i ==  2 ? printf("Rising\n"): i == 3 ? printf("Lowering\n") : printf("Wrong state\n");
}

//indicate up and down state
void led_state_u_d(bool b){
    DigitalOut led1(LED1);
    DigitalOut led2(LED2);
    b ? led1 = 1 : led2 = 1;
}

//blink led
void blink(DigitalOut led){
    led = !led;
}

//indicate risin and lowering state 
void led_state_r_l(bool b){
    DigitalOut led3(LED3);
    DigitalOut led4(LED4);
    b ? blink(led3) : blink(led4);
}

//led state
void led_state(bool b1, bool b2){
    b1 ? led_state_u_d(b2) : led_state_r_l(b2);
}

//read ambient light
unsigned short read_al(void){
    AnalogIn al_s(A2);
    return al_s.read_u16();
}

void summer(unsigned short al){
    print_mode(true);
    if(al < th){
        print_state(2);
        led_state(false, false);
        ThisThread::sleep_for(3s);
        print_state(0);
        led_state(true, false);
        to_rise = false;
    } else{
        print_state(3);
        led_state(false, true);
        ThisThread::sleep_for(3s);
        print_state(1);
        led_state(true, true);
    }
}

void winter(unsigned short al){
    print_mode(false);
    if(al < th){
        print_state(3);
        led_state(false, true);
        ThisThread::sleep_for(3s);
        print_state(1);
        led_state(true, false);
    } else{
        print_state(2);
        led_state(false, false);
        ThisThread::sleep_for(3s);
        print_state(0);
        led_state(true, true);
    }
}

//send thread
void send_lv(void){
    while(true){
        amb_l *al = mp.try_alloc();
        unsigned short a = read_al();
        //printf("a = %hu\n", a);-> check value read
        al->al = a;
        //printf("al->al = %hu\n", al->al);//check value stored in q
        q.try_put(al);
        ThisThread::sleep_for(8s);
    }
}

//ISR
void sw_f_handler(void){b = true;}

int main(){
    t.start(callback(send_lv));
    while(true){
        sw.mode(PullUp);
        sw.fall(sw_f_handler);

        if(b){
            osEvent evt = q.get();
            if(evt.status == osEventMessage){
                amb_l *al = (amb_l *)evt.value.p;
                unsigned short a = al->al;
                summer(a);
                mp.free(al);
                ThisThread::sleep_for(1s);
            }
        } else{
            osEvent evt = q.get();
            if(evt.status == osEventMessage){
                amb_l *al = (amb_l *)evt.value.p;
                unsigned short a = al->al;
                winter(a);
                mp.free(al);
                ThisThread::sleep_for(1s);
            }
        }
        ThisThread::sleep_for(1s);
    }
}
//TODO stop spamming lowering rising
//TODO fix timings
