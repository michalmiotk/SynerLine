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
#define wspx 400
#define wspy 300
#define wielkosc_strzalki 700
#define wielkosc_kola 1500
using namespace cv;
using namespace std;
void* test_stop(void* argument);
int find_contour(Mat image2,int r, int x, int y);
string stop_name = "stop_sign.xml";
CascadeClassifier klasyfikatory[10];
CascadeClassifier klasyfikator_stop;
float param1[8]= { 1.01, 1.2 ,1.4, 1.8, 2.2 , 2.55,2.65 , 3.0 };
int param2[3]= {3 , 2 }; // tablica parametrów wykorzystywanych  w funkcji detectMultiscale
int ilosc_lewo=0;  //ilość wykrytych znaków skrętu w lewo
int ilosc_prawo=0; //ilość wykrytych znaków skrętu w prawo
int stop_licznik=0;
Mat wytnij_obraz(Mat obraz_wejscie, int wsp_x, int wsp_y, int max_r,int *wsp_srx,int *wsp_sry);
void detectFace( struct strukturadlawatku* struktura,float nr, int nrwatku , int kierunek); //nr watku to nr 1 , 2 lub 3 , kierunek 1 dla skretu w prawo , 2 dla skretu w lewo
void gdzie_niebieskikontur(Mat niebieski_range,int* max_r ,int* x ,int*y);
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
};
int main(int argc, char** argv){
	 int upper=110, central=130;
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
	 if( !klasyfikator_stop.load( stop_name ) )        //Ładowanie pliku ze sprawdzeniem poprawnoci 
		{ 
				cout << "Nie znaleziono pliku " << stop_name << "."; 
		}
     VideoCapture cap(0); //  wybrana kamera 0
	 cap>>struktura.obraz;
	 if(!cap.isOpened()){
		  cout<<"no camera detected"<<endl;
	 }
	 for(int i=0;i<1000000;i++) {
			cout<<"klatka"<<i<<endl;
			cap>>struktura.obraz;
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
			for( uint32_t i = 0; i < contours.size(); i++ )
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
				int x,y;
				wytnij_obraz(stop_split[2], center_stop[0].x, center_stop[0].y, maxr_stop,&x,&y).copyTo(stop_split[2]);
				stop_split[2].copyTo(struktura_stop.obraz);
				test_stop((void*)(&struktura_stop));
			}
			//imshow("stop range", stop_range);
			//	resize(struktura.obraz, struktura.obraz, cvSize(wspx,wspy), 0, 0, cv::INTER_CUBIC);
			Mat kolorowy;
			struktura.obraz.copyTo(kolorowy);
			cvtColor(struktura.obraz, struktura.obraz,  CV_BGR2GRAY ); // konwersja przechwyconego obrazu do skali szarości
			Mat niebieski;
			Mat niebieski_range;
			vector<Mat> niebieski_split;
			cvtColor(kolorowy, niebieski,  CV_BGR2HSV );
			split(niebieski, niebieski_split);
			//imshow("original",struktura.obraz);
			vector<Vec3f> circles;
			//cout<<"mieszanie"<<endl;
			//Scalar lower[3]={90,0,0};
			//Scalar upper[3]={110,255,255};
			inRange(niebieski, Scalar(90,100,40),Scalar(110,255,255), niebieski_range);	
			if(pokaz_capture)imshow("capture", niebieski_range);
			Mat element=getStructuringElement(MORPH_RECT, Size(3,3));
			for(int c=0;c<1;c++)dilate(niebieski_range,niebieski_range,element);
			for(int c=0;c<2;c++)erode(niebieski_range,niebieski_range,element);
			if(pokaz_capture)imshow("po filtracji", niebieski_range);
			int max_r=0; //inicjalizacja zmiennej max_r zerem  w celu niespełenia późniejszego warunku if(max_r>0) jeżeli kola nie zostana znalezione  
			int wsp_x , wsp_y; // zmienne położenia największego koła na obrazie
			//cout<<i<<endl; // wyświetla nr. analizowanej klatki
			/*
			HoughCircles( struktura.obraz, circles, HOUGH_GRADIENT, 2 , struktura.obraz.rows/8 , upper, central,0, 0); //najmniejszy promień kola 30 najwiekszy 160, struktura.obraz.rows/10 = 120/10=12 - to najmniejsza odleglosc pomiedzy srodkami kol	
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
			*/
			//circle(struktura.obraz, Point(wsp_x,wsp_y),max_r,255,10,0);
			//imshow("kolo", struktura.obraz);
			//////////////////////////////////////////////////////////
			gdzie_niebieskikontur(niebieski_range,&max_r, &wsp_x, &wsp_y);
			//cout<<"mieszanie3"<<endl;
			//imshow("niebieski_range", niebieski_range);
			//cout<<"maxr"<<max_r<<"wsp_x"<<wsp_x<<"wsp_y"<<wsp_y<<endl;
			if(max_r>25)
			{	
				int liczba=0;
				int wsp_srx=0,wsp_sry=0;
				wytnij_obraz(niebieski_range, wsp_x, wsp_y, max_r,&wsp_srx,&wsp_sry).copyTo(niebieski_range);
				niebieski_range.copyTo(struktura.obraz);
				liczba=find_contour(niebieski_range,max_r,wsp_srx, wsp_sry);
				if(liczba==1)ilosc_prawo=ilosc_prawo+2; 
				if(liczba==3)ilosc_lewo=ilosc_lewo+2; 
			}
		
			if(ilosc_prawo >0 ||ilosc_lewo >0 || stop_licznik>0)
			{
				if(pokaz_wyciecie){imshow("circle",niebieski_range);}//imshow("xx",struktura.obraz);} 
				if(ilosc_prawo>ilosc_lewo){ cout<<"SKRET PRAWO"<<endl;shared_memory1->znak = 3; line(niebieski_range, Point(10,5), Point(15,5), Scalar(255, 255 ,255),   5, 8, 0);};
				if(ilosc_lewo>ilosc_prawo){ cout<<"SKRET LEWO"<<endl;shared_memory1->znak = 1; line(niebieski_range, Point(10,5), Point(15,5), Scalar(0, 0 ,0),   5, 8, 0);};
				if(stop_licznik>0 )
				{
					cout<<"stop"<<endl;
					shared_memory1->znak = 2; 
					line(niebieski_range, Point(15,15), Point(35,35), Scalar(255, 255 ,255),   5, 8, 0);
				}
				//if(ilosc_prawo >1){ shared_memory1->znak = 3; line(struktura.obraz, Point(10,5), Point(15,5), Scalar(255, 255 ,255),   5, 8, 0);};
				stop_licznik=0;
				ilosc_lewo=0;
				ilosc_prawo=0;
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

Mat wytnij_obraz(Mat obraz_wejscie, int wsp_x, int wsp_y, int max_r,int* wsp_srx, int* wsp_sry)
{
			//imshow("obraz_wejscie", obraz_wejscie);
			int x_pocz = wsp_x - max_r; // zadeklarowanie wspolrzednej x początku prostokatu zawierajacego znak
			x_pocz=x_pocz-max_r/4;
			if (x_pocz<0) x_pocz=0;
			*wsp_srx=wsp_x-x_pocz;
			int y_pocz = wsp_y - max_r;  // zadeklarowanie wspolrzednej y początku prostokatu zawierajacego znak
			y_pocz=y_pocz-max_r/4;
			if (y_pocz<0) y_pocz=0; 
			*wsp_sry=wsp_y-y_pocz;
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

int find_contour(Mat image2,int r, int x, int y)
{
	Mat niebieski_range;
	//
	//Mat image=cv::imread("circle2.png",1);
    Mat image1=cv::imread("circle1.png",1);
    Mat imagegray1, imagegray2, imageresult1, imageresult2;
    int thresh=100;
    double ans=10;
    cvtColor(image1, imagegray1,CV_BGR2GRAY);
    //cvtColor(image2,imagegray2,CV_BGR2GRAY);
    image2.copyTo(niebieski_range);
    for(int i=0; i<niebieski_range.rows;i++){
		for(int j=0;j<niebieski_range.cols;j++){
			niebieski_range.at<uchar>(i,j)=255-niebieski_range.at<uchar>(i,j);
		}
	}
	Mat element=getStructuringElement(MORPH_RECT, Size(3,3));
    //imshow("niebieski_range",niebieski_range);
    vector<vector<Point> > contours1, contours2;
    vector<Vec4i>hierarchy1, hierarchy2;
    Canny(imagegray1, imageresult1,thresh, thresh*2);
    Canny(niebieski_range, imageresult2,thresh, thresh*2); //zmienione
    int ilosc_pikseli=15; //uwazac zeby nie przepelnic zmiennej
	//
	int lewo=0, prawo=0;
	int sumal=0;
	int sumap=0;
	//imshow("niebieski_range",niebieski_range);
	Mat kur;
	niebieski_range.copyTo(kur);
	for(int i=0; i<ilosc_pikseli;i++){
		for(int j=0;j<ilosc_pikseli;j++){
			int wx=x-r/2+i-ilosc_pikseli/2;
			int wy=y+r/3+j-ilosc_pikseli/2;
			if((int)niebieski_range.at<uchar>(wx,wy)>100)sumal++;
			cout<<"lewa"<<(int)niebieski_range.at<uchar>(wx,wy)<<endl;
			line(kur, Point(wx,wy), Point(wx,wy), Scalar(100, 100 ,100),   1, 8, 0);
		}
	}
	for(int i=0; i<ilosc_pikseli;i++){
		for(int j=0;j<ilosc_pikseli;j++){
			int wx=x+r/2+i-ilosc_pikseli/2;
			int wy=y+r/3+j-ilosc_pikseli/2;
			if((int)niebieski_range.at<uchar>(wx,wy)>100)sumap++;
			cout<<"prawa"<<(int)niebieski_range.at<uchar>(wx,wy)<<endl;
			line(kur, Point(wx,wy), Point(wx,wy), Scalar(100, 100 ,100),   1, 8, 0);
		}
	}
	//imshow("kur",kur);
	if(sumal<sumap){lewo=1; prawo=0; cout<<"L"<<endl;} else{lewo=0; prawo=1;cout<<"P"<<endl;};
    findContours(imageresult1,contours1,hierarchy1,CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0));
    /*
    for(int i=0;i<contours1.size();i++)
    {
        Scalar color=Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
        drawContours(imageresult1,contours1,i,color,1,8,hierarchy1,0,Point());
    }
	*/
    findContours(imageresult2,contours2,hierarchy2,CV_RETR_TREE,CV_CHAIN_APPROX_SIMPLE,cvPoint(0,0));
    /*
    for(int i=0;i<contours2.size();i++)
    {
        Scalar color=Scalar(rng.uniform(0, 255), rng.uniform(0,255), rng.uniform(0,255));
        drawContours(imageresult2,contours2,i,color,1,8,hierarchy2,0,Point());
    }
    */
	  vector<Moments> mu(contours1.size() );
	 for( size_t i = 0; i < contours1.size(); i++ )
     {
		 mu[i] = moments( contours1[i], false ); 
		// cout<<mu[i].m10/mu[i].m00<<"m1x"<<endl;
	 }
	 
	 vector<Moments> mu2(contours2.size() );
	  for( size_t i = 0; i < contours2.size(); i++ )

     {
		 mu2[i] = moments( contours2[i], false ); 
		 //cout<<mu2[i].m10/mu2[i].m00<<"m2x"<<endl;
	 }
	 double prog_podobienstwa=3;
	 double podobienstwo=9;
    for(uint i=0;i<contours2.size();i++)
    {
        if(mu2[i].m00>wielkosc_strzalki)podobienstwo=matchShapes(contours1[0],contours2[i],CV_CONTOURS_MATCH_I1,0);
        //cout<<"masa"<<mu2[i].m00<<endl;
        if(mu2[i].m00>wielkosc_strzalki){cout<<"podobienstwo"<<podobienstwo<<endl; cout<<"kierunek"<<mu2[i].m10/(r*mu2[i].m00)<<"m2x"<<endl;}
		if(podobienstwo<ans) ans=podobienstwo;
    }
   
    //cout<<"x";
	if(ans<prog_podobienstwa && lewo==1 && prawo==0){ return 3;}
	if(ans<prog_podobienstwa && lewo==0 && prawo==1){ return 1;}
	return -1;
}
void gdzie_niebieskikontur(Mat niebieski_range, int* max_r,int* x, int* y)
{
		vector<Vec4i> hierarchyn;
		Mat niebieski_canny;
		Mat niebieski_invert;
		niebieski_range.copyTo(niebieski_invert);
		vector<vector<Point> > contoursn;
		cout<<"mieszanie2.3"<<endl;
		for(int i=0; i<niebieski_invert.rows;i++){
				for(int j=0;j<niebieski_invert.cols;j++){
					niebieski_invert.at<uchar>(i,j)=255-niebieski_invert.at<uchar>(i,j);
				}
		}
		cout<<"mieszanie3"<<endl;
		
		Canny(niebieski_invert, niebieski_canny,0 ,0, 3);
		findContours(niebieski_canny, contoursn, hierarchyn, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point(0,0) );
		vector<vector<Point> > contoursn_poly( contoursn.size() );
		vector<Point2f>centern( contoursn.size() );
		vector<float>radiusn( contoursn.size() );
		vector<Point2f>center_n(contoursn.size() );
		int maxr_n=0;
		cout<<"xd1-"<<endl;
		if(contoursn.size()>0)
		{
			cout<<"xd3332"<<endl;
				for( uint32_t i = 0; i < contoursn.size(); i++)
				{ 
					if (cv::contourArea(contoursn[i]) > wielkosc_kola)
					{
						cout<<"xd2"<<endl;
						cout<<"kontur ma"<<contourArea(contoursn[i])<<endl;
						approxPolyDP( Mat(contoursn[i]), contoursn_poly[i], 3, true );
						minEnclosingCircle( (Mat)contoursn_poly[i], centern[i], radiusn[i] );
						if(radiusn[i]>maxr_n)
						{ 	
						 maxr_n=radiusn[i];
						 center_n[0]=centern[i];
						}
					}
				}
			*x=center_n[0].x;
			*y=center_n[0].y;
			*max_r=maxr_n;
		}else{
			*x=0;
			*y=0;
			*max_r=0;
		}
		cout<<"gdzie nie bieski"<<endl;
}
     /*
     //cap.set(cv::CAP_PROP_EXPOSURE, 0);
     cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
     cap.set(cv::CAP_PROP_FRAME_HEIGHT,480);
     //cap.set(cv::CAP_PROP_FPS, 90);
     // cap.set(cv::CAP_PROP_ISO_SPEED, 90);
     //cap.set(cv::CAP_PROP_EXPOSURE, 100);
     for(int x;x<200;x++){
		cout<<"fps"<<cap.get(cv::CAP_PROP_FPS)<<endl;
		cout<<"WIDTH"<<cap.get(cv::CAP_PROP_FRAME_WIDTH)<<endl;
		cout<<"EXPOSURE"<<cap.get(cv::CAP_PROP_EXPOSURE)<<endl;
		cout<<"ISO"<<cap.get(cv::CAP_PROP_ISO_SPEED)<<endl;
		cout<<"CODEC"<<cap.get(cv::CAP_PROP_FOURCC)<<endl;
	 }
	 */
