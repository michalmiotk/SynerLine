// ta wersja dziala!
// trzeba podzielic na watki

#include <stdio.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>                    
#include "opencv2/objdetect/objdetect.hpp" 
#include "opencv2/imgproc/imgproc.hpp" 
#include <string> 
#include <iostream> 
#include <sys/shm.h>		//Used for shared memory
#define wspx 320
#define wspy 240
using namespace cv;
using namespace std;

string kaskada_w_prawo_plik = "skret_w_lewo_LBP.xml";    
string kaskada_w_lewo_plik = "skret_w_lewo_LBP.xml";
string kaskada_stop_plik = "stop5.xml";

RNG rng(12345);

void UruchomKaskade(Mat img, CascadeClassifier kaskada, int  & parametr_podobienstwa, int min_sasiedztwo, int max_sasiedztwo, float min_r, float max_r, int min_wymiar, int max_wymiar, int skalowanie_obrazka_x, int skalowanie_obrazka_y);
void UruchomKaskadeStop(Mat img, CascadeClassifier kaskada, int  & parametr_podobienstwa);
void PasekPostepu (Mat & img, int maks, int zapelnienie, int kolor, int wysokosc);
Mat WytnijKolo(Mat obraz, int promien, Point srodek, float proporcja_naddatek_promien);
int OdlegloscPunktow(CvPoint p1, CvPoint p2);
Mat WytnijOsmiokat(Mat obraz, int bok, Point srodek, float wspolczynnik_bok_naddatek);
int SzukajOsmiokatow(Mat img, Point & wynik, int & wynik_dlugosc_boku, Mat & wykryty_osmiokat);

//----- SHARED MEMORY -----
struct shared_memory1_struct {
	int some_flag;
	char some_data[1024];
};

void *shared_memory1_pointer = (void *)0;
//VARIABLES:
struct shared_memory1_struct *shared_memory1;
int shared_memory1_id;
struct kaskada_dane{
	Mat img;
	CascadeClassifier kaskada[6];
	int licznik;
	int min_sasiedztwo;
	int max_sasiedztwo;
	float min_r;
	float max_r; 
	int min_wymiar;
	int max_wymiar;
	int skalowanie_obrazka_x;
	int skalowanie_obrazka_y;
};

int parametr = 10;

int zakoncz=0;

int parametr0 = 55;
int parametr1 = 55;
int parametr2 = 55;

int wstawiany = 10;

int gornagranica = 2;
int dolnagranica = 90;

int a = 240;
int b = 140;

int minpole = 600;
int maxpole = 8000;

int mindl = 60;
int maxdl = 800;




