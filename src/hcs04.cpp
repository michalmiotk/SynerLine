#include <stdio.h>
#include <stdlib.h>
#include <pigpio.h>
#include <iostream>
using namespace std;
#define TRIG 14
#define ECHO 15
 
void setup_hcsr() {
        gpioSetMode(TRIG, PI_OUTPUT);
        gpioSetMode(ECHO, PI_INPUT);
        //TRIG pin must start LOW
        gpioWrite(TRIG, 0);
        gpioDelay(30000);
        cout<<"odczekalem 30us"<<endl;
}
 
int getCM() {
        //Send trig pulse
        gpioWrite(TRIG, 1);
        gpioDelay(10);
        gpioWrite(TRIG, 0);
        //Wait for echo start
        while(gpioRead(ECHO) == 0);
         //Wait for echo end
        uint32_t startTime = gpioTick();
        while(gpioRead(ECHO) == 1);
        int travelTime = gpioTick() - startTime;
 
        //Get distance in cm
        int distance = travelTime / 58;
 
        return distance;
}
 
int main(void) {
		cout<<"inicjalizacja:"<<gpioInitialise()<<endl;
		setup_hcsr();
		while(1)
		{
			printf("Distance: %dcm\n", getCM());
			gpioDelay(300000);
		}
		gpioTerminate();
        return 0;
}
