#include "VisTrack.h"


const int RETRO_H_MIN = 50;
const int RETRO_H_MAX = 70;

const int RETRO_S_MIN = 250;
const int RETRO_S_MAX = 255;

const int RETRO_V_MIN = 30;
const int RETRO_V_MAX = 255;

cv::Mat LocalProcessImage;
bool IsDisplayable = false;

std::mutex mtx;
std::condition_variable cdv;

void CJ::VisionTracking::SetupVision(cv::Mat *ImageSrc, int CamPort, int FPS, int ResHeight, int ResWidth, int Exposure, std::string Name, bool RetroTrack) {
  std::cout << "CJ-Vision Setup Called" << std::endl;
  if (RetroTrack == true){ Exposure = -100; }
  cam = Camera.cam.CamSetup(CamPort, FPS, ResHeight, ResWidth, Exposure, Name, RetroTrack);
  *ImageSrc = Camera.cam.ImageReturn(cam, Name);
  std::cout << "Setup Complete" << std::endl;
}

void RetroTrackThread(cv::Mat *OutputImage, cv::Mat *InputImage, int ErosionSize, int DialationSize) {
  while (true) {
    cv::cvtColor(*InputImage, LocalProcessImage, cv::COLOR_BGR2HSV); // Uses HSV Spectrum

    // Keeps Only green pixles
    cv::inRange(LocalProcessImage, cv::Scalar(RETRO_H_MIN, RETRO_S_MIN, RETRO_V_MIN), cv::Scalar(RETRO_H_MAX, RETRO_S_MAX, RETRO_V_MAX), LocalProcessImage);

    // Removes pixles at a certain size, And dilates the image to get rid of gaps
    if (ErosionSize > 0) {
      cv::erode(LocalProcessImage, LocalProcessImage, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ErosionSize, ErosionSize)));
      cv::dilate(LocalProcessImage, LocalProcessImage, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(DialationSize, DialationSize)));
    }
    LocalProcessImage.copyTo(*OutputImage);
  }
}

void CJ::VisionTracking::RetroTrack(cv::Mat *OutputImage, cv::Mat *InputImage, int ErosionSize, int DialationSize) {
  while (Camera.cam.sink.GrabFrame(*InputImage) == 0) {
    std::cout << "Can't Get Input Frame (Retro Track Thread)" << std::endl;
  } 
  std::cout << "Input Frame Found (Retro Track Thread)" << std::endl;
  std::thread RetroThread(RetroTrackThread, OutputImage, InputImage, ErosionSize, DialationSize);
  while (Camera.cam.sink.GrabFrame(*OutputImage) == 0) {
    std::cout << "Can't Get Output Frame (Retro Track Thread)" << std::endl;
  }
  std::cout << "Output Frame Found (Retro Track Thread)" << std::endl;
  RetroThread.detach();
}

void CustomTrackThread(cv::Mat *OutputImage, cv::Mat *InputImage, int HColourLowRange, int HColourHighRange, int SColourLowRange, int SColourHighRange, int VColourLowRange, int VColourHighRange, int ErosionSize, int DialationSize, cs::UsbCamera cam) {
  while (true) {
    cv::cvtColor(*InputImage, LocalProcessImage, cv::COLOR_BGR2HSV); // Uses HSV Spectrum

    // Keeps Only green pixles
    cv::inRange(LocalProcessImage, cv::Scalar(HColourLowRange, SColourLowRange, VColourLowRange), cv::Scalar(HColourHighRange, SColourHighRange, VColourHighRange), LocalProcessImage);
    
    // Removes pixles at a certain size, And dilates the image to get rid of gaps
    if (ErosionSize > 0) {
      cv::erode(LocalProcessImage, LocalProcessImage, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(ErosionSize, ErosionSize)));
      cv::dilate(LocalProcessImage, LocalProcessImage, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(DialationSize, DialationSize)));
    }
    LocalProcessImage.copyTo(*OutputImage);
  }
}

void CJ::VisionTracking::CustomTrack(cv::Mat *OutputImage, cv::Mat *InputImage, int HColourLowRange, int HColourHighRange, int SColourLowRange, int SColourHighRange, int VColourLowRange, int VColourHighRange, int ErosionSize, int DialationSize) {
  while (Camera.cam.sink.GrabFrame(*InputImage) == 0) {
    std::cout << "Can't Get Input Frame (Custom Track Thread)" << std::endl;
  }
  std::cout << "Input Frame Found (Custom Thread)" << std::endl;
  std::thread CustomThread(CustomTrackThread, OutputImage, InputImage, HColourLowRange, HColourHighRange, SColourLowRange, SColourHighRange, VColourLowRange, VColourHighRange,  ErosionSize, DialationSize, cam);
  CustomThread.detach();
  std::cout << "Custom Tracking Setup Complete" << std::endl;
}