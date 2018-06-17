#include <sys/shm.h> //biblioteka od shared memory
#include <sys/types.h>
#include <unistd.h>
#include <limits>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <pigpio.h>
#include <cstdlib>
#include <opencv/cv.h>       
#include <opencv/highgui.h>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>  
#include <time.h>
#include <signal.h>
#include <vector>
#define channel_left 21	
#define channel_right 20
#define szybkosc 120
#include "a.h"
using namespace cv;
using namespace std;
void translacja(float cm);
void rotacja(float kat);
int main(int argc, char** argv){
	 gpio_set();
	 int v=300;
	 cvNamedWindow("v", CV_WINDOW_AUTOSIZE);
	 cvCreateTrackbar("vl", "v", &v, 1000 ,NULL);
	 rback();
	 translacja(70);
	 cvWaitKey(2000);
	 rotacja(180);
	 cvWaitKey(2000);
	 translacja(70);
	 cvWaitKey(2000);
	 rotacja(-180);
	 //while(1) if( (cvWaitKey(1) & 255) == 27 ) break;
     gpioPWM(channel_left,0);
	 gpioPWM(channel_right,0);
	 //gpioTerminate();
     return (EXIT_SUCCESS);  
}
void translacja(float cm)
{
	float srednica_opony=7.8;
	float pi=3.14;
	int impulsy_na_obrot=1496;
	int hamowanie_droga=3;
	int hamowanie_impuls=(hamowanie_droga/(srednica_opony*pi))*impulsy_na_obrot;
	cout<<"hamowanie"<<hamowanie_impuls<<endl;
    int kierunek_lewa=0;
	int kierunek_prawa=0;
	if(cm>(hamowanie_droga+1))
	{
		kierunek_lewa=1;
		kierunek_prawa=1;
		int impuls_przod=((cm/(srednica_opony*pi))*impulsy_na_obrot)-hamowanie_impuls;
		pozycja_zadana_l=Pulse_left+impuls_przod;
		pozycja_zadana_r=Pulse_right+impuls_przod;
		cout<<impuls_przod<<"impuls_przod"<<endl;
	}
	if(cm<-(hamowanie_droga+1))
	{
		kierunek_lewa=0;
		kierunek_prawa=0;
		int impuls_tyl=((cm/(srednica_opony*pi))*impulsy_na_obrot)+hamowanie_impuls;
		pozycja_zadana_l=Pulse_left+impuls_tyl;
		pozycja_zadana_r=Pulse_right+impuls_tyl;
		cout<<impuls_tyl<<"impuls_tyl"<<endl;
	}
	int lewa_ok=0;
	int prawa_ok=0;
	while(!lewa_ok || !prawa_ok)
	{
		if(!lewa_ok)
		{
			if(kierunek_lewa==1)
			{
				lahead();
				if(pozycja_zadana_l<Pulse_left){ lewa_ok=1; lback(); gpioPWM(channel_left,300); cvWaitKey(5);}
				else gpioPWM(channel_left,szybkosc);
			}
			else
			{
				lback();
				if(pozycja_zadana_l>Pulse_left){ lewa_ok=1;  lahead(); gpioPWM(channel_left,300); cvWaitKey(5);}
				else gpioPWM(channel_left,szybkosc);
			} 
			if(lewa_ok)gpioPWM(channel_left,0);
		}
		if(!prawa_ok)
		{
			if(kierunek_prawa==1)
			{
				rahead();
				if(pozycja_zadana_r<Pulse_right){ prawa_ok=1;  rback();gpioPWM(channel_right,1000); cvWaitKey(2); }
				else gpioPWM(channel_right,szybkosc);
			}
			else
			{
				rback();
				if(pozycja_zadana_r>Pulse_right){ prawa_ok=1;  rahead();gpioPWM(channel_right,1000); cvWaitKey(2);}
				else gpioPWM(channel_right,szybkosc);
			}
			if(prawa_ok)gpioPWM(channel_right,0);
		} 	
	}
	gpioPWM(channel_left,0);
	gpioPWM(channel_right,0);
	cvWaitKey(400);
}
void rotacja(float kat)
{
	float cm=(kat/360)*70;
	float srednica_opony=7.8;
	float pi=3.14;
	int impulsy_na_obrot=1496;
	int hamowanie_droga=3;
	int hamowanie_impuls=(hamowanie_droga/(srednica_opony*pi))*impulsy_na_obrot;
	cout<<"HAMOWANIE"<<hamowanie_impuls<<endl;
    int kierunek_lewa=0;
	int kierunek_prawa=0;
	if(cm>(hamowanie_droga+1))
	{
		kierunek_lewa=1;
		kierunek_prawa=0;
		int impuls_przod=((cm/(srednica_opony*pi))*impulsy_na_obrot)-hamowanie_impuls;
		pozycja_zadana_l=Pulse_left+impuls_przod;
		pozycja_zadana_r=Pulse_right-impuls_przod;
		cout<<impuls_przod<<"impuls_przod"<<endl;
	}
	if(cm<-(hamowanie_droga+1))
	{
		kierunek_lewa=0;
		kierunek_prawa=1;
		int impuls_tyl=((cm/(srednica_opony*pi))*impulsy_na_obrot)+hamowanie_impuls;
		pozycja_zadana_l=Pulse_left+impuls_tyl;
		pozycja_zadana_r=Pulse_right-impuls_tyl;
		cout<<impuls_tyl<"impuls_tyl"<<endl;
	}
	int lewa_ok=0;
	int prawa_ok=0;
	while(!lewa_ok || !prawa_ok)
	{
		if(!lewa_ok)
		{
			if(kierunek_lewa==1)
			{
				lahead();
				if(pozycja_zadana_l<Pulse_left){ lewa_ok=1; lback(); gpioPWM(channel_left,szybkosc); cvWaitKey(5);}
				else gpioPWM(channel_left,szybkosc);
			}
			else
			{
				lback();
				if(pozycja_zadana_l>Pulse_left){ lewa_ok=1;  lahead(); gpioPWM(channel_left,szybkosc); cvWaitKey(5);}
				else gpioPWM(channel_left,szybkosc);
			} 
			if(lewa_ok)gpioPWM(channel_left,0);
		}
		if(!prawa_ok)
		{
			if(kierunek_prawa==1)
			{
				rahead();
				if(pozycja_zadana_r<Pulse_right){ prawa_ok=1;  rback();gpioPWM(channel_right,1000); cvWaitKey(2); }
				else gpioPWM(channel_right,szybkosc);
			}
			else
			{
				rback();
				if(pozycja_zadana_r>Pulse_right){ prawa_ok=1;  rahead();gpioPWM(channel_right,1000); cvWaitKey(2);}
				else gpioPWM(channel_right,szybkosc);
			}
			if(prawa_ok)gpioPWM(channel_right,0);
		} 	
	}
	gpioPWM(channel_left,0);
	gpioPWM(channel_right,0);
	cvWaitKey(400);
}
