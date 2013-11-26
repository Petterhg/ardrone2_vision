/*
 * Copyright (C) 2012-2013 Kevin van Hecke
 *
 * This file is part of Paparazzi.
 *
 * Paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Paparazzi; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * @file subsystems/video/video_ardrone2.c
 * Video implementation for ardrone2.
 *
 * Use the tcp output of a custom GStreamer framework plugin to receive
 * telemetry based on video
 */

#include "video_ardrone2.h"

#include <stdio.h>
#include "video_message_structs_sky.h"
#include "subsystems/gps/gps_ardrone2.h"
#include "subsystems/imu/imu_ardrone2_raw.h"

#include <sys/socket.h>       /*  socket definitions        */
#include <sys/types.h>        /*  socket types              */
#include <arpa/inet.h>        /*  inet (3) funtions         */
#include <unistd.h>           /*  misc. UNIX functions      */
#include <errno.h>
#include <string.h> 		/* memset */

#include "state.h" // for altitude
#include "math/pprz_algebra_int.h"


#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#ifndef DOWNLINK_DEVICE
#define DOWNLINK_DEVICE DOWNLINK_AP_DEVICE
#endif
#include "messages.h"
#include "subsystems/datalink/downlink.h"

//#include "subsystems/radio_control.h"
#include "boards/ardrone/navdata.h"


struct VideoARDrone video_impl;
struct gst2ppz_message_struct gst2ppz;
struct ppz2gst_message_struct ppz2gst;

/*  Global constants  */

#define MAX_LINE           (1000)

/*  Global variables  */
int       list_s;                /*  listening socket          */
struct    sockaddr_in servaddr;  /*  socket address structure  */
char      buffer[MAX_LINE];      /*  character buffer          */
char     *endptr;                /*  for strtol()              */


int closeSocket(void) {
	return close(list_s);
}

int initSocket() {

    /*  Create the listening socket  */
    if ( (list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	fprintf(stderr, "tcp server: Error creating listening socket.\n");
	return -1;
    }


    /*  Set all bytes in socket address structure to
        zero, and fill in the relevant data members   */

	char ipa[10];
	sprintf(ipa, "127.0.0.1");

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_port        = htons(PORT);
	if(inet_pton(AF_INET, ipa, &servaddr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }



    if( connect(list_s, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
       printf("\n Error : Video Connect Failed. Is gst-launch running? \n");
       return 1;
    }

	printf("\n Video framework connected! \n");

	return 1;
}

/*  Read a line from a socket  */

int Read_msg_socket(char * data, unsigned int size) {

	int n;
	n = read(list_s, data, size);
    return n;

}


/*  Write a line to a socket  */

ssize_t Write_msg_socket(char * data, unsigned int size) {
    size_t      nleft;
    ssize_t     nwritten;
	nleft  = size;
	nwritten =0;

    while ( nleft > 0 ) {
	if ( (nwritten = write(list_s, data, nleft)) <= 0 ) {
	    if ( errno == EINTR )
		nwritten = 0;
	    else
		return -1;
	}
	nleft  -= nwritten;
	data += nwritten;
    }

    return nwritten;

}

void video_init(void) {


}

//TODO: rename to video process? Receive does not cover full contents, as also updated alt. values are send to gst framework
void video_receive(void) {

	
	
	
	//read the data from the video tcp socket

	if (Read_msg_socket((char *) &gst2ppz,sizeof(gst2ppz))>=0) {		
		video_impl.counter = gst2ppz.counter;
		
		//received new optical flow output:
		//int roll = gst2ppz.optic_flow_x;
		//int pitch = gst2ppz.optic_flow_y;
		
		//printf("Optic flow: %d, %d\n", roll,pitch);
		
    	//DOWNLINK_SEND_VIDEO_TELEMETRY( DefaultChannel, DefaultDevice, &gst2ppz.blob_x1, &gst2ppz.blob_y1,&gst2ppz.blob_x2, &gst2ppz.blob_y2,&gst2ppz.blob_x3, &gst2ppz.blob_y3,&gst2ppz.blob_x4, &gst2ppz.blob_y4);  
		
		


	}

	struct Int32Eulers* att = stateGetNedToBodyEulers_i();
	ppz2gst.counter++;
	ppz2gst.ID = 0x0003;
	ppz2gst.roll = att->phi;
	ppz2gst.pitch = att->theta;
	//ppz2gst.alt = navdata_getHeight();
	Write_msg_socket((char *) &ppz2gst,sizeof(ppz2gst));

	//printf("Roll: %d, Pitch: %d, height: %d\n",att->phi,att->theta,alt); 

}




void video_start(void)
{

	//init and start the GST framework
	//for now this is being done by the makefile.omap from ppz center upload button
	//the following code does not work properly:
	//	int status = system("/data/video/kevin/initvideoall.sh");
	//as it waits until script is done (which never happens)
	//-> no init is needed, framework is started automatically

	//init the socket
	initSocket();


}

void video_stop(void)
{
	printf( "Closing video socket %d", closeSocket());
}