int main()
{
	VideoCapture capture = VideoCapture(0);//Przechwycienie uchwytu kamery o nr. 0 
	Mat frame, img, hsv_img, wycinek, obrocone; //Miejsce na obrazki 
	kaskada_dane lewo_struktura;
	kaskada_dane prawo_struktura;
	kaskada_dane stop_struktura;
	for(int i=0;i<6;i++){
		// ladowanie kaskad do pamieci
		if (!lewo_struktura.kaskada[i].load(kaskada_w_lewo_plik))        //£adowanie pliku ze sprawdzeniem poprawnoci
		{
			cout << "Nie znaleziono pliku " << kaskada_w_lewo_plik << ".";
			return -2;
		}
	
		if (!prawo_struktura.kaskada[i].load(kaskada_w_prawo_plik))        //£adowanie pliku ze sprawdzeniem poprawnoci
		{
			cout << "Nie znaleziono pliku " << kaskada_w_prawo_plik << ".";
			return -2;
		}
	
		if (!stop_struktura.kaskada[i].load(kaskada_stop_plik))        //£adowanie pliku ze sprawdzeniem poprawnoci
		{
			cout << "Nie znaleziono pliku " << kaskada_stop_plik << ".";
			return -2;
		}
	}
	

	int licznik_blokady_wykrywania=0;
	int licznik_blokady_stopu=0;
	int taktowanie_programu = 0;
	int czy_wykryto_kola=0;
	int takty_wykrycia_znaku=0;
	lewo_struktura.licznik=0;
	prawo_struktura.licznik=0;
	stop_struktura.licznik=0;
	namedWindow("Sterowanie", CV_WINDOW_AUTOSIZE);    //Utworzenie 2 okien 
	
	
	//int anuluj_stop=0;
	createTrackbar("Zakoncz", "Sterowanie", &zakoncz, 1, NULL);
	
	
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
	
	int wiadomosc_o_znaku=0;
	
	
	
	//system("/bin/bash -c ./obsluga_dzwieku &");
	while (zakoncz == 0)                    //Odczekanie 20 ms 
	{
		waitKey(50);
		//capture.set(cv::CAP_PROP_FRAME_WIDTH, wspx);
        //capture.set(cv::CAP_PROP_FRAME_HEIGHT, wspy);
		capture >> frame;                    //Pobranie kolejnej klatki 
		frame.copyTo(img); // Skopiowanie klatki do img
		//cvtColor(img, hsv_img, CV_BGR2YCbCr);        //Konwrsja do HSV 
		Size size (180, 135);
		resize(img, img, size);
		
		

		// wykrywanie kolek
		czy_wykryto_kola = 0;
		vector<Vec3f> circles;
				
		Mat obrabiany = img;
		cvtColor(obrabiany, obrabiany, CV_BGR2GRAY);
		GaussianBlur(obrabiany, obrabiany, Size(9, 9), 2, 2);

		// Apply the Hough Transform to find the circles
		HoughCircles(obrabiany, circles, CV_HOUGH_GRADIENT, 2, obrabiany.rows / 8, 80, 105, 0, 0);
		
		
	
		
		if (licznik_blokady_stopu != 0) 
		{
			licznik_blokady_stopu++;
			PasekPostepu(img, 40, licznik_blokady_stopu, 3, 5);
		}
		if (licznik_blokady_stopu == 40) licznik_blokady_stopu=0;
		
		
		
		
		// szukanie osmiokatow ponizej
		int bok = 0;
		int czy_wykryto_osmiokat=0;
		Point ser;
		czy_wykryto_osmiokat = SzukajOsmiokatow(img, ser, bok, stop_struktura.img);
        
        if (( czy_wykryto_osmiokat == 1) && (licznik_blokady_stopu == 0))
        {
        
			UruchomKaskade(stop_struktura.img, stop_struktura.kaskada[0], stop_struktura.licznik, 2, 4, 1.12, 1.15, 20, 70, 70, 70);
			imshow("Wycinek", stop_struktura.img);
        
			if (stop_struktura.licznik > 6)
			{
				wiadomosc_o_znaku=2;
				cout << "Znak stop" << endl;
				stop_struktura.licznik=0;
				licznik_blokady_stopu++;
				
			}
        
        
        
		}
		
		
		if (licznik_blokady_stopu != 0)
		{
			licznik_blokady_stopu++;
			PasekPostepu(img, 120, licznik_blokady_stopu, 3, 4);
			
			
		}
		
		if (licznik_blokady_stopu == 120) licznik_blokady_stopu=0;
	

		int promien = 0;
		Point srodek;

		for (size_t i = 0; i < circles.size(); i++)
		{
			czy_wykryto_kola = 1;
			Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
			int radius = cvRound(circles[i][2]);
			promien = radius;
			srodek = center;

			// Narysuj kola
			//circle(img, center, radius, Scalar(0, 0, 255), 3, 8, 0);
		}
		
		if  ((czy_wykryto_kola == 1) && (takty_wykrycia_znaku == 0) && (licznik_blokady_wykrywania == 0))
		{
				takty_wykrycia_znaku++;
				prawo_struktura.img = WytnijKolo(img, promien, srodek, 0.4);
				imshow("Wycinek", prawo_struktura.img);
				flip(prawo_struktura.img, lewo_struktura.img, 1);
				
				
				// zapusc kaskade
				// zwroc wartosci jakies tam
				// jesli ktoras przekroczy prog, przerwij
				
				// kaskada proboa10, czyli ta aktualnie uzywana najlepiej dziala przy rozmiarze obrazka 60-70
				UruchomKaskade(lewo_struktura.img, prawo_struktura.kaskada[0], prawo_struktura.licznik, 3, 5, 1.20, 1.35, 20, 70, 70, 70);
				UruchomKaskade(prawo_struktura.img, prawo_struktura.kaskada[3],  lewo_struktura.licznik, 3, 5, 1.20, 1.35, 20, 70, 70, 70);
				
				if ((prawo_struktura.licznik > 6) || (lewo_struktura.licznik > 6))
				{
					licznik_blokady_wykrywania++;
					
					if (lewo_struktura.licznik >prawo_struktura.licznik) 
					{
						cout << "Skret w lewo" << endl;
						wiadomosc_o_znaku=1;
					}
					else 
					{
						cout << "Skret w prawo" << endl;
						wiadomosc_o_znaku=3;
					}
					
					
					lewo_struktura.licznik=0;
					prawo_struktura.licznik=0;
					
				}
				
				
			
		}
		
		//cout << "lewo_struktura.licznik: " << lewo_struktura.licznik << "     " << "licznik_skret_w_prawo: " <<  licznik_skret_w_prawo << endl;
		PasekPostepu(img, 6, lewo_struktura.licznik, 1, 1);
		PasekPostepu(img, 6, prawo_struktura.licznik, 2, 2);
		PasekPostepu(img, 6, stop_struktura.licznik, 3, 3);
		
		
		if  ((czy_wykryto_kola == 0) && (takty_wykrycia_znaku == 0))
		{
			cvDestroyWindow("Wycinek");
			
		}
		
		
		if (takty_wykrycia_znaku != 0) 
		{
			takty_wykrycia_znaku++;
			PasekPostepu(img, 5, takty_wykrycia_znaku, 1, 7);
		}
		if (takty_wykrycia_znaku == 5) takty_wykrycia_znaku = 0;
		
		
		
		
		if (licznik_blokady_wykrywania != 0) 
		{
			licznik_blokady_wykrywania ++;
			PasekPostepu(img, 40, licznik_blokady_wykrywania, 3, 6);
		}
		if (licznik_blokady_wykrywania == 40) licznik_blokady_wykrywania = 0;
			
		
		
		
		
		

		//detectFace(img);
		imshow("obrazek", img);            //Obrazek Orginalny 
		
		
		if (wiadomosc_o_znaku != 0)
	{
			if (shared_memory1->some_flag == 1)
		{
			shared_memory1->some_flag = 0;
			shared_memory1->some_data[0] = wiadomosc_o_znaku;
			shared_memory1->some_data[9] = wiadomosc_o_znaku;
			wiadomosc_o_znaku=0;
			shared_memory1->some_flag = 1;
		}
		
		
		
		
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
	capture.release();                        //Zwolnienie uchwytu kamery 
	return 0;
}









void UruchomKaskade(Mat img, CascadeClassifier kaskada, int  & parametr_podobienstwa, int min_sasiedztwo, int max_sasiedztwo, float min_r, float max_r, int min_wymiar, int max_wymiar, int skalowanie_obrazka_x, int skalowanie_obrazka_y)
{
	vector<Rect> wykryte;                           
	Mat img_gray; 
	int ile_trafien=0;                                

	               //Konwersja obrazu do odcieni szarosci 
	Size size(skalowanie_obrazka_x, skalowanie_obrazka_y);
	resize(img, img, size);
	cvtColor(img, img_gray, CV_BGR2GRAY); 
	// nie usuwac, wykomentowywac
	//cout << "Wymiar minimalny: " << min_wymiar << "           Wymiar maksymalny" << max_wymiar << endl;
	
	
	int rozmiary=0;
	
	// tutaj powinna byc kilka razy kaskada odpalona ze zmiennymi parametrami
	
	
	for (rozmiary = min_wymiar; rozmiary<max_wymiar ; rozmiary=rozmiary+5)
	{
	
		for (int a=min_sasiedztwo; a<max_sasiedztwo;a++)
		{
			for (float b=min_r; b<max_r ; b=b+0.01)
			{
					kaskada.detectMultiScale(img_gray, wykryte, b, a, 0 | CV_HAAR_SCALE_IMAGE, Size(rozmiary, rozmiary));
	
				for (unsigned i = 0; i < wykryte.size(); i++)
				{
					//Rect rect_wykryte(wykryte[i]);    //Kwadrat okreslajacy wykryty obiekt
					//rectangle(img, rect_wykryte, Scalar(120, 5, 86), 2, 2, 0);
					ile_trafien++;
				}
			
			
			
			}
	


	
		}
	}
	//imshow("Traifenie", img);                        //Pokazanie obrazka w oknmie o nazwie "Hello Face !" 
	//cout << "Trafiena: " << ile_trafien << endl;
	parametr_podobienstwa = parametr_podobienstwa + ile_trafien;

}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void PasekPostepu (Mat & img, int maks, int zapelnienie, int kolor, int wysokosc)
{
	// wysokosc 1-7
	// kolor 1-3
	
	
	int jedna_jednostka_postepu = img.cols / maks;
	
	if (kolor == 1) line (img, Point(0, (img.rows / 8)*wysokosc), Point (zapelnienie*jedna_jednostka_postepu, (img.rows / 8)*wysokosc), Scalar(250, 0, 0), 5, 4, 0);
	if (kolor == 2) line (img, Point(0, (img.rows / 8)*wysokosc), Point (zapelnienie*jedna_jednostka_postepu, (img.rows / 8)*wysokosc), Scalar(0, 250, 0), 5, 4, 0);
	if (kolor == 3) line (img, Point(0, (img.rows / 8)*wysokosc), Point (zapelnienie*jedna_jednostka_postepu, (img.rows / 8)*wysokosc), Scalar(0, 0, 250), 5, 4, 0);
	
	
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Mat WytnijKolo(Mat obraz, int promien, Point srodek, float proporcja_naddatek_promien)
{
	
	int naddatek = proporcja_naddatek_promien*promien;
	
	Mat dst;
	int a = srodek.x - (promien + naddatek);
	if (a < 0) a = 0;

	int b = srodek.y - (promien + naddatek);
	if (b < 0) b = 0;

	int szer = (promien + naddatek) * 2;
	if ((szer + a) > obraz.cols) szer = obraz.cols - a;

	int wys = (promien + naddatek) * 2;
	if ((wys + b) > obraz.rows) wys = obraz.rows - b;



	//cout << "x: " << a << endl << "y: " << b << endl;
	obraz(Rect(a, b, szer, wys)).copyTo(dst);





	return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UruchomKaskadeStop(Mat img, CascadeClassifier kaskada, int & parametr_podobienstwa)
{
	vector<Rect> wykryte;                           
	Mat img_gray; 
	int ile_trafien=0;                                

	cvtColor(img, img_gray, CV_BGR2GRAY);                //Konwersja obrazu do odcieni szarosci 
	
	float b = 1.1;
	int a = 3;
	
	// tutaj powinna byc kilka razy kaskada odpalona ze zmiennymi parametrami
	
				kaskada.detectMultiScale(img_gray, wykryte, b, a, 0 | CV_HAAR_SCALE_IMAGE, Size(50, 50));
	
			for (unsigned i = 0; i < wykryte.size(); i++)
			{
				//Rect rect_wykryte(wykryte[i]);    //Kwadrat okreslajacy wykryty obiekt
				//rectangle(img, rect_wykryte, Scalar(120, 5, 86), 2, 2, 0);
				ile_trafien++;
			}
	
	

	
	
	//imshow("Traifenie", img);                        //Pokazanie obrazka w oknmie o nazwie "Hello Face !" 
	//cout << "Trafiena: " << ile_trafien << endl;
	parametr_podobienstwa = parametr_podobienstwa + ile_trafien;
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int OdlegloscPunktow (CvPoint p1, CvPoint p2)
{
    int wynik = sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));



    return wynik;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


Mat WytnijOsmiokat(Mat obraz, int bok, Point srodek, float wspolczynnik_bok_naddatek)
{

    // PARAMETR
    int naddatek = bok * wspolczynnik_bok_naddatek;


    Mat dst;
    int a = srodek.x - (bok + naddatek);
    if (a < 0) a = 0;

    int b = srodek.y - (bok + naddatek);
    if (b < 0) b = 0;

    int szer = (bok + naddatek) * 2;
    if ((szer + a) > obraz.cols) szer = obraz.cols - a;

    int wys = (bok + naddatek) * 2;
    if ((wys + b) > obraz.rows) wys = obraz.rows - b;



    //cout << "x: " << a << endl << "y: " << b << endl;
    obraz(Rect(a, b, szer, wys)).copyTo(dst);





    return dst;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int SzukajOsmiokatow(Mat img, Point & wynik, int & wynik_dlugosc_boku, Mat & wykryty_osmiokat)
{

    static int blokada = 0;
    static int trafienie = 0;


    if (blokada != 0) blokada++;
    if (blokada > 5) blokada = 0;
    trafienie--;
    if (trafienie < 0) trafienie = 0;
    if (trafienie > 30) trafienie = 30;

    Mat wycinek;
    Point srodek_osmiokata(0, 0);


    Mat src_gray;
    cvtColor(img, src_gray, CV_BGR2GRAY);

    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;
    Canny(src_gray, src_gray, dolnagranica, gornagranica, 3);
    findContours(src_gray, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

    /// Get the moments
    vector<Moments> mu(contours.size());
    for (int i = 0; i < contours.size(); i++)
    {
        mu[i] = moments(contours[i], false);
    }

    ///  Get the mass centers:
    vector<Point2f> mc(contours.size());
    for (int i = 0; i < contours.size(); i++)
    {
        mc[i] = Point2f(mu[i].m10 / mu[i].m00, mu[i].m01 / mu[i].m00);
    }

    // slaby punkt, poprawic
    volatile int tabrozmiar[800];
    volatile int tabdlugosc[800];

    for (int i = 0; i < 800; i++) tabrozmiar[i] = 0;
    for (int i = 0; i < 800; i++) tabdlugosc[i] = 0;

    int j = 0;
    for (int i = 0; i< contours.size(); i++)
    {
        j++;
        // printf(" * Contour[%d] - Area (M_00) = %.2f - Area OpenCV: %.2f - Length: %.2f \n", i, mu[i].m00, contourArea(contours[i]), arcLength(contours[i], true));

        if ((contourArea(contours[i]) > minpole) && (contourArea(contours[i]) < maxpole))
        {
            tabrozmiar[i] = 1;    // 
            if ((arcLength(contours[i], true) > mindl) && (arcLength(contours[i], true) < maxdl))
            {
                tabdlugosc[i] = 1;    // 
                //printf(" * Contour[%d] - Area (M_00) = %.2f - Area OpenCV: %.2f - Length: %.2f \n", i, mu[i].m00, contourArea(contours[i]), arcLength(contours[i], true));
            }
        }
    }




    /// Draw contours
    Mat drawing = Mat::zeros(src_gray.size(), CV_8UC3);
    for (int i = 0; i< contours.size(); i++)
    {
        if ((tabrozmiar[i] == 1) && (tabdlugosc[i] == 1))
        {
            Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
            drawContours(drawing, contours, i, color, 2, 8, hierarchy, 0, Point());
        }

    }



    cvtColor(drawing, drawing, CV_BGR2GRAY);


    //imshow("Po pierwszej filtracji", drawing);

    //*************************** poczatek


 
   IplImage img_wsk_wip = img;
   IplImage img_canny_wip = drawing;
   
   IplImage* img_wsk = &img_wsk_wip;
   IplImage* img_canny = &img_canny_wip;
   
    IplImage* imgGrayScale = cvCreateImage(cvGetSize(img_wsk), 8, 1);

    cvCvtColor(img_wsk, imgGrayScale, CV_BGR2GRAY);
    cvThreshold(imgGrayScale, imgGrayScale, 128, 255, CV_THRESH_BINARY);


    /*IplImage* img_canny = cvCreateImage(cvGetSize(img_wsk), 8, 1);
    cvCanny(img_wsk, img_canny, 600, 1000, 3);
    */



    //cvNamedWindow("Canny");
    //cvShowImage("Canny", img_canny);




    CvSeq* contoursa;  //hold the pointer to a contour in the memory block
    CvSeq* result;   //hold sequence of points of a contour
    CvMemStorage *storage = cvCreateMemStorage(0); //storage area for all contours


    //finding all contours in the image
    cvFindContours(img_canny, storage, &contoursa, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));



    //iterating through each contour
    while (contoursa)
    {

        //obtain a sequence of points of contour, pointed by the variable 'contour'
        result = cvApproxPoly(contoursa, sizeof(CvContour), storage, CV_POLY_APPROX_DP, cvContourPerimeter(contoursa)*0.02, 0);












        if (result->total == 8)
        {
            //iterating through each point
            CvPoint *pt[8];
            CvPoint ptcenter;
            for (int i = 0; i<8; i++){
                pt[i] = (CvPoint*)cvGetSeqElem(result, i);
            }


            int srednia_dlugosc_boku = 0;
            int nieforemny = 0;
            for (int i = 0; i < 7; i++)
            {
                srednia_dlugosc_boku = srednia_dlugosc_boku + OdlegloscPunktow(*pt[i], *pt[i + 1]);
            }

            srednia_dlugosc_boku = srednia_dlugosc_boku + OdlegloscPunktow(*pt[7], *pt[0]);
            srednia_dlugosc_boku = srednia_dlugosc_boku / 8;
            wynik_dlugosc_boku = srednia_dlugosc_boku;

            //cout << "Srednia dl: " << srednia_dlugosc_boku << endl;
            float paramentr_max = 1.3;
            float parametr_min = 0.7;
            if ((OdlegloscPunktow(*pt[0], *pt[1]) > srednia_dlugosc_boku * paramentr_max) || (OdlegloscPunktow(*pt[0], *pt[1]) < srednia_dlugosc_boku * parametr_min)) nieforemny++;
            if ((OdlegloscPunktow(*pt[1], *pt[2]) > srednia_dlugosc_boku * paramentr_max) || (OdlegloscPunktow(*pt[1], *pt[2]) < srednia_dlugosc_boku * parametr_min)) nieforemny++;
            if ((OdlegloscPunktow(*pt[2], *pt[3]) > srednia_dlugosc_boku * paramentr_max) || (OdlegloscPunktow(*pt[2], *pt[3]) < srednia_dlugosc_boku * parametr_min)) nieforemny++;
            if ((OdlegloscPunktow(*pt[3], *pt[4]) > srednia_dlugosc_boku * paramentr_max) || (OdlegloscPunktow(*pt[3], *pt[4]) < srednia_dlugosc_boku * parametr_min)) nieforemny++;
            if ((OdlegloscPunktow(*pt[4], *pt[5]) > srednia_dlugosc_boku * paramentr_max) || (OdlegloscPunktow(*pt[4], *pt[5]) < srednia_dlugosc_boku * parametr_min)) nieforemny++;
            if ((OdlegloscPunktow(*pt[5], *pt[6]) > srednia_dlugosc_boku * paramentr_max) || (OdlegloscPunktow(*pt[5], *pt[6]) < srednia_dlugosc_boku * parametr_min)) nieforemny++;
            if ((OdlegloscPunktow(*pt[6], *pt[7]) > srednia_dlugosc_boku * paramentr_max) || (OdlegloscPunktow(*pt[6], *pt[7]) < srednia_dlugosc_boku * parametr_min)) nieforemny++;
            if ((OdlegloscPunktow(*pt[7], *pt[0]) > srednia_dlugosc_boku * paramentr_max) || (OdlegloscPunktow(*pt[7], *pt[0]) < srednia_dlugosc_boku * parametr_min)) nieforemny++;

            //cout << "Nieforemnosc: " << nieforemny << endl;

            ptcenter.x = (pt[0]->x + pt[1]->x + pt[2]->x + pt[3]->x + pt[4]->x + pt[5]->x + pt[6]->x + pt[7]->x) / 8;
            ptcenter.y = (pt[0]->y + pt[1]->y + pt[2]->y + pt[3]->y + pt[4]->y + pt[5]->y + pt[6]->y + pt[7]->y) / 8;


            //cvCircle(img_wsk, ptcenter, 10, cvScalar(0, 0, 200), 1, 8, 0);



            

            // jesli wszystkie boki maja mw. ta sama dlugosc
            if (nieforemny == 0) 
            {
                //drawing lines around the heptagon
                /*cvLine(img_wsk, *pt[0], *pt[1], cvScalar(0, 255, 0), 4);
                cvLine(img_wsk, *pt[1], *pt[2], cvScalar(0, 255, 0), 4);
                cvLine(img_wsk, *pt[2], *pt[3], cvScalar(0, 255, 0), 4);
                cvLine(img_wsk, *pt[3], *pt[4], cvScalar(0, 255, 0), 4);
                cvLine(img_wsk, *pt[4], *pt[5], cvScalar(0, 255, 0), 4);
                cvLine(img_wsk, *pt[5], *pt[6], cvScalar(0, 255, 0), 4);
                cvLine(img_wsk, *pt[6], *pt[7], cvScalar(0, 255, 0), 4);
                cvLine(img_wsk, *pt[7], *pt[0], cvScalar(0, 255, 0), 4);*/

                wynik.x = ptcenter.x;
                wynik.y = ptcenter.y;
                

                wycinek = WytnijOsmiokat(img, wynik_dlugosc_boku, wynik, 0.75);

                Size size(180, 180);
                resize(wycinek, wycinek, size);
                Mat wycinekdwa;
                // wykrywanie kol

                cvtColor(wycinek, wycinekdwa, CV_BGR2GRAY);
                vector<Vec3f> circles;
                GaussianBlur(wycinekdwa, wycinekdwa, Size(9, 9), 2, 2);
                /// Apply the Hough Transform to find the circles
                HoughCircles(wycinekdwa, circles, CV_HOUGH_GRADIENT, 2, wycinekdwa.rows / 8, 80, 120, 0, 0);

                int wykryto_kola = 0;
                int promien = 0;
                Point srodek;

                for (size_t i = 0; i < circles.size(); i++)
                {
                    //czy_wykryto_kola = 1;
                    Point center(cvRound(circles[i][0]), cvRound(circles[i][1]));
                    int radius = cvRound(circles[i][2]);
                    promien = radius;
                    srodek = center;


                    // Narysuj kola
                    //circle(wycinekdwa, center, radius, Scalar(0, 0, 255), 3, 8, 0);
                    wykryto_kola = 1;
                    blokada = 1;

                }

                //cout << "Kola: " << wykryto_kola << "     " << "trafienie:  " << trafienie << "    blokada: " << blokada << endl;

                int flaga = 0;
                if (((wykryto_kola == 0) && (blokada == 0)) && (trafienie > 5))
                {
                    wykryty_osmiokat = wycinek;
                    trafienie = trafienie + 5;
                    return 1;
                }

                if ((wykryto_kola == 0) && (blokada == 0))
                {
                    trafienie = trafienie + 5;
                }

                //cout << "zwracane" << flaga << endl;


                return 0;

            }


        }

        //obtain the next contour
        contoursa = contoursa->h_next;

    }

    



    //cvNamedWindow("Po drugiej filtracji");
    //cvShowImage("Po drugiej filtracji", img_wsk);
    //cvWaitKey(10); //wait for a key press

    //cleaning up
    //cvDestroyAllWindows();
    //cvReleaseMemStorage(&storage);
    //cvReleaseImage(&img_wsk);
    //cvReleaseImage(&imgGrayScale);







    return 0;
}

