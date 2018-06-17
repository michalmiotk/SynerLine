#define channel_left 21	
#define channel_right 20
#define encoder_left_a 11
#define encoder_left_b 9
#define encoder_right_a 10
#define encoder_right_b 22
#define TRIG 14
#define ECHO 15
#define PULSPREV 555
#define ilosc_pix 100
#include <math.h>
volatile long sumlwheel=0;
volatile long sumrwheel=0;
volatile int poplwheel=0;
volatile int poprwheel=0;
volatile long int obrrwheel=0;
volatile long int obrlwheel=0;
volatile int wykryto_znak=0;
volatile int licznik_czasu_znak=0;
volatile int vr;
volatile int vl;
volatile int distance_hcsr1;
using namespace cv;
using namespace std;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int counter=0;
int secshot, micshot;
float zliczacz=0;
int ilosc_zliczen=0;
void klatka(struct text* a, int rzedy, int kolumny, int korekcja, Mat array, Mat pochodna, int* punkty, int* katy, int* mid);
void clrscr(void);
volatile int Pulse_left=0;
volatile int Pulse_right=0;
uint32_t prev_time_left_a_r=0;
uint32_t prev_time_left_b_r=0;
uint32_t prev_time_left_a_f=0;
uint32_t prev_time_left_b_f=0;
uint32_t prev_time_right_a_r=0;
uint32_t prev_time_right_b_r=0;
uint32_t prev_time_right_a_f=0;
uint32_t prev_time_right_b_f=0;
volatile int v_left_a_r=0;
volatile int v_left_b_r=0;
volatile int v_left_a_f=0;
volatile int v_left_b_f=0;
volatile int v_right_a_r=0;
volatile int v_right_b_r=0;
volatile int v_right_a_f=0;
volatile int v_right_b_f=0;
volatile int pop_pin_left=0;
volatile int pop_stan_left=0;
volatile int pop_pin_right=0;
volatile int pop_stan_right=0;
volatile int kier_left=1;
volatile int kier_right=1;
volatile int pozycja_zadana_l=0;
volatile int pozycja_zadana_r=0;
volatile int prawe_stop=0;
volatile int lewe_stop=0;
int threshold(Mat hsvv);
void encoderPulseright(int gpio, int lev, uint32_t tick);
void encoderPulseleft(int gpio, int lev, uint32_t tick);
void sign_check(void);
void setup_hcsr();
void getCM();
void echo_alert(int gpio, int lev, uint32_t tick);
void encoder_set(void);
volatile uint32_t echo_start;
volatile int zbocze_narastajace=1;
volatile int zbocze_opadajace=1;
volatile int zezwol_jazda_hcsr=1;
volatile int licznik_przeszkod=0;
volatile int obrocono=1;
volatile int przejechano=1;
int czerwony=150;
int hmin=10;
int hmax=150;
int eye_position=10;
struct text{
	Mat fram;
	Mat fram2;
	int tres;
	int view, percent, black, white, distant, small_distant,  min_canny, max_canny, line_length, line_gap, prog_min;
	int plus_prog;
	int plus_roznica_lewaprawa;
	int odchylenie_standardowe;
	float koswietlenie;
	int kp, kd, kkat,v;
    int jedz;
    int nr;
};
struct video{
	VideoCapture cap_zero;
	Mat frame_zero;
	Mat frame_one;
};
void czas_funkcji(int* psekundy, int* pmikrosekundy, char* napis){
		int secs, mics;
		gpioTime(PI_TIME_RELATIVE, &secs, &mics);
		cout<<"......czas"<<napis<<1/((float)secs-float(*psekundy)+0.000001*((float)mics-(float)*pmikrosekundy) )<<(float)secs-float(*psekundy)+0.000001*((float)mics-(float)*pmikrosekundy)<<endl;
		zliczacz = zliczacz+((float)secs-float(*psekundy)+0.000001*((float)mics-(float)*pmikrosekundy) );	
		ilosc_zliczen++;
		if(ilosc_zliczen>20){
			 ilosc_zliczen=0;
			 cout<<"###############################"<<endl;
			 cout<<"###############################"<<endl;
			 cout<<"#20 klatek trwa"<<zliczacz<<endl;
			 cout<<"#fpsrednie:"<<1/(zliczacz/20)<<endl;
			 cout<<"###############################"<<endl;
			 cout<<"###############################"<<endl;
			 zliczacz=0;
			 ilosc_zliczen=0;
		}	 
		*psekundy=secs;
		*pmikrosekundy=mics;
}
void gpio_set(void){
	 gpioInitialise();
	 encoder_set();
	 gpioSetMode(13, PI_OUTPUT); //AIN2
	 gpioSetMode(6, PI_OUTPUT);  //AIN1
	 gpioSetMode(19, PI_OUTPUT); //BIN1
	 gpioSetMode(26, PI_OUTPUT);  //BIN2
	 gpioSetMode(channel_left, PI_OUTPUT);  
	 gpioSetMode(channel_right, PI_OUTPUT);  
	 gpioSetPWMrange(channel_left,1000);
	 gpioSetPWMrange(channel_right,1000);
	 gpioSetPWMfrequency(channel_left,500);
	 gpioSetPWMfrequency(channel_right,500);
	 gpioPWM(channel_left,0);
	 gpioPWM(channel_right,0);
	 gpioSetTimerFunc(0,10,sign_check);
	 setup_hcsr();
}
void encoder_set(void)
{
	gpioSetMode(encoder_left_a,PI_INPUT);
	gpioSetMode(encoder_left_b,PI_INPUT);
	gpioSetMode(encoder_right_a,PI_INPUT);
	gpioSetMode(encoder_right_b,PI_INPUT);
	gpioSetPullUpDown(encoder_left_a, PI_PUD_UP);
	gpioSetPullUpDown(encoder_left_b, PI_PUD_UP);
	gpioSetPullUpDown(encoder_right_a, PI_PUD_UP);
	gpioSetPullUpDown(encoder_right_b, PI_PUD_UP);
	gpioSetAlertFunc(encoder_left_a, encoderPulseright);
	gpioSetAlertFunc(encoder_left_b, encoderPulseright);
	gpioSetAlertFunc(encoder_right_a, encoderPulseleft);
	gpioSetAlertFunc(encoder_right_b, encoderPulseleft);
}
void create_windows_trackbars(struct text* a, int* min_canny, int* max_canny, int* distant, int* small_distant, int* line_length, int* line_gap, int* kp, int* ki, int* kd, int* v){
	 cvNamedWindow("pd", 1);
	 //cvNamedWindow("birdeye", 1);
	 cvCreateTrackbar("eyeposition", "pd", &eye_position, 80 ,NULL);
	 cvCreateTrackbar("dthoriz", "pd", distant, 20 ,NULL);
	 cvCreateTrackbar("dtvert", "pd", small_distant, 20 ,NULL);
	 //cvCreateTrackbar("prog_min", "pd", &(a->prog_min), 90 ,NULL);
	 cvCreateTrackbar("pprog", "pd", &(a->plus_prog),80 ,NULL);
	 cvCreateTrackbar("rlewaprawa", "pd", &(a->plus_roznica_lewaprawa),80 ,NULL);
	 cvCreateTrackbar("os", "pd", &(a->odchylenie_standardowe),190 ,NULL);
	 cvCreateTrackbar("back", "pd", line_gap, 50 ,NULL);
	 cvCreateTrackbar("kp", "pd", kp,40 ,NULL);
	 cvCreateTrackbar("kd", "pd", kd, 500 ,NULL);
	 cvCreateTrackbar("v", "pd", v, 100 ,NULL);
	 //cvCreateTrackbar("red", "pd", &czerwony, 250 ,NULL);
	 //cvCreateTrackbar("hmin", "pd", &hmin, 250 ,NULL);
	 //cvCreateTrackbar("hmax", "pd", &hmax, 250 ,NULL);
	 cvCreateTrackbar("jazda", "pd", &(a->jedz),1 ,NULL);
}
void* camera_capture(void* struct_pointer){
	int secs, mics, psecs, pmics;
    gpioTime(PI_TIME_RELATIVE, &psecs, &pmics);	
	struct video* capturing = (struct video*) struct_pointer;
	(capturing->cap_zero ) >> (capturing->frame_zero);
	gpioTime(PI_TIME_RELATIVE, &secs, &mics);
    //cout<<"capture time"<<(float)secs-float(psecs)+0.000001*((float)mics-(float)pmics)<<endl;
    return NULL;
}
void clrscr(void){
	cout<<"\033[2J"<<"\033[0;0f";
}
void rahead(void){
	gpioWrite(6,0);
	gpioWrite(13,1);
}
void rback(void){
	gpioWrite(6,1);
	gpioWrite(13,0);
}
void lahead(void){
	 gpioWrite(26,0);
	 gpioWrite(19,1);
}
void lback(void){
	 gpioWrite(26,1);
	 gpioWrite(19,0);
}
void encoderPulseright(int gpio, int lev, uint32_t tick)
{
	if(gpio == 9){
		if(lev==1)
		{
			v_right_a_r= 1000000/(tick-prev_time_right_a_r);
			prev_time_right_a_r=tick;
			//cout<<"v_left_a_r"<<v_left_a_r<<endl;
			if(pop_pin_right==11 && pop_stan_right== 1 ){ kier_right = 0; };
			if(pop_pin_right==11 && pop_stan_right== 0 ){ kier_right = 1; };
			pop_stan_right=1;
		}
		if(lev==0)
		{
			//Pulse_left_a++;
			v_right_a_f= 1000000/(tick-prev_time_right_a_f);
			prev_time_right_a_f=tick;
			//cout<<"v_left_a_f"<<v_left_a_f<<endl;
			if(pop_pin_right==11 && pop_stan_right== 0 ) {kier_right = 0; };
			if(pop_pin_right==11 && pop_stan_right== 1 ) {kier_right = 1; }
			pop_stan_right=0;
		}
		pop_pin_right=9;
	}
	if(gpio == 11){
		if(lev==1)
		{
			v_right_b_r= 1000000/(tick-prev_time_right_b_r);
			prev_time_right_b_r=tick;
			if(pop_pin_right==9 && pop_stan_right== 1 ) kier_right = 1;
			if(pop_pin_right==9 && pop_stan_right== 0 ) kier_right = 0;
			pop_stan_right=1;	
		}
		if(lev==0)
		{
			//Pulse_left_a++;
			v_right_b_f= 1000000/(tick-prev_time_right_b_f);
			prev_time_right_b_f=tick;
			//cout<<"v_left_b_f"<<v_left_b_f<<endl;
			if(pop_pin_right==9 && pop_stan_right== 1 ) kier_right = 0;
			if(pop_pin_right==9 && pop_stan_right== 0 ) kier_right = 1;
			pop_stan_right=0;
		}
		pop_pin_right=11;
	}
	if(kier_right) Pulse_right++; else Pulse_right--;
	//cout<<"licznik prawe"<<Pulse_right<<endl;
}
void encoderPulseleft(int gpio, int lev, uint32_t tick)
{
	if(gpio ==10){ 
		if(lev==1)
		{
			v_left_a_r= 1000000/(tick-prev_time_left_a_r);
			prev_time_left_a_r=tick;
			//cout<<"v_left_a_r"<<v_left_a_r<<endl;
			if(pop_pin_left==22 && pop_stan_left== 1 ){ kier_left = 0; };
			if(pop_pin_left==22 && pop_stan_left== 0 ){ kier_left = 1; };
			pop_stan_left=1;
		}
		if(lev==0)
		{
			//Pulse_left_a++;
			v_left_a_f= 1000000/(tick-prev_time_left_a_f);
			prev_time_left_a_f=tick;
			//cout<<"v_left_a_f"<<v_left_a_f<<endl;
			if(pop_pin_left==22 && pop_stan_left== 0 ) {kier_left = 0; };
			if(pop_pin_left==22 && pop_stan_left== 1 ) {kier_left = 1; }
			pop_stan_left=0;
		}
		pop_pin_left=10;
	}
	if(gpio == 22){
		if(lev==1)
		{
			v_left_b_r= 1000000/(tick-prev_time_left_b_r);
			prev_time_left_b_r=tick;
			if(pop_pin_left==10 && pop_stan_left== 1 ) kier_left = 1;
			if(pop_pin_left==10 && pop_stan_left== 0 ) kier_left = 0;
			pop_stan_left=1;	
		}
		if(lev==0)
		{
			//Pulse_left_a++;
			v_right_b_f= 1000000/(tick-prev_time_left_b_f);
			prev_time_left_b_f=tick;
			//cout<<"v_left_b_f"<<v_left_b_f<<endl;
			if(pop_pin_left==10 && pop_stan_left== 1 ) kier_left = 0;
			if(pop_pin_left==10 && pop_stan_left== 0 ) kier_left = 1;
			pop_stan_left=0;
		}
		pop_pin_left=22;
	}
	//cout<<"kierunek lewe"<<kier_left<<endl;
	if(kier_left){ Pulse_left++; }else Pulse_left--;
	//cout<<"licznik lewe"<<Pulse_left<<endl;
}
int threshold(Mat hsvv)
{
  int max[ilosc_pix];
  int min[ilosc_pix];
  for(int i=0;i<ilosc_pix;i++){max[i]=0;min[i]=100;};
  for(int g=0 ;g<hsvv.rows;g++)
	    {
			 for(int h=0 ;h<hsvv.cols-1;h++)
			{
				int wartosc=hsvv.at<uchar>(g,h);
				for(int i=0;i<ilosc_pix;i++)
				{
					if(wartosc>max[i]) {max[i]=wartosc; break;};
				}
				for(int i=0;i<ilosc_pix;i++)
				{
					if(wartosc<min[i]) {min[i]=wartosc;break;};
				}
			}	
		}	
	int suma_min=0;
	int suma_max=0;
	for(int i=10;i<ilosc_pix;i++)
	{
		
		suma_min=suma_min+min[i];
		suma_max=suma_max+max[i];
	}
	for(int i=0;i<ilosc_pix;i++)cout<<"min"<<min[i]<<"max"<<max[i]<<endl;
	int max_prog=suma_max/(ilosc_pix-10);
	int min_prog=suma_min/(ilosc_pix-10);
	return max_prog-min_prog;
}
void setup_hcsr() {
        gpioSetMode(TRIG, PI_OUTPUT);
        gpioSetMode(ECHO, PI_INPUT);
        //TRIG pin must start LOW
        gpioWrite(TRIG, 0);
        gpioDelay(30000);
        cout<<"odczekalem 30us"<<endl;
        gpioSetTimerFunc(1,100,getCM);
        gpioSetAlertFunc(ECHO, echo_alert);
}
void getCM() 
{
        gpioWrite(TRIG, 1);
        gpioDelay(10);
        gpioWrite(TRIG, 0);
}
void echo_alert(int gpio, int lev, uint32_t tick)
{
	if(gpio==ECHO && lev==1 && zbocze_opadajace==1){echo_start = tick; zbocze_narastajace=1; zbocze_opadajace=0;};
	if(gpio==ECHO && lev==0 && zbocze_narastajace==1)
	{
		zbocze_narastajace=0;
		zbocze_opadajace=1;
		uint32_t czas_echo=tick;
		int travelTime=czas_echo-echo_start;	
		int distance = travelTime / 58;
		if(distance>0 && distance<400)
		{
			//cout<<"odleglosc"<<distance<<endl;
			distance_hcsr1=distance;
			if(distance_hcsr1<15)licznik_przeszkod++;
			else licznik_przeszkod=0;
			if(licznik_przeszkod>1)zezwol_jazda_hcsr=0; 
			else zezwol_jazda_hcsr=1;
		}
	}
}
/*
void obroc_kat(int kat)
{
	if(kat>0)
	{
		lprzod();
		ptyl();
		pozycja_zadana_l=Pulse_left+kat*10;
		pozycja_zadana_r=Pulse_right-kat*10;
	}
	if(pozycja_zadana_l>Pulse_left)
	{
			lprzod();
			gpioPWM(channel_left,400);
			pprzod();
	}
	
}
*/
void sign_check(void)
{
	if(shared_memory1->znak) 
	{
		if(shared_memory1->znak==1) wykryto_znak=1;
		if(shared_memory1->znak==2) wykryto_znak=2;
		if(shared_memory1->znak==3) wykryto_znak=3;
		shared_memory1->znak=0;
		licznik_czasu_znak=0;
	}
	if(wykryto_znak)licznik_czasu_znak++;
	if(licznik_czasu_znak>700) wykryto_znak=0;
}
