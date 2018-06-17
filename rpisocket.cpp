#include <opencv2/highgui/highgui.hpp> 
#include "opencv2/imgproc/imgproc.hpp" 
#include <string> 
#include <iostream> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <string>
#include <iostream>
#include <sys/shm.h>		//Used for shared memory
 
using namespace cv; 
using namespace std; 

void Wyslij_Obraz (Mat oryginal, int szerokosc_poczatkowa, int szerokosc_koncowa, int wysokosc_poczatkowa, int wysokosc_koncowa, char * tablica_docelowa);
void error(const char *msg)
{
    perror(msg);
    exit(1);
}



//----- SHARED MEMORY -----
struct shared_memory1_struct {
	int znak;
};

void *shared_memory1_pointer = (void *)0;
//VARIABLES:
struct shared_memory1_struct *shared_memory1;
int shared_memory1_id;




int main() 
{ 
    VideoCapture capture = VideoCapture(0);//Przechwycienie uchwytu kamery o nr. 0 
    
    
    
    
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
    
       // wyczysc utworzony sektor pamieci
		shared_memory1->znak = 0;   
    
    // *******************************************************
	// UTWORZ SOCKET
     int sockfd, newsockfd, portno;
     socklen_t clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
  
	  portno = 51718;
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
    
   
    
     listen(sockfd,5);
     clilen = sizeof(cli_addr);
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");
     bzero(buffer,256);
     
     char message[6402];
    // *******************************************************
    
    
     
    
    
	// ********************************************************
	// poczekaj na sygnal inicjalizacji	
		
     n = read(newsockfd,buffer,2);
     if (n < 0) error("ERROR reading from socket");
     printf("Polaczenie nawiazane:: %d\n",buffer[0]);
     
    // *******************************************************
    
    
    
    
    int wiadomosc_o_znaku=0;
    int zakoncz = 0;
    Mat szary;
    Mat przeslany (120, 160, CV_8U);		// X i Y sa zamienione miejscami tutaj
  
    Mat frame, img;
 
 
	 while (1) //zakoncz == 0 )                   
    { 
		
 
	
		waitKey(1);
        capture >> frame;                    //Pobranie kolejnej klatki 
        frame.copyTo(img);                //Skopiowanie klatki do img 
        
        Size size(160,120);//the dst image size,e.g.100x100
		resize(img,img,size);//resize image
  
        cvtColor(img, szary, CV_BGR2GRAY);
        
      
        
        
        
        
        
        // przeslij obraz
		// przesylaj pakiety 160 x 40
		// i trzy takie wyslij


		
	for (int i = 0; i < 3; i++)
	{
		Wyslij_Obraz(szary, 0, 160, (i*40), ((i+1)*40), message);
		message[0] = i; 
		message[6401] = i; 
	
     n = write(newsockfd, message,6402 );
     if (n < 0) error("ERROR writing to socket");
   
     //cout<<"xd"<<endl;
	 n = read(newsockfd,buffer,255);
     if (n < 0) error("ERROR reading from socket");
   
   
	}
	
	
	
		// jesli otrzymano informacje o znaku zapisz ja do oddzielnej zmiennej
		if (buffer[0] != 0) 
		{
			wiadomosc_o_znaku = buffer[0];
			if (wiadomosc_o_znaku == 1) cout << "Nakaz skretu w lewo!" << endl;
			if (wiadomosc_o_znaku == 3) cout << "Nakaz skretu w prawo!" << endl;
		}
		
	// jesli ta zmienna nie jest zerowa sprobj ja wyslac
	if (wiadomosc_o_znaku != 0)
	{
			shared_memory1->znak = wiadomosc_o_znaku;
	}	
		
		
		
        
        
       
       // imshow("nazwa", szary );            //Obrazek Orginalny ewentualny podglad
        
        
        
        
        

    } 
    capture.release();                        //Zwolnienie uchwytu kamery 
     close(newsockfd);
     close(sockfd);
     
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


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Wyslij_Obraz (Mat oryginal, int szerokosc_poczatkowa, int szerokosc_koncowa, int wysokosc_poczatkowa, int wysokosc_koncowa, char * tablica_docelowa)
{
	//int licznik = ((szerokosc_koncowa - szerokosc_poczatkowa) * (wysokosc_koncowa - wysokosc_poczatkowa))+2;
		int licznik=0;
		tablica_docelowa [licznik] = 0;
		licznik++;
		
		for (int y= wysokosc_poczatkowa ; y < wysokosc_koncowa ; y++)
		{
			
			
			for (int x= szerokosc_poczatkowa ; x < szerokosc_koncowa ; x++)
			{
				
				tablica_docelowa[licznik] = oryginal.at<uchar>(y, x);
				licznik++;
				
			}
		
			
			
		}
		
		tablica_docelowa[licznik] = 0;
		//cout << "Nadawane:   pierwsza: " << tablica_docelowa[0] << "    ostatnia: " << tablica_docelowa[licznik] << endl;
	
	
	
	
}


