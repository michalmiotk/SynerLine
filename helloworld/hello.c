#include "cv.h"
#include "highgui.h"
 
int main(){
	IplImage* obraz;
	obraz = cvLoadImage("linux.jpg",1);
	cvNamedWindow("Sample 1", 1);
	cvShowImage("Sample 1", obraz);
	cvWaitKey(0);
	cvDestroyWindow("Sample 1");
	cvReleaseImage(&obraz);
	return 0;
}
