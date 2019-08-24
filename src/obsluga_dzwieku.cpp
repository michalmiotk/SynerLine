#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include <sys/shm.h>		//Used for shared memory
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>                    
#include "opencv2/objdetect/objdetect.hpp" 
#include "opencv2/imgproc/imgproc.hpp" 


using namespace std;
using namespace cv;

//----- SHARED MEMORY -----
struct shared_memory1_struct {
	int some_flag;
	char some_data[1024];
};

void *shared_memory1_pointer = (void *)0;
//VARIABLES:
struct shared_memory1_struct *shared_memory1;
int shared_memory1_id;

int main ()
{
	 //--------------------------------
	//----- CREATE SHARED MEMORY -----
	//--------------------------------
	printf("Creating shared memory...\n");
	shared_memory1_id = shmget((key_t)1234, sizeof(struct shared_memory1_struct), 0777 | IPC_CREAT);		//<<<<< SET THE SHARED MEMORY KEY    (Shared memory key , Size in bytes, Permission flags)
	//	Shared memory key
	//		Unique non zero integer (usually 32 bit).  Needs to avoid clashing with another other processes shared memory (you just have to pick a random value and hope - ftok() can help with this but it still doesn't guarantee to avoid colision)
	//	Permission flags
	//		Operation permissions 	Octal value
	//		Read by user 			00400
	//		Write by user 			00200
	//		Read by group 			00040
	//		Write by group 			00020
	//		Read by others 			00004
	//		Write by others			00002
	//		Examples:
	//			0666 Everyone can read and write

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

	//Assign the shared_memory segment
	shared_memory1 = (struct shared_memory1_struct *)shared_memory1_pointer;
    
		// flaga - nie bardzo wiem po co, ale jest
		// [0] - adresat: LF
		// [9] - adresat: program dzwiekowy
       // wyczysc utworzony sektor pamieci
		shared_memory1->some_flag = 1;
		shared_memory1->some_data[0] = 0;
		shared_memory1->some_data[9] = 0;
	
	
	
	
	cout << "Program uruchomiony..." << endl;
	
	int numer_nagrania=0;
	while (1)
	{
		
		waitKey(50);
		if (shared_memory1->some_data[9] != 0)
		{
			numer_nagrania = shared_memory1->some_data[9];
			shared_memory1->some_data[9] = 0;
		}
		
		
		if (numer_nagrania == 1) 
		{
			system("omxplayer //home/pi/Desktop/znak_skret_w_lewo.m4a");
			numer_nagrania=0;
		}
		
		
		if (numer_nagrania == 2) 
		{
			system("omxplayer //home/pi/Desktop/znak_stop.m4a");
			numer_nagrania=0;
		}
		
		
		if (numer_nagrania == 3) 
		{
			system("omxplayer //home/pi/Desktop/znak_skret_w_prawo.m4a");
			numer_nagrania=0;
		}
		
		
		
		
		
		
		
		
		
		
	}
	
	
	 //--------------------------------
	//----- DETACH SHARED MEMORY -----
	//--------------------------------
	//Detach and delete
	if (shmdt(shared_memory1_pointer) == -1)
	{
		fprintf(stderr, "shmdt failed\n");
		//exit(EXIT_FAILURE);
	}
	if (shmctl(shared_memory1_id, IPC_RMID, 0) == -1)
	{
		fprintf(stderr, "shmctl(IPC_RMID) failed\n");
		//exit(EXIT_FAILURE);
	}
	
	
	return 0;
}
