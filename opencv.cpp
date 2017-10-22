//
//  OpenCVWrapper.mm
//  Apfyt
//
//  Created by Josh Payne on 4/5/17.
//  Copyright © 2017 Apfyt. All rights reserved.
//

#include <iostream>
#include <stdio.h>
#include "OpenCV.h"
#import "UIImage+OpenCV.h"
#import "opencv2/videoio/cap_ios.h"
#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/core/ocl.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/calib3d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/mat.hpp"

using namespace std;
using namespace cv;

int movementLeftX = 0;
int movementRightX = 0;
int movementLeftY = 0;
int movementRightY = 0;

#define DEGREES_RADIANS(angle) ((angle) / 180.0 * M_PI)
#define PI 3.14159265

static void UIImageToMat(UIImage *image, cv::Mat &mat) {
    
    // Create a pixel buffer.
    NSInteger width = CGImageGetWidth(image.CGImage);
    NSInteger height = CGImageGetHeight(image.CGImage);
    CGImageRef imageRef = image.CGImage;
    cv::Mat mat8uc4 = cv::Mat((int)height, (int)width, CV_8UC4);
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef contextRef = CGBitmapContextCreate(mat8uc4.data, mat8uc4.cols, mat8uc4.rows, 8, mat8uc4.step, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrderDefault);
    CGContextDrawImage(contextRef, CGRectMake(0, 0, width, height), imageRef);
    CGContextRelease(contextRef);
    CGColorSpaceRelease(colorSpace);
    
    // Draw all pixels to the buffer.
    cv::Mat mat8uc3 = cv::Mat((int)width, (int)height, CV_8UC3);
    cv::cvtColor(mat8uc4, mat8uc3, CV_RGBA2BGR);
    
    mat = mat8uc3;
}

@implementation OpenCV

