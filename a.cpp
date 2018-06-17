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
//SHARED MEMORY
struct shared_memory1_struct {
	int znak;
};
void *shared_memory1_pointer = (void *)0;
//VARIABLES:
volatile struct shared_memory1_struct *shared_memory1;
volatile int shared_memory1_id;
//KONIEC SHARED MEMORY
#include "a.h"
#include "imageprocessing.h"
#include "klatka.h"
#include <vector>
using namespace cv;
using namespace std;
void position_check(void);
void detect_vertical(struct text* a, Mat wcolor,  Mat bcolor,Mat out, int* vertical_points, int czesc, int kolumny, int rzedy);
void drawverticalpoints(int part, Mat tab, int* points, int columns, int rows);
void create_windows_trackbars(struct text* a,int* min_canny, int* max_canny, int* 
distant, int* small_distant, int* line_length, int* line_gap, int* kp, 
int* ki, int* kd, int* v);
void klatka(struct text* a, int rzedy, int kolumny, int korekcja, Mat array, Mat 
pochodna, int* punkty, int* katy, int* mid);
void gpio_set(void);
Mat image_processing(void* void_pointer, Mat camera_frame);
void* camera_capture(void* struct_pointer);
void clrscr(void);
void drawpoints(int part, Mat tab, int* points, int rows);
void anglemid(int part,int rows, int columns, int* points, int* angles, int* mid);
int main(int argc, char** argv){
	 cout<<"Creating shared memory...\n"<<endl;
	 shared_memory1_id = shmget((key_t)1234, sizeof(struct shared_memory1_struct), 0666 | IPC_CREAT);		//<<<<< SET THE SHARED MEMORY KEY    (Shared memory key , Size in bytes, Permission flags)
	 if (shared_memory1_id == -1)
	 {
		fprintf(stderr, "Shared memory shmget() failed\n");
		exit(EXIT_FAILURE);
	 }
	 //Make the shared memory accessible to the program
	 shared_memory1_pointer = shmat(shared_memory1_id, (void *)0, 0);
	 if (shared_memory1_pointer == (void *)-1)
	 {
		fprintf(stderr, "Shared memory shmat() failed\n");
		exit(EXIT_FAILURE);
	 }
	 printf("Shared memory attached at %X\n", (int)shared_memory1_pointer);
	 shared_memory1 = (struct shared_memory1_struct *)shared_memory1_pointer;
	 shared_memory1->znak=0;
	 gpio_set();
     FILE *generaloptions;
	 generaloptions=fopen("a.txt", "r");
     	 if(generaloptions == NULL){
		  printf("nie moge otworzyc pliku a.txt");
		  return 22;
	 }
	 struct text nast;
	 fscanf(generaloptions,"%d %d %d %d %d %d %d %f %d %d %d %d %d %d %d %d"
	 ,&nast.jedz, &nast.view ,&nast.percent,&nast.black , 
	 &nast.white,&nast.distant, &nast.small_distant, &nast.koswietlenie, 
	 &nast.min_canny, &nast.max_canny , &nast.line_length, &nast.line_gap, 
	 &nast.prog_min, &nast.plus_prog,
	&nast.plus_roznica_lewaprawa , &nast.odchylenie_standardowe);
	 fclose(generaloptions);
	 printf("skala %d black: %d , white: %d \n distant: %d koswietlenie %f\n ",  nast.percent, nast.black , nast.white, nast.distant, nast.koswietlenie);
  	 FILE *pidoptions;
	 pidoptions=fopen("pd.txt", "r");
     	 if(pidoptions == NULL){
		  printf("nie moge otworzyc pliku pid.txt");
		  return 22;
	 }
	 fscanf(
	 generaloptions,"%d\n%d\n%d\n%d\n ",&nast.kp 
	 ,&nast.kkat,&nast.kd, &nast.v);
	 fclose(generaloptions);
	 create_windows_trackbars(&nast,&nast.min_canny, &nast.max_canny, 
	 &nast.distant, &nast.small_distant, &nast.line_length,  
	 &nast.line_gap,&nast.kp, &nast.kkat, &nast.kd, &nast.v);
     VideoCapture cap(1);
     cap.set(cv::CAP_PROP_FRAME_WIDTH, 320);
     cap.set(cv::CAP_PROP_FRAME_HEIGHT, 240);
     cap.set(cv::CAP_PROP_FPS, 90);
	 struct video main_capture;
	 Mat img;
	 cap>> img;
	 vector<Mat> img_split;
	 cvtColor(img,img,CV_BGR2HSV); 
	 split(img,img_split);
	 //main_capture.frame_zero =img;	
	 main_capture.cap_zero = cap;
	 //nast.prog
	 nast.tres=threshold(img_split[2]);
	 cout<<"prog"<<nast.tres<<endl; //pobranie progu z jednej z pierwszych klatek- robot musi byc na poczatku na linii
	 int minus_prog=nast.tres/3;
	 nast.tres=nast.tres-minus_prog;
	 if(!(main_capture.cap_zero).isOpened()){
		  cout<<"no camera detected"<<endl;
	 }
	 system("/bin/bash -c ./imageclassifier &");
	 //system("/bin/bash -c ./rpisocket &");
	 for(int i=0;i<540000;i++)
	 {
		    //cout<<shared_memory1->some_flag<<"jest wpisana w shared memory"<<endl;
			nast.nr=i;
			camera_capture((void*) &main_capture);
			image_processing((void*)(&nast), main_capture.frame_zero);
			if( (cvWaitKey(1) & 255) == 27 ) break;
			//lback();
			//gpioPWM(channel_left,300);
			//gpioPWM(channel_right,1000);
			//cout<<i<<":frame"<<endl;
		}
    	FILE* generalsave;
    	if((generalsave = fopen("a.txt" , "w") )==NULL)return 10;
        fprintf(generalsave ,"%d %d %d %d %d %d %d %f %d %d %d %d %d %d %d %d",nast.jedz, nast.view,nast.percent ,nast.black , nast.white ,nast.distant, nast.small_distant, nast.koswietlenie, 
        nast.min_canny, nast.max_canny , nast.line_length,  nast.line_gap, nast.prog_min,nast.plus_prog,
	    nast.plus_roznica_lewaprawa , nast.odchylenie_standardowe);
        fclose(generalsave);
        FILE* pidsave;
    	if((pidsave = fopen("pd.txt" , "w") )==NULL)return 10;
        fprintf(pidsave ,"%d\n%d\n%d\n%d",nast.kp,nast.kkat ,nast.kd, 
        nast.v);
        fclose(pidsave);
        gpioPWM(channel_left,0);
		gpioPWM(channel_right,0);
		gpioTerminate();
        return (EXIT_SUCCESS);  
}
