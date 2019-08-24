#include <math.h>
#include "translacja.h"
#define minus_hist 30
volatile int pop_punkt_sredni;
volatile int pop_pwma;
volatile int pop_pwmb;
volatile int pop_blad;
long int sum_blad=0;
volatile int line_lengsth=10;
volatile int silniki_err_sum=0;
volatile int silniki_err_pop=0;
void detect_vertical(struct text* a, Mat wcolor,  Mat bcolor,Mat out, int* vertical_points, int czesc, int kolumny, int rzedy);
int16_t srednia(int16_t tab[], int16_t line_length);
int pid_silnik_lewy(int predkosc_zadana);
int pid_silnik_prawy(int predkosc_zadana);
#define SECTION 6
int threshold(Mat hsvv);
pthread_mutex_t lock;
struct finding_line{
	int thres;
	int odleglosc_v;
	int odleglosc_h;
	Mat mat_in;
	Mat mat_out;
	int plus_prog;
	int plus_roznica_lewaprawa;
	int odchylenie_standardowe;
};
void* find_vertical(void* argument);
void* find_horizontala(void* argument);
void* find_horizontalb(void* argument);
void* find_vertical_deritative(void* argument);
void* find_horizontal_deritative(void* argument);
void setLabel(cv::Mat& im, const std::string label, std::vector <Point>&contour);
Mat image_processing(void* void_pointer, Mat camera_frame){
		int secs, mics, psecs, pmics;
		gpioTime(PI_TIME_RELATIVE, &psecs, &pmics);		
        struct text* f=(struct text*) void_pointer;
		uint16_t rows=(480*((*f).percent))/100;
		uint16_t columns=(640*((*f).percent))/100;
		//cout<<columns<<"kolumns"<<endl;
		Mat hsv_img_r , color_img;
		vector<Mat> hsv_split;
		//blur(f->fram, f->fram, cv::Size(3,3) ); 
		cv::resize(camera_frame, hsv_img_r, cvSize(columns,rows), 0, 0, cv::INTER_CUBIC);
		Mat hsv_img_o;
		hsv_img_r.copyTo(hsv_img_o);
		/*
		Rect Recred(0,0, columns, 10);	
		Mat red=hsv_img_o(Recred);
		vector<Mat> red3c;
		split(red,red3c);
		imshow("rectout",red3c[2]);
		Mat outred;
		red3c[2].copyTo(outred);
		blur(outred,outred,Size(2,2));
		for(int rows=0; rows<red3c[2].rows; rows++)
		{
				for(int columns=0 ; columns<red3c[2].cols;columns++)
				{
					if(red3c[2].at<uchar>(rows,columns)>czerwony && red3c[0].at<uchar>(rows,columns)>hmin &&red3c[0].at<uchar>(rows,columns)<hmax)outred.at<uchar>(rows,columns)=255;
					else outred.at<uchar>(rows,columns)=0;
				}
		}
		imshow("rec after",outred);
		*/
		
		cvtColor(hsv_img_o,color_img,CV_BGR2HSV); 
        split(color_img,hsv_split);
        /*
        Mat lambda(2, 4 , CV_32FC1);
		lambda = Mat::zeros(hsv_split[2].rows, hsv_split[2].cols ,hsv_img_o.type());
		Point2f inputquad[4];
		//inputquad[0]=Point2f((1*columns/6),0);
		//inputquad[1]=Point2f((5*columns/6),0);
	    inputquad[0]=Point2f(0,0);
		inputquad[1]=Point2f(columns-1,0);
		inputquad[2]=Point2f(columns-1,rows-1);
		inputquad[3]=Point2f(0,rows-1);
		Point2f outputquad[4];
		outputquad[0]=Point2f(0,0);
		outputquad[1]=Point2f(columns-1,0);
		outputquad[2]=Point2f(columns-eye_position,rows-1);
		outputquad[3]=Point2f(eye_position,rows-1);
		lambda = getPerspectiveTransform(inputquad, outputquad);
		warpPerspective(hsv_split[2], hsv_split[2], lambda, hsv_img_o.size());
		*/
        /*
        Mat rozmyty;
        hsv_split[2].copyTo(rozmyty);
	    for(int g=0;g<1;g++)cv::GaussianBlur(rozmyty, rozmyty, cv::Size(0, 0), 3);
	    cv::addWeighted(hsv_split[2], 2, rozmyty, -1, 0, hsv_split[2]);
		*/
       // Mat g(hsv_img_o.rows,hsv_img_o.cols , CV_8UC1);
        //imshow("f",g);
       
        finding_line arg;
        arg.mat_in=hsv_split[2].clone();
        arg.mat_out=hsv_split[2].clone();
        Rect prostokat(0,hsv_split[2].rows/5, hsv_split[2].cols, 2*hsv_split[2].rows/3);
        hsv_split[2]=hsv_split[2](prostokat);
        arg.thres=f->tres;
		if(arg.thres<f->prog_min) arg.thres=f->prog_min;
		//cout<<arg.thres<<endl;
		arg.odleglosc_v=f->small_distant;
		arg.odleglosc_h=f->distant;
		arg.plus_prog=f->plus_prog;
		arg.plus_roznica_lewaprawa=f->plus_roznica_lewaprawa;
		arg.odchylenie_standardowe=f->odchylenie_standardowe;
		int count=0;
		int rc2,rc3, rc4;
		pthread_t  thread2, thread3, thread4;
		if( (rc2=pthread_create( &thread2, NULL, &find_horizontala, (void*)(&arg)) ))
			{
				printf("Thread 2creation failed %d \n", rc2);
			}
		if( (rc3=pthread_create( &thread3, NULL, &find_horizontalb, (void*)(&arg)) ))
			{
				printf("Thread 3creation failed %d \n", rc3);
			}
		if( (rc4=pthread_create( &thread4, NULL, &find_vertical, (void*)(&arg)) ))
			{
				printf("Thread 4creation failed %d \n", rc3);
			}
		pthread_join(thread2, NULL);
		pthread_join(thread3, NULL);
		pthread_join(thread4, NULL); 
		/*
		find_vertical((void*)(&arg));
		find_horizontala((void*)(&arg));
		find_horizontalb((void*)(&arg));
		*/ 
		//find_vertical_deritative((void*)(&arg));
		//find_horizontal_deritative((void*)(&arg));
		float korekcjajasnosci=0;
		int* punkty = new int[rows*2];
		int* katy = new int[SECTION];
		int* mid = new int[SECTION];
		for(int i=0; i<SECTION;i++) katy[i]=-1;
		for(int i=0; i<SECTION;i++) mid[i]=-1;
		int* punkty_ost= new int[rows]; for(int i=0; i<rows; i++) punkty_ost[i]=-3;
	    for(int i=0; i<rows; i++) punkty[i]=-1;
		Rect Rec(hsv_split[2].cols/8,hsv_split[2].rows/8, 3*hsv_split[2].cols/4, 3*hsv_split[2].rows/4);
		Mat bw;
		arg.mat_out.copyTo(bw);
		rectangle(hsv_split[2], Rec, Scalar(255), 1, 8 ,0);
		Mat bigger=bw(Rec);
		//cv::resize(bigger, bigger, cvSize(2*columns,2*rows), 0, 0, cv::INTER_CUBIC);
	    
	    //("WEJŚCIE", arg.mat_in);
	    for(int g=0 ;g<bigger.rows;g++)
	    {
			 for(int h=0 ;h<bigger.cols-1;h++)
			{
				if(bigger.at<uchar>(g,h)<255) bigger.at<uchar>(g,h)=255; else bigger.at<uchar>(g,h)=0;
			}	
		}	
        vector<vector<Point> > contours;
		cv::findContours(bigger.clone(), contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
		int size=contours.size();
		size=abs(size);
		for (int i = 0; i < size; i++)
        {
                if (cv::contourArea(contours[i]) > 25)count++;
				//if(count>2){for(int h=0; h<88;h++) cout<<"SKRZYZOWANIE"<<endl;}
        } 
        //cv::imshow("dst", bigger);
        imshow("b",  bigger);
        //imshow("a",arg.mat_out);
	    klatka(f,rows, columns,  korekcjajasnosci, hsv_split[2],arg.mat_out, punkty, katy, mid);
		int* verticalpoints = new int[columns];
		for(int i=0;i<columns;i++) verticalpoints[i]=-1;
		int ilosc=0, pozycja=0, ilosckat=0;
		for(int i=SECTION-1; i>-1;i--)
		{
			if(mid[i]!=-1)
			{
				pozycja=mid[i];
				ilosc=1;
			}
		}
		if(ilosc) pozycja=pozycja/ilosc;
		else pozycja=0;
		int najwiekszy_kat=0;
		for(int i=0; i<SECTION;i++) if(katy[i]!=-1){ilosckat++;
			if(abs(katy[i])>abs(najwiekszy_kat)) najwiekszy_kat=katy[i];
		}
		//if(ilosckat){ cout<<"kat="<<najwiekszy_kat<<endl;}else{cout<<"nieznaleziono kata"<<endl;}
		if(ilosc){
		    if(pozycja>columns/2) pop_punkt_sredni=1;
		    if(pozycja>0 && pozycja<columns/2) pop_punkt_sredni=0;
		}
		int pwma, pwmb;
		int blad=(columns/2-(pozycja));
		blad=blad/2;
		sum_blad=sum_blad+blad;
		if(sum_blad>10000) sum_blad=10000;
		if(sum_blad<-10000) sum_blad=-10000;
		int rozniczka=blad-pop_blad;
		int wartosc_zadana=f->v*10;
		if(ilosc){
		  pwmb=wartosc_zadana-(f->kp*blad+f->kd*rozniczka+f->ki*sum_blad/1000);//+f->kkat*najwiekszy_kat/90);
		  pwma=wartosc_zadana+(f->kp*blad+f->kd*rozniczka+f->ki*sum_blad/1000);//+f->kkat*najwiekszy_kat/90);
		}else
		{
			
			if(pop_punkt_sredni==1){
					pwmb=f->line_gap*20;
					pwma=-f->line_gap*20;
			}else{
					pwmb=-f->line_gap*20;
					pwma=f->line_gap*20;
			}
		
		}
		int wykryte_skrzyzowanie=0;
		if(count>2)
		{
			cout<<"SKRZYZOWANIE"<<f->nr<<endl;
			wykryte_skrzyzowanie=1;
		}//wykryte_skrzyzowanie=1;
		if(pwma>600) pwma=600;
		if(pwmb>600) pwmb=600;
		if(pwma<-600) pwma=-600;
		if(pwmb<-600) pwmb=-600;
		if(1)
		{	
			if(pwma>0) rahead(); else{ rback(); pwma=-pwma;}
		    if(pwmb>0) lahead(); else{ lback(); pwmb=-pwmb;}
			if(f->jedz && zezwol_jazda_hcsr==1)gpioPWM(channel_right,pwma); else gpioPWM(channel_right,0);
			if(f->jedz && zezwol_jazda_hcsr==1)gpioPWM(channel_left,pwmb); else gpioPWM(channel_left,0);
			//cout<<"pwm lewy"<<pwmb<<endl;
			//cout<<"pwm prawy"<<pwma<<endl;
			//int wypelnienie=pid_silnik_lewy(1000);
			//gpioPWM(channel_a,wypelnienie);
			pop_pwma=pwma;
			pop_pwmb=pwmb;
			pop_blad=blad;
		}
		if(wykryte_skrzyzowanie && wykryto_znak)
		{
			if(wykryto_znak==1)
			{
				//translacja(8);
				rotacja(-100);
				cout<<"rotacja -80"<<endl;
			}
			if(wykryto_znak==2)
			{
				gpioPWM(channel_right,0);
				gpioPWM(channel_left,0);
				cvWaitKey(4000);
			}
			if(wykryto_znak==3)
			{
				//translacja(8);
				rotacja(100);
				cout<<"rotacja 80"<<endl;
			}
			wykryto_znak=0;
		}
		gpioTime(PI_TIME_RELATIVE, &secs, &mics);
		//cout<<"imageprocessing time"<<1/((float)secs-float(psecs)+0.000001*((float)mics-(float)pmics) )<<(float)secs-float(psecs)+0.000001*((float)mics-(float)pmics)<<endl;
		return arg.mat_out;
}

void* find_horizontala(void* argument)
{
	finding_line* arg=(struct finding_line*) argument;
	int16_t prog_histogram = arg->thres;
	int16_t line_length=arg->odleglosc_h;
	for(int rows=1; rows<arg->mat_in.rows/2; rows++)
  {
	  for(int columns=1 ; columns<arg->mat_in.cols-5*line_length;columns++)
	  {
		int16_t srednia_lewa=0;
		int16_t srednia_srodek=0;
		int16_t srednia_prawa=0;
		//int odchylenie_lewa=0;
		//int odchylenie_srodek=0;
		//int odchylenie_prawa=0;
		//int odchyl_lewa=0, odchyl_prawa=0;
		int16_t tab_lewa[50];
		int16_t tab_srodek[50];
		int16_t tab_prawa[50];
		for(int z=0; z<line_length;z++) 
		{
			tab_lewa[z]=arg->mat_in.at<uchar>(rows,columns+z);
		}
		srednia_lewa=srednia(tab_lewa, line_length);
		for(int z=0; z<line_length;z++) 
		{
			tab_srodek[z]=arg->mat_in.at<uchar>(rows,columns+1.5*line_length+z);
		}
		srednia_srodek=srednia(tab_srodek, line_length);
		for(int z=0; z<line_length;z++) 
		{
			tab_prawa[z]=arg->mat_in.at<uchar>(rows,columns+3*line_length+z);
		}
		srednia_prawa=srednia(tab_prawa, line_length);
		/*
		for(int z=0; z<line_length;z++) 
		{
			odchyl_prawa=odchyl_prawa+(int)pow(tab_prawa[z]-odchylenie_prawa,2);
		}
		odchylenie_prawa=odchyl_prawa/line_length;
		tu byl koment
		for(int z=0; z<line_length;z++) 
		{
			odchyl_srodek=odchyl_srodek+(int)pow(tab_prawa[z]-odchylenie_srodek,2);
		}
		odchylenie_srodek=odchyl_srodek/line_length;
		tu byl koment
		for(int z=0; z<line_length;z++) 
		{
			odchyl_lewa=odchyl_lewa+(int)pow(tab_prawa[z]-odchylenie_lewa,2);
		}
		odchylenie_lewa=odchyl_lewa/line_length;
		*/
		if( abs(srednia_lewa-srednia_prawa)<arg->plus_roznica_lewaprawa && (srednia_lewa-srednia_srodek>(prog_histogram+arg->plus_prog-minus_hist)) && (srednia_prawa-srednia_srodek>(prog_histogram+arg->plus_prog-minus_hist)))// && odchylenie_lewa+odchylenie_prawa<arg->odchylenie_standardowe*4000)
		{	
			pthread_mutex_lock(&lock);
			line(arg->mat_out, Point(columns+line_length,rows), Point(columns+2*line_length,rows), Scalar(255, 255 ,255),   5, 8, 0);
			pthread_mutex_unlock(&lock);
		}
	}
  }
  return NULL;
}
void* find_horizontalb(void* argument)
{
	finding_line* arg=(struct finding_line*) argument;
	int16_t prog_histogram = arg->thres;
	int16_t line_length=arg->odleglosc_h;
	for(int rows=(arg->mat_in.rows)/2; rows<arg->mat_in.rows; rows++)
  {
	  for(int columns=1 ; columns<arg->mat_in.cols-5*line_length;columns++)
	  {
		int16_t srednia_lewa=0;
		int16_t srednia_srodek=0;
		int16_t srednia_prawa=0;
		//int odchylenie_lewa=0;
		//int odchylenie_srodek=0;
		//int odchylenie_prawa=0;
		//int odchyl_lewa=0, odchyl_prawa=0;
		int16_t tab_lewa[50];
		int16_t tab_srodek[50];
		int16_t tab_prawa[50];
		for(int z=0; z<line_length;z++) 
		{
			tab_lewa[z]=arg->mat_in.at<uchar>(rows,columns+z);
		}
		srednia_lewa=srednia(tab_lewa, line_length);
		for(int z=0; z<line_length;z++) 
		{
			tab_srodek[z]=arg->mat_in.at<uchar>(rows,columns+1.5*line_length+z);
		}
		srednia_srodek=srednia(tab_srodek, line_length);
		for(int z=0; z<line_length;z++) 
		{
			tab_prawa[z]=arg->mat_in.at<uchar>(rows,columns+3*line_length+z);
		}
		srednia_prawa=srednia(tab_prawa, line_length);
		/*
		for(int z=0; z<line_length;z++) 
		{
			odchyl_prawa=odchyl_prawa+(int)pow(tab_prawa[z]-odchylenie_prawa,2);
		}
		odchylenie_prawa=odchyl_prawa/line_length;
		tu byl koment
		for(int z=0; z<line_length;z++) 
		{
			odchyl_srodek=odchyl_srodek+(int)pow(tab_prawa[z]-odchylenie_srodek,2);
		}
		odchylenie_srodek=odchyl_srodek/line_length;
		tu byl koment
		for(int z=0; z<line_length;z++) 
		{
			odchyl_lewa=odchyl_lewa+(int)pow(tab_prawa[z]-odchylenie_lewa,2);
		}
		odchylenie_lewa=odchyl_lewa/line_length;
		*/
		if( abs(srednia_lewa-srednia_prawa)<arg->plus_roznica_lewaprawa && (srednia_lewa-srednia_srodek>(prog_histogram+arg->plus_prog-minus_hist)) && (srednia_prawa-srednia_srodek>(prog_histogram+arg->plus_prog-minus_hist)))// && odchylenie_lewa+odchylenie_prawa<arg->odchylenie_standardowe*4000)
		{	
			pthread_mutex_lock(&lock);
			line(arg->mat_out, Point(columns+line_length,rows), Point(columns+2*line_length,rows), Scalar(255, 255 ,255),   5, 8, 0);
			pthread_mutex_unlock(&lock);
		}
	}
  }
  return NULL;
}
void* find_vertical(void* argument)
{
	finding_line* arg=(struct finding_line*) argument;
	uint8_t line_length=arg->odleglosc_v;
	uint8_t prog_histogram = arg->thres;
	for(int columns=1; columns<arg->mat_in.cols-1; columns++)
  {
	  for(int rows=1 ; rows<arg->mat_in.rows-5*line_length;rows++)
	  {
		int16_t srednia_lewa=0;
		int16_t srednia_srodek=0;
		int16_t srednia_prawa=0;
		//int odchylenie_lewa=0;
		//int odchylenie_srodek=0;
		//int odchylenie_prawa=0;
		//int odchyl_lewa=0, odchyl_prawa=0;
		int16_t tab_lewa[50];
		int16_t tab_srodek[50];
		int16_t tab_prawa[50];
		for(int16_t z=0; z<line_length;z++) 
		{
			tab_lewa[z]=arg->mat_in.at<uchar>(rows+z,columns);
		}
		srednia_lewa=srednia(tab_lewa, line_length);
		for(int16_t z=0; z<line_length;z++) 
		{
			tab_srodek[z]=arg->mat_in.at<uchar>(rows+1.5*line_length+z,columns);
		}
		srednia_srodek=srednia(tab_srodek, line_length);
		for(int16_t z=0; z<line_length;z++) 
		{
			tab_prawa[z]=arg->mat_in.at<uchar>(rows+3*line_length+z,columns);
		}
		srednia_prawa=srednia(tab_prawa, line_length);
		/*
		for(int z=0; z<line_length;z++) 
		{
			odchyl_prawa=odchyl_prawa+(int)pow(tab_prawa[z]-odchylenie_prawa,2);
		}
		odchylenie_prawa=odchyl_prawa/line_length;
		*/
		/*
		for(int z=0; z<line_length;z++) 
		{
			odchyl_srodek=odchyl_srodek+(int)pow(tab_prawa[z]-odchylenie_srodek,2);
		}
		odchylenie_srodek=odchyl_srodek/line_length;
		*/
		/*
		for(int16_t z=0; z<line_length;z++) 
		{
			odchyl_lewa=odchyl_lewa+(int)pow(tab_prawa[z]-odchylenie_lewa,2);
		}
		odchylenie_lewa=odchyl_lewa/line_length;
		*/
		if( abs(srednia_lewa-srednia_prawa)<arg->plus_roznica_lewaprawa && (srednia_lewa-srednia_srodek>(prog_histogram+arg->plus_prog-minus_hist)) && (srednia_prawa-srednia_srodek>(prog_histogram+arg->plus_prog-minus_hist)))// && odchylenie_lewa+odchylenie_prawa<arg->odchylenie_standardowe*4000)
		{	
			pthread_mutex_lock(&lock);
			if(columns<(arg->mat_in.cols-2*line_length))
																	//+2*line_length
				line(arg->mat_out, Point(columns, rows+2*line_length), Point(columns,rows+2*line_length), Scalar(255,255, 255), 5, 8, 0);
			else
				line(arg->mat_out, Point(columns, rows+2*line_length), Point(columns,rows+2*line_length), Scalar(255,255, 255), 5, 8, 0);
			pthread_mutex_unlock(&lock);
		}
	}
  }
  return NULL;
}
int16_t srednia(int16_t tab[], int16_t line_length){
	int suma=0;
	for(int i=0; i<line_length; i++)
	{
		suma=suma+tab[i];
	}
	return suma/line_length;
}
void setLabel(cv::Mat& im, const std::string label, std::vector <Point>&contour)
{
    int fontface = cv::FONT_HERSHEY_SIMPLEX;
    double scale = 0.4;
    int thickness = 1;
    int baseline = 0;
 
    cv::Size text = cv::getTextSize(label, fontface, scale, thickness, &baseline);
    cv::Rect r = cv::boundingRect(contour);
 
    cv::Point pt(r.x + ((r.width - text.width) / 2), r.y + ((r.height + text.height) / 2));
    cv::rectangle(im, pt + cv::Point(0, baseline), pt + cv::Point(text.width, -text.height), CV_RGB(255,255,255), CV_FILLED);
    cv::putText(im, label, pt, fontface, scale, CV_RGB(0,0,0), thickness, 8);
}
int pid_silnik_lewy(int predkosc_zadana)
{
		int error=predkosc_zadana-v_left_a_r;
		int pwm = predkosc_zadana/3;
		float kp = 0.1;
		float ki=0.01;
		float kd=0.1;
		if(error>100000) error=100000;
		if(error<-100000)error=-100000; 
		silniki_err_sum+=error;
		int pochodna=error-silniki_err_pop;
		int wypelnienie;
		wypelnienie = pwm + kp*error+ki*silniki_err_sum + kd*pochodna; 
		if(wypelnienie>700) wypelnienie = 700;
		if(wypelnienie<0) wypelnienie = 0;
		silniki_err_pop=error;
		return wypelnienie;
}
/*
void* find_horizontal_deritative(void* argument)
{
	finding_line* arg=(struct finding_line*) argument;
	uint8_t min_length=arg->odleglosc_v;
	uint8_t max_length=arg->odleglosc_h;
	uint8_t maxThreshold = arg->thres;
	uint8_t odleglosc= arg->plus_prog;
 for(int rows=0; rows<arg->mat_in.rows; rows++)
  {
    int max[2]={0,0};
    int min[2]={0,0};
	for(int columns=odleglosc; columns<arg->mat_in.cols ; columns++)
	{
		if(arg->mat_in.at<uchar>(rows,columns)-arg->mat_in.at<uchar>(rows,columns-odleglosc)>max[0])
		{
			max[0]=arg->mat_in.at<uchar>(rows,columns)-arg->mat_in.at<uchar>(rows,columns-odleglosc);
			max[1]=columns;
		}
		if(arg->mat_in.at<uchar>(rows,columns)-arg->mat_in.at<uchar>(rows,columns-odleglosc)<min[0])
		{
			min[0]=arg->mat_in.at<uchar>(rows,columns)-arg->mat_in.at<uchar>(rows,columns-odleglosc);
			min[1]=columns;
		}
	}
	if(max[1]-min[1]>min_length && max[1]-min[1]<max_length && max[0]-min[0]>maxThreshold){

			line(arg->mat_out, Point(min[1],rows), Point(max[1],rows), (255,255,255), 5, 8, 0);
	}
  }
  return NULL;
}
void* find_vertical_deritative(void* argument)
{
	finding_line* arg=(struct finding_line*) argument;
	uint8_t min_length=arg->odleglosc_v;
	uint8_t max_length=arg->odleglosc_h;
	uint8_t maxThreshold = arg->thres;
	uint8_t odleglosc= arg->plus_prog;
  for(int columns=0; columns<arg->mat_in.cols; columns++)
  {
    int max[2]={0,0};
    int min[2]={0,0};
	for(int rows=odleglosc; rows<arg->mat_in.rows; rows++)
	{
		if(arg->mat_in.at<uchar>(rows,columns)-arg->mat_in.at<uchar>(rows-odleglosc,columns)>max[0])
		{
			max[0]=arg->mat_in.at<uchar>(rows,columns)-arg->mat_in.at<uchar>(rows-maxThreshold,columns);
			max[1]=rows;
		}
		if(arg->mat_in.at<uchar>(rows,columns)-arg->mat_in.at<uchar>(rows-odleglosc,columns)<min[0])
		{
			min[0]=arg->mat_in.at<uchar>(rows,columns)-arg->mat_in.at<uchar>(rows-maxThreshold,columns);
			min[1]=rows;
		}
	}
	if(max[1]-min[1]>min_length && max[1]-min[1]<max_length && max[0]-min[0]> maxThreshold)
	{

		line(arg->mat_out, Point(columns,min[1]), Point(columns,max[1]), (255,255,255), 5, 8, 0);
	}
  }
  return NULL;
}
*/
