#include <stdio.h>
#include <stdlib.h>
#include "../LightLib/light_interface.h"

#include <cv.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <cxcore.h>

int main()
{
    IplImage *pstImage = NULL;
    IplImage *pstImageYuv = NULL;
    pstImage = cvLoadImage("1.png", 1);

    IA_LIGHT_INTERFACE_S stLightInterface;
    IA_LIGHT_AREA_S stLightArea;
    IA_LIGHT_RSLT_S stLightRslt;
    void *pvLight = NULL;

    stLightInterface.iBitCnt = 8;
    stLightInterface.iImgHgt = pstImage->height;
    stLightInterface.iImgWid = pstImage->width;
    stLightInterface.iStride = stLightInterface.iImgWid;

    IA_Light_Init(&stLightInterface, &pvLight);

    pstImageYuv = cvCreateImage(cvSize(pstImage->width, pstImage->height * 3 / 2), IPL_DEPTH_8U, 1);
    cvCvtColor(pstImage, pstImageYuv, CV_BGR2YUV_IYUV);

    stLightArea.stAreaSet.i16X1 = 0;
    stLightArea.stAreaSet.i16Y1 = 0;
    stLightArea.stAreaSet.i16X2 = stLightInterface.iImgWid;
    stLightArea.stAreaSet.i16Y2 = stLightInterface.iImgHgt;
    stLightArea.stImage.pucImgData = pstImageYuv->imageData;
    stLightArea.stImage.usBitCnt = 8;
    stLightArea.stImage.usHeight = stLightInterface.iImgHgt;
    stLightArea.stImage.usWidth = stLightInterface.iImgWid;
    stLightArea.stImage.usStride = stLightInterface.iStride;

    IA_Light_Work(pvLight, &stLightArea, &stLightRslt);

    for (int i = 0; i < stLightRslt.iCount; i++)
    {
        cvCircle(pstImage, cvPoint(stLightRslt.astDetRslt[i].iCenterX, stLightRslt.astDetRslt[i].iCenterY), 3, CV_RGB(255, 0, 0), 2, 8, 0);
    }

    cvShowImage("Org", pstImage);
    cvWaitKey(0);
    cvReleaseImage(&pstImage);
}