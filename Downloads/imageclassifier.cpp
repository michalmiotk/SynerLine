#include <sys/shm.h> //biblioteka od shared memory
#include <sys/types.h>
#include <iostream>
#include <pigpio.h>
#include <vector>
#include <opencv/cv.h>       
#include <opencv/highgui.h>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>  
#define wspx 320
#define wspy 240
using namespace cv;
using namespace std;
void* test1(void* argument); //deklaracja jednej z funkcji uruchamianych jako oddzielny wątek
void* test2(void* argument);
void* test3(void* argument);
void* test1_flip(void* argument);
void* test2_flip(void* argument);
void* test3_flip(void* argument);
void* test_stop(void* argument);
string face_cascade_name = "nakaz_p.xml";
string stop_name = "stop_sign.xml";
CascadeClassifier klasyfikatory[10];
CascadeClassifier klasyfikator_stop;
float param1[8]= { 1.01, 1.2 ,1.4, 1.8, 2.2 , 2.55,2.65 , 3.0 };
int param2[3]= {3 , 2 }; // tablica parametrów wykorzystywanych  w funkcji detectMultiscale
int ilosc_lewo=0;  //ilość wykrytych znaków skrętu w lewo
int ilosc_prawo=0; //ilość wykrytych znaków skrętu w prawo
int stop_licznik=0;
Mat wytnij_obraz(Mat obraz_wejscie, int wsp_x, int wsp_y, int max_r);
void detectFace( struct strukturadlawatku* struktura,float nr, int nrwatku , int kierunek); //nr watku to nr 1 , 2 lub 3 , kierunek 1 dla skretu w prawo , 2 dla skretu w lewo
pthread_mutex_t lock;
int pokaz_capture=0;
int pokaz_wyciecie=1;
//SHARED MEMORY
struct shared_memory1_struct {
	int znak;
};
void *shared_memory1_pointer = (void *)0;
//VARIABLES:
struct shared_memory1_struct *shared_memory1;
int shared_memory1_id;
//KONIEC SHARED MEMORY
struct strukturadlawatku
{
	Mat obraz;
	//struct shared_memory1_struct *shared_memory_struct_text; // wskaznik na strukture shared memory
};
int main(int argc, char** argv){
	 int upper=80, central=120;
	 cvNamedWindow("circle", CV_WINDOW_AUTOSIZE); //stworzenie okna o nazwie circle w którym będą wyświetlane suwaki
	 cvCreateTrackbar("upper", "circle", &upper, 150 ,NULL); //suwak modyfikujacy zmienna upper( do wykrywania kola) 
	 cvCreateTrackbar("central", "circle", &central, 150 ,NULL);//suwak modyfikujacy zmienna central( do wykrywania kola) 
	 cvCreateTrackbar("pokaz capture", "circle", &pokaz_capture, 1 ,NULL);
	 cvCreateTrackbar("pokaz wyciecie", "circle", &pokaz_wyciecie, 1 ,NULL);
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
	 cout<<"Shared memory attached at"<<(int)shared_memory1_pointer<<endl;
	 shared_memory1 = (struct shared_memory1_struct *)shared_memory1_pointer;
	 struct strukturadlawatku struktura;
	 struct strukturadlawatku struktura_stop; 
	 struct strukturadlawatku struktura_flip; //struktura zawierajaca obrocony(lustrzane odbicie) obraz 
	 //struktura.shared_memory_struct_text=(struct shared_memory1_struct *)shared_memory1_pointer;
	 string face_cascade_name = "nakaz_p.xml"; 
	 for(int b=0; b<10; b++) // załadowanie klasyfikatorów w pętli - każdy wątek(thread) musi mieć osobny klasyfikator
	 {
		if( !klasyfikatory[b].load( face_cascade_name ) )        //Ładowanie pliku ze sprawdzeniem poprawnoci 
		{ 
				cout << "Nie znaleziono pliku " << face_cascade_name << "."; 
		}	
	 }
	 if( !klasyfikator_stop.load( stop_name ) )        //Ładowanie pliku ze sprawdzeniem poprawnoci 
		{ 
				cout << "Nie znaleziono pliku " << stop_name << "."; 
		}
     VideoCapture cap(0); //  wybrana kamera 0
     /*
     cap.set(cv::CAP_PROP_EXPOSURE, 0);
     cap.set(cv::CAP_PROP_FRAME_WIDTH, wspx);
     cap.set(cv::CAP_PROP_FRAME_HEIGHT, wspy);
     cap.set(cv::CAP_PROP_FPS, 90);
      cap.set(cv::CAP_PROP_ISO_SPEED, 90);
     //cap.set(cv::CAP_PROP_EXPOSURE, 100);
     for(int x;x<200;x++){
		cout<<"fps"<<cap.get(cv::CAP_PROP_FPS)<<endl;
		cout<<"WIDTH"<<cap.get(cv::CAP_PROP_FRAME_WIDTH)<<endl;
		cout<<"EXPOSURE"<<cap.get(cv::CAP_PROP_EXPOSURE)<<endl;
		cout<<"ISO"<<cap.get(cv::CAP_PROP_ISO_SPEED)<<endl;
		cout<<"CODEC"<<cap.get(cv::CAP_PROP_FOURCC)<<endl;
	 }
	 */
	 cap>>struktura.obraz;
	 if(!cap.isOpened()){
		  cout<<"no camera detected"<<endl;
	 }
	 for(int i=0;i<1000000;i++) {
			cap >> struktura.obraz;
			Size size(wspx, wspy);
			resize(struktura.obraz, struktura.obraz, size);
			Mat stop;
			vector<Mat> stop_split;
			cvtColor(struktura.obraz, stop,  CV_BGR2HSV );
			split(stop, stop_split);
			Mat stop_range;
			inRange(stop_split[0], 0, 40, stop_range);
			vector<vector<Point> > contours;
			vector<Vec4i> hierarchy;
			Mat stop_canny;
			if(pokaz_capture)imshow("capture", struktura.obraz);
			Canny(stop_range, stop_canny,0 ,0, 3);
			//imshow("cont" , stop_canny);
			findContours(stop_canny, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0) );
			Mat drawing(Mat::zeros(stop_canny.size(), CV_8UC3));
			vector<vector<Point> > contours_poly( contours.size() );
			vector<Point2f>center( contours.size() );
			vector<float>radius( contours.size() );
			RNG rng(12345);
			vector<Point2f>center_stop(contours.size() );;
			int maxr_stop=0;
			for( int i = 0; i < contours.size(); i++ )
			{ 
				if (cv::contourArea(contours[i]) > 400)
				{
					approxPolyDP( Mat(contours[i]), contours_poly[i], 3, true );
					minEnclosingCircle( (Mat)contours_poly[i], center[i], radius[i] );
					Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0, 255));
					drawContours(drawing , contours_poly, i , color, 1, 8, vector<Vec4i>() ,0 , Point() );
					circle(drawing, center[i] , (int) radius[i], color ,2 ,8 ,0 );
					if(radius[i]>maxr_stop)
					{ 	
						 maxr_stop=radius[i];
						 center_stop[0]=center[i];
					}
				}
 			}
 			//imshow("countours", drawing);
 			if(maxr_stop)
 			{
				wytnij_obraz(stop_split[2], center_stop[0].x, center_stop[0].y, maxr_stop).copyTo(stop_split[2]);
				stop_split[2].copyTo(struktura_stop.obraz);
				test_stop((void*)(&struktura_stop));
			}
			//imshow("stop range", stop_range);
			//	resize(struktura.obraz, struktura.obraz, cvSize(wspx,wspy), 0, 0, cv::INTER_CUBIC);
			cvtColor(struktura.obraz, struktura.obraz,  CV_BGR2GRAY ); // konwersja przechwyconego obrazu do skali szarości
			//imshow("original",struktura.obraz);
			vector<Vec3f> circles;
			HoughCircles(struktura.obraz, circles, HOUGH_GRADIENT, 2 , struktura.obraz.rows/8 , upper, central,0, 0); //najmniejszy promień kola 30 najwiekszy 160, struktura.obraz.rows/10 = 120/10=12 - to najmniejsza odleglosc pomiedzy srodkami kol
			int max_r=0; //inicjalizacja zmiennej max_r zerem  w celu niespełenia późniejszego warunku if(max_r>0) jeżeli kola nie zostana znalezione  
			int wsp_x , wsp_y; // zmienne położenia największego koła na obrazie
			//cout<<i<<endl; // wyświetla nr. analizowanej klatki
			for(size_t i=0; i< circles.size(); i++)
			{
				Vec3i c=circles[i];
				if(c[2] > max_r) // sortowanie w celu znalezienia największego kola
				{ 
					max_r=c[2];
					wsp_x=c[0];
					wsp_y=c[1];
				}				
			}
			if(max_r>55)
			{	
				wytnij_obraz(struktura.obraz, wsp_x, wsp_y, max_r).copyTo(struktura.obraz);
				struktura.obraz.copyTo(struktura_stop.obraz);
				//cv::resize(struktura.obraz, struktura.obraz, cvSize(160,120), 0, 0, cv::INTER_CUBIC);
				flip(struktura.obraz, struktura_flip.obraz ,1); //stworzenie lustrzanego odbicia
				int rc0, rc1, rc2,rc3, rc6, rc7, rc8;
				pthread_t thread0, thread1,thread2, thread3,thread6,thread7,thread8;
				/*
				if( (rc0=pthread_create( &thread0, NULL, &test_stop, (void*)(&struktura_stop)) ))
				{
					cout<<"Thread 0creation failed"<<rc0<<endl;
				}
				*/
				if( (rc1=pthread_create( &thread1, NULL, &test1, (void*)(&struktura)) ))
				{
					cout<<"Thread 1creation failed"<<rc1<<endl;
				}
				
				if( (rc2=pthread_create( &thread2, NULL, &test2, (void*)(&struktura)) ))
				{
					cout<<"Thread 2creation failed"<<rc2<<endl;
				}
				
				//if( (rc3=pthread_create( &thread3, NULL, &test3, (void*)(&struktura)) ))
				//{
				//	cout<<"Thread 3creation failed"<<rc3<<endl;
				//}	
				
				if( (rc6=pthread_create( &thread6, NULL, &test1_flip, (void*)(&struktura_flip)) ))
				{
					cout<<"Thread 6creation failed"<<rc6<<endl;
				}
				
				if( (rc7=pthread_create( &thread7, NULL, &test2_flip, (void*)(&struktura_flip)) ))
				{
					cout<<"Thread 7creation failed"<<rc7<<endl;
				}
				
				//if( (rc8=pthread_create( &thread8, NULL, &test3_flip, (void*)(&struktura_flip)) ))
				//{
				//	cout<<"Thread 8creation failed"<<rc8<<endl;
				//}
				//pthread_join(thread0, NULL);
				pthread_join(thread1, NULL);
				pthread_join(thread2, NULL);
				//pthread_join(thread3, NULL);
				
				pthread_join(thread6, NULL);
				pthread_join(thread7, NULL); 
				//pthread_join(thread8, NULL);
				
			if(ilosc_prawo >0 ||ilosc_lewo >0 || stop_licznik>0)
			{
					if(ilosc_prawo>ilosc_lewo){ cout<<"SKRET PRAWO"<<endl;shared_memory1->znak = 3; line(struktura.obraz, Point(10,5), Point(15,5), Scalar(255, 255 ,255),   5, 8, 0);};
					if(ilosc_lewo>ilosc_prawo){ cout<<"SKRET LEWO"<<endl;shared_memory1->znak = 1; line(struktura.obraz, Point(10,5), Point(15,5), Scalar(0, 0 ,0),   5, 8, 0);};
				
				if(stop_licznik>0 )
				{
					cout<<"stop"<<endl;
					shared_memory1->znak = 2; 
					line(struktura.obraz, Point(15,15), Point(35,35), Scalar(255, 255 ,255),   5, 8, 0);};
				}
				//if(ilosc_prawo >1){ shared_memory1->znak = 3; line(struktura.obraz, Point(10,5), Point(15,5), Scalar(255, 255 ,255),   5, 8, 0);};
				stop_licznik=0;
				ilosc_lewo=0;
				ilosc_prawo=0;
				if(pokaz_wyciecie)imshow("circle",struktura.obraz);
			}
			if( (cvWaitKey(33) & 255) == 27 ) break;
		}
		return (EXIT_SUCCESS);  
}	
void detectFace( struct strukturadlawatku* struktura, int nrface , int kierunek) 
{ 	
	//(*struktura).shared_memory_struct_text->some_flag=7;
	double wspolczynnik=1.05;
	//double kolumny=((double)(struktura->obraz).cols);
	//wspolczynnik=kolumny/160.01;
    if(wspolczynnik<1.01) wspolczynnik=1.01;
    //cout<<wspolczynnik<<"wspolcynnik"<<(struktura->obraz).cols<<"cols"<<endl;
    vector<Rect> faces[8];   
	for(int i=0; i<2; i++) //3 przejscia petli bo 3 wartosci w tablicy param2
	{
		if(nrface == i+1)
		{
				for(int j=i; j<8; j=j+2)//8 przejść petli bo 8 wartosci w tablicy param1
				{
					if(kierunek==1 )
					{ 
						klasyfikatory[i].detectMultiScale(struktura->obraz, faces[j], param1[j]*wspolczynnik, param2[0], 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) ); 
						//cout<<"nr tablicy"<<i<<endl;
						for(unsigned k = 0 ; k<faces[j].size(); k++)
						{
							ilosc_prawo++;
						}	
					}
					if(kierunek==2 )
					{
						klasyfikatory[i+5].detectMultiScale(struktura->obraz, faces[j], param1[j]*wspolczynnik, param2[0], 0|CV_HAAR_SCALE_IMAGE, Size(30, 30) ); 
						for(unsigned k = 0 ; k<faces[j].size(); k++)
						{
							ilosc_lewo++;
						}	
					}
			    }
		}
	}	
}
void* test_stop(void* argument)
{
	vector<Rect> stops;   
	klasyfikator_stop.detectMultiScale(((struct strukturadlawatku*)argument)->obraz, stops, 1.01, 3, 0|CV_HAAR_SCALE_IMAGE, Size(20, 20) );  
	for(unsigned k = 0 ; k<stops.size(); k++)
	{
			stop_licznik++;
			cout<<"stoop"<<endl;
	}
	vector<Rect> stops2;   
	klasyfikator_stop.detectMultiScale(((struct strukturadlawatku*)argument)->obraz, stops2, 1.01, 3, 0|CV_HAAR_SCALE_IMAGE, Size(50, 50) );  
	for(unsigned k = 0 ; k<stops.size(); k++)
	{
			stop_licznik++;
			cout<<"stoopduzy"<<endl;
	}
	return NULL;
}
void* test1(void* argument)
{
	detectFace(((struct strukturadlawatku*)(argument)), 1 ,1);
	return NULL;
}
void* test2(void* argument)
{
	detectFace(((struct strukturadlawatku*)(argument)), 2,1);
	return NULL;
}
void* test3(void* argument)
{
	detectFace(((struct strukturadlawatku*)(argument)), 3, 1);
	return NULL;
}
void* test1_flip(void* argument)
{
	detectFace(((struct strukturadlawatku*)(argument)), 1, 2 );
	return NULL;
}
void* test2_flip(void* argument)
{
	detectFace(((struct strukturadlawatku*)(argument)), 2, 2);
	return NULL;
}
void* test3_flip(void* argument)
{
	detectFace(((struct strukturadlawatku*)(argument)), 3, 2);
	return NULL;
}
Mat wytnij_obraz(Mat obraz_wejscie, int wsp_x, int wsp_y, int max_r)
{
			//imshow("obraz_wejscie", obraz_wejscie);
			int x_pocz = wsp_x - max_r; // zadeklarowanie wspolrzednej x początku prostokatu zawierajacego znak
			x_pocz=x_pocz-max_r/4;
			if (x_pocz<0) x_pocz=0;
			int y_pocz = wsp_y - max_r;  // zadeklarowanie wspolrzednej y początku prostokatu zawierajacego znak
			y_pocz=y_pocz-max_r/4;
			if (y_pocz<0) y_pocz=0; 
			int width = 2*max_r;
			width=width+max_r/2;
			if(x_pocz + width > obraz_wejscie.cols) width = obraz_wejscie.cols- x_pocz; //jesli prostokat wystaje poza obraz to zmniejsz jego wymiary
			int height =  2*max_r;
			height=height+max_r/2;
			if(y_pocz + height > obraz_wejscie.rows) height = obraz_wejscie.rows- y_pocz;
			Rect wycinanie(x_pocz, y_pocz , width , height); // stworz prostokat o podanych 
			Mat prostokat = obraz_wejscie(wycinanie);
			return prostokat;
}