+ (long)decode:(nonnull UIImage *)image {
    cv::Mat bgrMat;
    UIImageToMat(image, bgrMat);
    cv::Mat grayMat;
    cv::cvtColor(bgrMat, grayMat, CV_BGR2GRAY);
    //medianBlur(grayMat, grayMat, 15);
    //![reduce_noise]
    long hexInt = 0;
    std::vector<Vec3f> circles;
    std::vector<Vec3f> realCircles;
    /// Apply the Hough Transform to find the circles
    HoughCircles(grayMat, circles, CV_HOUGH_GRADIENT, 1, grayMat.rows/80, 100, 30, 5, 35);
    
    /// Find the extreme circles, check scancode validity
    double roughRadius = 0.0;
    double avgRadius = 0.0;
    int counter = 0;
    int extLeft = INT_MAX;
    int extRight = 0;
    int lC;
    int rC;
    Vec3f leftCircle;
    Vec3f rightCircle;
    for (size_t i = 0; i < circles.size(); i++) {
        roughRadius += cvRound(circles[i][2]);
    }
    roughRadius /= circles.size();

    for (size_t i = 0; i < circles.size(); i++) {
        
        if (roughRadius*.8 < cvRound(circles[i][2]) < roughRadius*1.2) {
            avgRadius += cvRound(circles[i][2]);
            counter++;
            if (cvRound(circles[i][1]) < extLeft) {
                extLeft = cvRound(circles[i][1]);
                lC = int(i);
                rightCircle = circles[i];
            
            }
            if (cvRound(circles[i][1]) > extRight) {
                extRight = cvRound(circles[i][1]);
                rC = int(i);
                leftCircle = circles[i];
            }
        }
    }
    if (counter != 0) {
        avgRadius /= counter;
    }

    double yDim = leftCircle[0] - rightCircle[0];
    double xDim = leftCircle[1] - rightCircle[1];

    Scalar color;
    String binary = "";
    
    double theta = atan((yDim/2)/(xDim/2)) * (180/PI);
    double cosTheta = cos(theta*PI/180);

    double adjacent6 = sqrt((avgRadius * 6 * avgRadius* 6)-((cosTheta*(avgRadius * 6))*(cosTheta*(avgRadius * 6))));
    double adjacent4 = sqrt((avgRadius * 4 * avgRadius* 4)-((cosTheta*(avgRadius * 4))*(cosTheta*(avgRadius * 4))));
    double adjacent2 = sqrt((avgRadius * 2 * avgRadius* 2)-((cosTheta*(avgRadius * 2))*(cosTheta*(avgRadius * 2))));
    
    if (leftCircle[0] < rightCircle[0]) {
        adjacent6*=-1;
        adjacent4*=-1;
        adjacent2*=-1;
    }

    if (avgRadius*2*9.60 < sqrt((yDim)*(yDim) + (xDim)*(xDim)) && sqrt((yDim)*(yDim) + (xDim)*(xDim)) < avgRadius*2*11.8 && abs(yDim) < 60) {
        cv::Point cen;
        ///Top layer
        for (int i = 1; i < 8; i++) {
            cen.y = cvRound((leftCircle[1]) - ((xDim/9)*i) - (xDim/18) + adjacent6);
            cen.x = cvRound(leftCircle[0] + avgRadius/3 - (cosTheta*(avgRadius * 6) + (yDim/9)*i));
            color = grayMat.at<uchar>(cen);
            if (color.val[0] < 120) {
                binary += "1";
            }
            else binary += "0";
           
        }
        ///Midtop Layer
        for (int i = 0; i < 8; i++) {
            cen.y = cvRound((leftCircle[1]) - ((xDim/9)*i) - (xDim/9) + adjacent4);
            cen.x = cvRound(leftCircle[0] + avgRadius/4 - (cosTheta*(avgRadius * 4) + (yDim/9)*i));
            color = grayMat.at<uchar>(cen);
            if (color.val[0] < 120) {
                binary += "1";
            }
            else binary += "0";
        }
        ///Up Midleft
        for (int i = 0; i < 2; i++) {
            cen.y = cvRound((leftCircle[1]) - ((xDim/9)*i) - (xDim/18) + adjacent2);
            cen.x = cvRound(leftCircle[0] + avgRadius/5 - (cosTheta*(avgRadius * 2) + (yDim/9)*i));
            color = grayMat.at<uchar>(cen);
            if (color.val[0] < 120) {
                binary += "1";
            }
            else binary += "0";
        }
        ///Up Midright
        for (int i = 7; i < 9; i++) {
            cen.y = cvRound((leftCircle[1]) - ((xDim/9)*i) - (xDim/18) + adjacent2);
            cen.x = cvRound(leftCircle[0] + avgRadius/5 - (cosTheta*(avgRadius * 2) + (yDim/9)*i));
            color = grayMat.at<uchar>(cen);
            if (color.val[0] < 120) {
                binary += "1";
            }
            else binary += "0";
        }
        ///left
        cen.y = cvRound(leftCircle[1] - xDim/9);
        cen.x = cvRound(leftCircle[0] - yDim/9);
        if (color.val[0] < 120) {
            binary += "1";
        }
        else binary += "0";
        ///right
        cen.y = cvRound((leftCircle[1]) - ((xDim/9)*8));
        cen.x = cvRound(leftCircle[0] - yDim/9*8);
        color = grayMat.at<uchar>(cen);
        if (color.val[0] < 120) {
            binary += "1";
        }
        else binary += "0";
        ///Down Midleft
        for (int i = 0; i < 2; i++) {
            cen.y = cvRound((leftCircle[1]) - ((xDim/9)*i) - (xDim/18) - adjacent2);
            cen.x = cvRound(leftCircle[0] - avgRadius/5 + (cosTheta*(avgRadius * 2) - (yDim/9)*i));
            color = grayMat.at<uchar>(cen);
            if (color.val[0] < 120) {
                binary += "1";
            }
            else binary += "0";
        }
        ///Down Midright
        for (int i = 7; i < 9; i++) {
            cen.y = cvRound((leftCircle[1]) - ((xDim/9)*i) - (xDim/18) - adjacent2);
            cen.x = cvRound(leftCircle[0] - avgRadius/5 + (cosTheta*(avgRadius * 2) - (yDim/9)*i));
            color = grayMat.at<uchar>(cen);
            if (color.val[0] < 120) {
                binary += "1";
            }
            else binary += "0";
        }
        ///Midbottom Layer
        for (int i = 0; i < 8; i++) {
            cen.y = cvRound((leftCircle[1]) - ((xDim/9)*i) - (xDim/9) - adjacent4);
            cen.x = cvRound(leftCircle[0] - avgRadius/4 + (cosTheta*(avgRadius * 4) - (yDim/9)*i));
            color = grayMat.at<uchar>(cen);
            if (color.val[0] < 120) {
                binary += "1";
            }
            else binary += "0";
        }
        ///Bottom layer
        for (int i = 1; i < 8; i++) {
            cen.y = cvRound((leftCircle[1]) - ((xDim/9)*i) - (xDim/18) - adjacent6);
            cen.x = cvRound(leftCircle[0] - avgRadius/3 + (cosTheta*(avgRadius * 6) - (yDim/9)*i));
            color = grayMat.at<uchar>(cen);
            if (color.val[0] < 120) {
                binary += "1";
            }
            else binary += "0";
        }
    }
    
    //return hexInt;
    hexInt=0;
    int len = int(binary.size());
    for (int i=0;i<len;i++) {
        hexInt+=( binary[len-i-1]-48) * pow(2,i);
    }
    return hexInt;
}

@end