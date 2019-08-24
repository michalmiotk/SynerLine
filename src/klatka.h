#include <math.h>
#define PI 3.14
using namespace std;
void anglemid(int part,int rows, int columns, int* points, int* angles, int* mid);
void drawverticalpoints(int part, Mat tab, int* points, int columns, int rows);
void drawpoints(int part, Mat tab, int* points, int rows);
void detect_vertical(struct text* a, Mat wcolor,  Mat bcolor,Mat out, int* vertical_points, int czesc, int kolumny, int rzedy);
void klatka(struct text* a,int rzedy, int kolumny,  int korekcja, Mat array,Mat pochodna, int* punkty, int* katy, int* mid){
  for(int czesc=0;czesc<SECTION;czesc++)
  {
	int pointdisappear=0;
 	for(int k=czesc*rzedy/SECTION;k<(czesc+1)*rzedy/SECTION;k++)
 	{
			int ilosc=0; 
			int suma=0;
			for(int j=1;j<kolumny-1;j++)
			{
				if(pochodna.at<uchar>(k,j)==255)
				{
					ilosc++;
					suma=suma+j;
				}
			}			
			if(ilosc){ punkty[k]=suma/ilosc; } else{ pointdisappear++; punkty[k]=-1; } 
	}
	if(pointdisappear>5)
	{ 
		for(int x=czesc*rzedy/SECTION; x<(czesc+1)*rzedy/SECTION;x++) { punkty[x]=-1; }
	}
	else
	{ 
		anglemid(czesc, rzedy, kolumny, punkty, katy, mid);
		drawpoints(czesc, array, punkty, rzedy);	
	}
  }
  imshow("out" , array);
}

void drawpoints(int part, Mat tab, int* points, int rows){
	if(part>SECTION) cout<<"zle dopasowana ilosc sekcji"<<endl;
	for(int i=part*rows/SECTION; i<(part+1)*rows/SECTION; i++)
	{
	  if(points[i] != -1) {line(tab,cv::Point(points[i],i), cv::Point(points[i],i),255,2,CV_AA);} 
	 // else cout<<"brak punktu " <<i<<"=="<<points[i]<<endl;
	}
}

void anglemid(int part, int rows, int columns, int* points, int* angles, int* mid){
		    int pierwszy_punkt_x=-1, pierwszy_punkt_y=-1, ostatni_punkt_x=-1, ostatni_punkt_y=-1;
			for(int i=part*rows/SECTION; i<(part+1)*rows/SECTION; i++){
				if(points[i] !=-1){
					if(pierwszy_punkt_y !=-1){
						ostatni_punkt_x=points[i];
						ostatni_punkt_y=i;
					}
					if(pierwszy_punkt_y ==-1){
						pierwszy_punkt_x=points[i];
						pierwszy_punkt_y=i;
					}
				}
			}
			double distancex=double(ostatni_punkt_x-pierwszy_punkt_x);
			double distancey=double(ostatni_punkt_y-pierwszy_punkt_y);
			//cout<<pierwszy_punkt_x<<"pierwszt puntkx"<<ostatni_punkt_x<<"ostatni punkt"<<endl;
			if(pierwszy_punkt_x != -1 && ostatni_punkt_x !=-1){
				//cout<<distancey<<"to y"<<distancex<<"to x"<<endl;
				int angle=((atan(distancex/distancey))*180/PI);
				//int angle=((atan(1))*180/PI);
				angles[part]=angle;
				//cout<<part<<"kat"<<angle<<endl;
			}else{ //cout<<"nieznaleziono kata"<<endl; 
				angles[part]=-1; 
			}
			int sumaodl=0;
			int ilosc_punktow=0;
			for(int i=part*rows/SECTION; i<(part+1)*rows/SECTION; i++){
				if(points[i] !=-1){
					sumaodl=sumaodl+points[i];
					ilosc_punktow++;
				}
			}
			if(ilosc_punktow) mid[part]=sumaodl/ilosc_punktow;
}



