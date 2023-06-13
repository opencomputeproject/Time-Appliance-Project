/*
 *  Tangle User Interface Source File.
 *
 *  Tangle lets user fetch remote machine uncorrected monotonic clock time 
 *  corresponding to a given uncorrected clock time on a local machine
 *  
 *  Usage:
 *      ./tangle <Remote System IP>  \
 *                <local uncorrected monotonic clock time seconds>  \
 *                <local uncorrected monotonic clock time nanoseconds>  
 *  Return:
 *      Returns remote systems uncorrected monotonic clock time for 
 *
 *      
 *  Error: 
 *     a. Local machine or remote machine is not tracking the requested raw time 
 *        against utc time
 *     b. Local machine or remote machine time service is not running
 *
 *
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include "msg.h"
#include <time.h>

void printmsg(tangle_api_t *api){
  printf ("MSG_TYPE = %d\n", api->msg);
  
  switch (api->msg)
  {
     case GET_RAW_FROM_UTC_REQ:
     case GET_UTC_FROM_RAW_RES:
        printf ("utc time = %d\n", api->val.utc_time.epoch);
        break;

     case GET_UTC_FROM_RAW_REQ:
     case GET_RAW_FROM_UTC_RES:
        printf ("\traw time(s) = %llu\n", api->val.raw_time.sec);
        printf ("\traw time(ns) = %llu\n", api->val.raw_time.nsec);
        break;
     default:
        //No Action
        break;
  }
}


void getRawFromUtc (u32_t epoch, raw_time_t *raw, int fd)
{ 
    tangle_api_t api;
   int nbytes = 0, n=0;
   char rbuf[1024];

   memset(rbuf, '0',sizeof(rbuf));
    api.msg = GET_RAW_FROM_UTC_REQ;
    api.val.utc_time.epoch = epoch; // TODO: network byte order

   if ((nbytes = write(fd, &api, sizeof(api))) < 0)
   {
      printf("\n Error: Write error\n");
      return;
   }

    while ( (n = read(fd, rbuf, sizeof(rbuf)-1)) > 0)
    {
        //printmsg((tangle_api_t*)rbuf);
        raw->sec = ((tangle_api_t*)rbuf)->val.raw_time.sec;
        raw->nsec = ((tangle_api_t*)rbuf)->val.raw_time.nsec;
        return;
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 
   
}

void getUtcFromRaw (raw_time_t *raw,  utc_time_t *utc, int fd)
{
   tangle_api_t api;
   int nbytes = 0, n=0;
   char rbuf[1024];

   memset(rbuf, '0',sizeof(rbuf));
   api.msg = GET_UTC_FROM_RAW_REQ;
   api.val.raw_time.sec = raw->sec;
   api.val.raw_time.nsec = raw->nsec;

   if ((nbytes = write(fd, &api, sizeof(api))) < 0)
   {
      printf("\n Error: Write error\n");
      return;
   }

    while ( (n = read(fd, rbuf, sizeof(rbuf)-1)) > 0)
    {
        //printmsg((tangle_api_t*)rbuf);

        utc->epoch = ((tangle_api_t*)rbuf)->val.utc_time.epoch;

        return;
    } 

    if(n < 0)
    {
        printf("\n Read error \n");
    } 
}

int main(int argc, char *argv[])
{
    char *end_p;
    int sockfd = 0;
    struct sockaddr_in serv_addr; 
    time_t ticks;

    raw_time_t raw, raw_remote;
    utc_time_t utc;

    if(argc != 4)
    {
        printf("\n Usage: %s <ip of server> <raw_time_sec> <raw_time_ns> \n",argv[0]);
        printf("\t\t raw_time is local system unlatered uptime.  range - [max(0, uptime - 10000), /proc/uptime]\n");  
        return 1;
    } 
    raw.sec = strtoll(argv[2], &end_p, 10);
    raw.nsec = strtoll(argv[3], &end_p, 10);
    //printf ("Supplied unaltered time = %lld.%lld\n", raw.sec, raw.nsec);


    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5214); 

    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 
    
    getUtcFromRaw (&raw, &utc, sockfd);


    close (sockfd);

    if (utc.epoch == 0)
    {
        printf ("No matching time for requested input [%lld:%lld]\n", raw.sec, raw.nsec);
        return 0;
    }


    // Fetch remote host raw time based on UTC time

    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5214); 

    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 

    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 
    
    getRawFromUtc (utc.epoch, &raw_remote, sockfd);


    ticks = utc.epoch;


    printf("\n\n{raw(local): [%lld.%lld]} {utc: %.24s [%ld sec since epoch]} {raw(remote): [%lld.%lld]}\r\n", raw.sec, raw.nsec, ctime(&ticks), ticks, raw_remote.sec, raw_remote.nsec);



    return 0;
}
