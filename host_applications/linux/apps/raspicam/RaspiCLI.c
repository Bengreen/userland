/*
Copyright (c) 2013, Broadcom Europe Ltd
Copyright (c) 2013, James Hughes
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * \file RaspiCLI.c
 * Code for handling command line parameters
 *
 * \date 4th March 2013
 * \Author: James Hughes
 *
 * Description
 *
 * Some functions/structures for command line parameter parsing
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include "interface/vcos/vcos.h"

#include "RaspiCLI.h"



#define CommandSharpness   0
#define CommandContrast    1
#define CommandBrightness  2
#define CommandSaturation  3
#define CommandISO         4
#define CommandVideoStab   5
#define CommandEVComp      6
#define CommandExposure  7
#define CommandAWB         8
#define CommandImageFX     9
#define CommandColourFX    10
#define CommandMeterMode   11
#define CommandRotation    12
#define CommandHFlip       13
#define CommandVFlip       14
#define CommandROI         15
#define CommandShutterSpeed 16


static COMMAND_LIST  cmdline_commands[] =
{
    {CommandSharpness,   "-sharpness", "sh", "Set image sharpness (-100 to 100)",  1},
    {CommandContrast,    "-contrast",  "co", "Set image contrast (-100 to 100)",  1},
    {CommandBrightness,  "-brightness","br", "Set image brightness (0 to 100)",  1},
    {CommandSaturation,  "-saturation","sa", "Set image saturation (-100 to 100)", 1},
    {CommandISO,         "-ISO",       "ISO","Set capture ISO",  1},
    {CommandVideoStab,   "-vstab",     "vs", "Turn on video stabilisation", 0},
    {CommandEVComp,      "-ev",        "ev", "Set EV compensation",  1},
    {CommandExposure,    "-exposure",  "ex", "Set exposure mode (see Notes)", 1},
    {CommandAWB,         "-awb",       "awb","Set AWB mode (see Notes)", 1},
    {CommandImageFX,     "-imxfx",     "ifx","Set image effect (see Notes)", 1},
    {CommandColourFX,    "-colfx",     "cfx","Set colour effect (U:V)",  1},
    {CommandMeterMode,   "-metering",  "mm", "Set metering mode (see Notes)", 1},
    {CommandRotation,    "-rotation",  "rot","Set image rotation (0-359)", 1},
    {CommandHFlip,       "-hflip",     "hf", "Set horizontal flip", 0},
    {CommandVFlip,       "-vflip",     "vf", "Set vertical flip", 0},
    {CommandROI,         "-roi",       "roi","Set region of interest (x,y,w,d as normalised coordinates [0.0-1.0])", 1},
    {CommandShutterSpeed,"-shutter",   "ss", "Set shutter speed in microseconds", 1}
};

static int cmdline_commands_size = sizeof(cmdline_commands) / sizeof(cmdline_commands[0]);




/**
 * Convert a string from command line to a comand_id from the list
 *
 * @param commands Array of command to check
 * @param num_command Number of commands in the array
 * @param arg String to search for in the list
 * @param num_parameters Returns the number of parameters used by the command
 *
 * @return command ID if found, -1 if not found
 *
 */
int raspicli_get_command_id(const COMMAND_LIST *commands, const int num_commands, const char *arg, int *num_parameters)
{
   int command_id = -1;
   int j;

   vcos_assert(commands);
   vcos_assert(num_parameters);
   vcos_assert(arg);

   if (!commands || !num_parameters || !arg)
      return -1;

   for (j = 0; j < num_commands; j++)
   {
      if (!strcmp(arg, commands[j].command) ||
          !strcmp(arg, commands[j].abbrev))
      {
         // match
         command_id = commands[j].id;
         *num_parameters = commands[j].num_parameters;
         break;
      }
   }

   return command_id;
}


/**
 * Display the list of commands in help format
 *
 * @param commands Array of command to check
 * @param num_command Number of commands in the arry
 *
 *
 */
void raspicli_display_help(const COMMAND_LIST *commands, const int num_commands)
{
   int i;

   vcos_assert(commands);

   if (!commands)
      return;

   for (i = 0; i < num_commands; i++)
   {
      fprintf(stderr, "-%s, -%s\t: %s\n", commands[i].abbrev,
         commands[i].command, commands[i].help);
   }
}



/**
 * Display help for command line options
 */
