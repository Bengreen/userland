
#include <iostream>
#include <fstream>

#include <stdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <sysexits.h>

#define VERSION_STRING "v1.3.2"

#include "bcm_host.h"
#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/mmal_logging.h"
#include "interface/mmal/mmal_buffer.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"


#include "RaspiCamControl.h"

#include <semaphore.h>

    /// Camera number to use - we only have one camera, indexed from 0.
#define CAMERA_NUMBER 0

    // Standard port setting for the camera component
#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2


    // Stills format information
#define STILLS_FRAME_RATE_NUM 3
#define STILLS_FRAME_RATE_DEN 1

    /// Video render needs at least 2 buffers.
#define VIDEO_OUTPUT_BUFFERS_NUM 3




class fastWrite {
public:
    fastWrite(std::ostream &buf, int size=10*1024) :
    buffer(new char[size]),
    index(0),
    bufferSize(size),
    out(buf)
    {
        
    }
    
    ~fastWrite() {
        flushBuffer();
        delete[] buffer;
    }
    
    template<typename writeObject_t>
    fastWrite& write(writeObject_t x, int padding=0);
    
    
    
private:
    
    void flushBuffer() {
        
        if(index) {
            out.write(buffer,index);
        }
        
        index=0;
        
    }
    
    char *buffer;
    int index;
    int bufferSize;
    std::ostream & out;
    
    
};


template<typename writeObject_t>
fastWrite& fastWrite::write(writeObject_t x, int padding) {
    
    if(padding<sizeof(writeObject_t))
        padding=sizeof(writeObject_t);
    
    if(index+sizeof(writeObject_t)>=bufferSize) {
        flushBuffer();
    }
    
        //Do crappy copy by char type
    
    std::copy(reinterpret_cast<const char *>(&x),
              reinterpret_cast<const char *>(&x)+sizeof(writeObject_t),
              buffer+index);
    
    
    
    std::fill(buffer+index+sizeof(writeObject_t),buffer+index+padding,char(0));
    index+=padding;
    
        //    std::cout<<"index:"<<index<<std::endl;
    
    return *this;
}


template<>
fastWrite& fastWrite::write<std::string>(std::string x, int padding) {
    
    if(padding<x.size())
        padding=int(x.size());
    
    if(index+padding>=bufferSize) {
        flushBuffer();
    }
    
        //    std::cout<<"string size:"<<x.size()<<std::endl;
        //    std::cout<<"start:"<<long(reinterpret_cast<const char *>(x.c_str()))<<std::endl;
        //    std::cout<<"end:"<<long(reinterpret_cast<const char *>(x.c_str()+x.size()))<<std::endl;
    
    std::copy(reinterpret_cast<const char *>(x.c_str()),
              reinterpret_cast<const char *>(x.c_str()+x.size()),
              buffer+index);
    
    
    
    std::fill(buffer+index+x.size(),buffer+index+padding,char(0));
    index+=padding;
    
    
        //    std::cout<<"index:"<<index<<std::endl;
    
    return *this;
}


/** Structure containing all state information for the current run
 */
typedef struct
{
    int timeout;                        /// Time taken before frame is grabbed and app then shuts down. Units are milliseconds
    int width;                          /// Requested width of image
    int height;                         /// requested height of image
    int verbose;                        /// !0 if want detailed run information
    int timelapse;                      /// Delay between each picture in timelapse mode. If 0, disable timelapse
    int useRGB;                         /// Output RGB data rather than YUV
    
    RASPICAM_CAMERA_PARAMETERS camera_parameters; /// Camera setup parameters
    
    MMAL_COMPONENT_T *camera_component;    /// Pointer to the camera component
    MMAL_COMPONENT_T *null_sink_component;    /// Pointer to the camera component
    MMAL_POOL_T *camera_pool;              /// Pointer to the pool of buffers used by camera stills port
} RASPISTILLYUV_STATE;



static void signal_handler(int signal_number)
{
        // Going to abort on all signals
    vcos_log_error("Aborting program\n");
    
        // Need to close any open stuff...
    
    exit(255);
}

static void default_status(RASPISTILLYUV_STATE *state)
{
    if (!state)
    {
        vcos_assert(0);
        return;
    }
    
        // Default everything to zero
    memset(state, 0, sizeof(RASPISTILLYUV_STATE));
    
        // Now set anything non-zero
    state->timeout = 5000; // 5s delay before take image
    state->width = 2592;
    state->height = 1944;
    state->timelapse = 0;
    
        // Setup preview window defaults
        //    raspipreview_set_defaults(&state->preview_parameters);
    
        // Set up the camera_parameters to default
    raspicamcontrol_set_defaults(&state->camera_parameters);
}


int main(int argc, const char **argv)
{
    
    
    int size_x = 2592;
    int size_y = 1944;
    
    std::ofstream myFile("newt.ppm", std::ofstream::out);
    
    
    myFile<<"P6\n"<<size_x<<" "<<size_y<<"\n255\n";
    
    {
        fastWrite myFast(myFile);
        
        for(int j=0;j<size_y;++j) {
            for(int i=0;i<size_x;++i) {
                myFast.write(char(i%256)).write(char(j%256)).write(char((i*j)%256));
            }
        }
        
    }
    std::cout<<"Have written: newt.ppm"<<std::endl;
    myFile.close();
    
    
    {
            // Our main data storage vessel..
        RASPISTILLYUV_STATE state;
        int exit_code = EX_OK;
        
        MMAL_STATUS_T status = MMAL_SUCCESS;
        MMAL_PORT_T *camera_preview_port = NULL;
        MMAL_PORT_T *camera_video_port = NULL;
        MMAL_PORT_T *camera_still_port = NULL;
        MMAL_PORT_T *preview_input_port = NULL;
        FILE *output_file = NULL;
        
        
        bcm_host_init();
     
            // Register our application with the logging system
        vcos_log_register("RaspiStill", VCOS_LOG_CATEGORY);
        
        signal(SIGINT, signal_handler);

        default_status(&state);

        
    }
    
    std::cout<<"Version 3"<<std::endl;
    return 0;
}




