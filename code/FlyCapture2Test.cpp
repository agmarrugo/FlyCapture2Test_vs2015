//=============================================================================
// This software is partially based on FlyCapture2Test.cpp by FLIR
//
//
// by professor Andres Marrugo, 2020 - Universidad Tecnologica de Bolivar, Colombia
//=============================================================================
//=============================================================================

#include "stdafx.h"

#include "FlyCapture2.h"
#include <iostream>
#include <sstream>

using namespace FlyCapture2;
using namespace std;

void PrintBuildInfo()
{
    FC2Version fc2Version;
    Utilities::GetLibraryVersion(&fc2Version);

    ostringstream version;
    version << "FlyCapture2 library version: " << fc2Version.major << "."
            << fc2Version.minor << "." << fc2Version.type << "."
            << fc2Version.build;
    cout << version.str() << endl;

    ostringstream timeStamp;
    timeStamp << "Application build date: " << __DATE__ << " " << __TIME__;
    cout << timeStamp.str() << endl << endl;
}

void PrintCameraInfo(CameraInfo *pCamInfo)
{
    cout << endl;
    cout << "*** CAMERA INFORMATION ***" << endl;
    cout << "Serial number - " << pCamInfo->serialNumber << endl;
    cout << "Camera model - " << pCamInfo->modelName << endl;
    cout << "Camera vendor - " << pCamInfo->vendorName << endl;
    cout << "Sensor - " << pCamInfo->sensorInfo << endl;
    cout << "Resolution - " << pCamInfo->sensorResolution << endl;
    cout << "Firmware version - " << pCamInfo->firmwareVersion << endl;
    cout << "Firmware build time - " << pCamInfo->firmwareBuildTime << endl
         << endl;
}

void PrintError(Error error) { error.PrintErrorTrace(); }

int RunSingleCamera(PGRGuid guid)
{
    const int k_numImages = 16;

    Error error;    

    // Connect to a camera
	Camera cam;
    error = cam.Connect(&guid);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    // Get the camera information
    CameraInfo camInfo;
    error = cam.GetCameraInfo(&camInfo);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    PrintCameraInfo(&camInfo);

    // Get the camera configuration
    FC2Config config;
    error = cam.GetConfiguration(&config);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    // Set the number of driver buffers used to 10.
    config.numBuffers = 100;
	
	config.grabMode = BUFFER_FRAMES; //modf
	config.highPerformanceRetrieveBuffer = true; //modf
	
	//modf

	//Declare a Property struct.
	Property prop;
	//Define the property to adjust.
	prop.type = SHUTTER;
	//Ensure the property is on.
	prop.onOff = true;
	//Ensure auto-adjust mode is off.
	prop.autoManualMode = false;
	//Ensure the property is set up to use absolute value control.
	prop.absControl = true;
	//Set the absolute value of shutter to 20 ms.
	prop.absValue = 2;
	//Set the property.
	error = cam.SetProperty(&prop);

	//Define the property to adjust.
	prop.type =	GAIN;
	//Ensure the property is on.
	prop.onOff = true;
	//Ensure auto-adjust mode is off.
	prop.autoManualMode = false;
	//Ensure the property is set up to use absolute value control.
	prop.absControl = true;
	//Set the absolute value of shutter to 20 ms.
	prop.absValue = 2;
	//Set the property.
	error = cam.SetProperty(&prop);


    // Set the camera configuration
    error = cam.SetConfiguration(&config);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

	// Get current trigger settings
	TriggerMode triggerMode;
	error = cam.GetTriggerMode(&triggerMode);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}

	// Set camera to trigger mode 14
	triggerMode.onOff = true;
	triggerMode.mode = 14;
	triggerMode.parameter = 0;

#ifdef SOFTWARE_TRIGGER_CAMERA
	// A source of 7 means software trigger
	triggerMode.source = 7;
#else
	// Triggering the camera externally using source 0.
	triggerMode.source = 0;
#endif

	error = cam.SetTriggerMode(&triggerMode);
	if (error != PGRERROR_OK)
	{
		PrintError(error);
		return -1;
	}


    // Start capturing images
    error = cam.StartCapture();
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    Image rawImage;
    for (int imageCnt = 0; imageCnt < k_numImages; imageCnt++)
    {
        // Retrieve an image
        error = cam.RetrieveBuffer(&rawImage);
        if (error != PGRERROR_OK)
        {
            PrintError(error);
            continue;
        }

        cout << "Grabbed image " << imageCnt << endl;

        // Create a converted image
        Image convertedImage;

        // Convert the raw image
        error = rawImage.Convert(PIXEL_FORMAT_MONO8, &convertedImage);
        if (error != PGRERROR_OK)
        {
            PrintError(error);
            return -1;
        }

        // Create a unique filename

        ostringstream filename;
        filename << "captured_images\\FlyCapture2Test-" << camInfo.serialNumber << "-"
                 << imageCnt << ".bmp";

        // Save the image. If a file format is not passed in, then the file
        // extension is parsed to attempt to determine the file format.
        error = convertedImage.Save(filename.str().c_str());
        if (error != PGRERROR_OK)
        {
            PrintError(error);
            return -1;
        }
    }

    // Stop capturing images
    error = cam.StopCapture();
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    // Disconnect the camera
    error = cam.Disconnect();
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    return 0;
}

int main(int /*argc*/, char ** /*argv*/)
{
    PrintBuildInfo();

    Error error;

    // Since this application saves images in the current folder
    // we must ensure that we have permission to write to this folder.
    // If we do not have permission, fail right away.
    FILE *tempFile = fopen("test.txt", "w+");
    if (tempFile == NULL)
    {
        cout << "Failed to create file in current folder.  Please check "
                "permissions."
             << endl;
        return -1;
    }
    fclose(tempFile);
    remove("test.txt");

    BusManager busMgr;
    unsigned int numCameras;
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    cout << "Number of cameras detected: " << numCameras << endl;

    for (unsigned int i = 0; i < numCameras; i++)
    {
        PGRGuid guid;
        error = busMgr.GetCameraFromIndex(i, &guid);
        if (error != PGRERROR_OK)
        {
            PrintError(error);
            return -1;
        }

        RunSingleCamera(guid);
    }

    cout << "Press Enter to exit..." << endl;
    cin.ignore();

    return 0;
}