void raspicamcontrol_display_help()
{
    int i;
    
    fprintf(stderr, "\nImage parameter commands\n\n");
    
    raspicli_display_help(cmdline_commands, cmdline_commands_size);
    
    fprintf(stderr, "\n\nNotes\n\nExposure mode options :\n%s", exposure_map[0].mode );
    
    for (i=1;i<exposure_map_size;i++)
    {
        fprintf(stderr, ",%s", exposure_map[i].mode);
    }
    
    fprintf(stderr, "\n\nAWB mode options :\n%s", awb_map[0].mode );
    
    for (i=1;i<awb_map_size;i++)
    {
        fprintf(stderr, ",%s", awb_map[i].mode);
    }
    
    fprintf(stderr, "\n\nImage Effect mode options :\n%s", imagefx_map[0].mode );
    
    for (i=1;i<imagefx_map_size;i++)
    {
        fprintf(stderr, ",%s", imagefx_map[i].mode);
    }
    
    fprintf(stderr, "\n\nMetering Mode options :\n%s", metering_mode_map[0].mode );
    
    for (i=1;i<metering_mode_map_size;i++)
    {
        fprintf(stderr, ",%s", metering_mode_map[i].mode);
    }
    
    fprintf(stderr, "\n");
}

/**
 * Parse a possible command pair - command and parameter
 * @param arg1 Command
 * @param arg2 Parameter (could be NULL)
 * @return How many parameters were used, 0,1,2
 */
int raspicamcontrol_parse_cmdline(RASPICAM_CAMERA_PARAMETERS *params, const char *arg1, const char *arg2)
{
    int command_id, used = 0, num_parameters;
    
    if (!arg1)
        return 0;
    
    command_id = raspicli_get_command_id(cmdline_commands, cmdline_commands_size, arg1, &num_parameters);
    
        // If invalid command, or we are missing a parameter, drop out
    if (command_id==-1 || (command_id != -1 && num_parameters > 0 && arg2 == NULL))
        return 0;
    
    switch (command_id)
    {
        case CommandSharpness : // sharpness - needs single number parameter
            sscanf(arg2, "%d", &params->sharpness);
            used = 2;
            break;
            
        case CommandContrast : // contrast - needs single number parameter
            sscanf(arg2, "%d", &params->contrast);
            used = 2;
            break;
            
        case CommandBrightness : // brightness - needs single number parameter
            sscanf(arg2, "%d", &params->brightness);
            used = 2;
            break;
            
        case CommandSaturation : // saturation - needs single number parameter
            sscanf(arg2, "%d", &params->saturation);
            used = 2;
            break;
            
        case CommandISO : // ISO - needs single number parameter
            sscanf(arg2, "%d", &params->ISO);
            used = 2;
            break;
            
        case CommandVideoStab : // video stabilisation - if here, its on
            params->videoStabilisation = 1;
            used = 1;
            break;
            
        case CommandEVComp : // EV - needs single number parameter
            sscanf(arg2, "%d", &params->exposureCompensation);
            used = 2;
            break;
            
        case CommandExposure : // exposure mode - needs string
            params->exposureMode = exposure_mode_from_string(arg2);
            used = 2;
            break;
            
        case CommandAWB : // AWB mode - needs single number parameter
            params->awbMode = awb_mode_from_string(arg2);
            used = 2;
            break;
            
        case CommandImageFX : // Image FX - needs string
            params->imageEffect = imagefx_mode_from_string(arg2);
            used = 2;
            break;
            
        case CommandColourFX : // Colour FX - needs string "u:v"
            sscanf(arg2, "%d:%d", &params->colourEffects.u, &params->colourEffects.v);
            params->colourEffects.enable = 1;
            used = 2;
            break;
            
        case CommandMeterMode:
            params->exposureMeterMode = metering_mode_from_string(arg2);
            used = 2;
            break;
            
        case CommandRotation : // Rotation - degree
            sscanf(arg2, "%d", &params->rotation);
            used = 2;
            break;
            
        case CommandHFlip :
            params->hflip  = 1;
            used = 1;
            break;
            
        case CommandVFlip :
            params->vflip = 1;
            used = 1;
            break;
            
        case CommandROI :
        {
            double x,y,w,h;
            int args;
            
            args = sscanf(arg2, "%lf,%lf,%lf,%lf", &x,&y,&w,&h);
            
            if (args != 4 || x > 1.0 || y > 1.0 || w > 1.0 || h > 1.0)
            {
                return 0;
            }
            
                // Make sure we stay within bounds
            if (x + w > 1.0)
                w = 1 - x;
            
            if (y + h > 1.0)
                h = 1 - y;
            
            params->roi.x = x;
            params->roi.y = y;
            params->roi.w = w;
            params->roi.h = h;
            
            used = 2;
            break;
        }
            
        case CommandShutterSpeed : // Shutter speed needs single number parameter
            sscanf(arg2, "%d", &params->shutter_speed);
            used = 2;
            break;
            
            
    }
    
    return used;
}


