#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include "msg.h"

enum { CAPACITY = 10000 };  

void init_tangle_wheel(void);


typedef struct tangle_data {
   struct timespec     raw_clock_time;
   //struct tm           utc_time;
   
   //system time. User can UTC time corresponding to the system time by calling gmtime
   time_t              utc_time;
} tangle_data_t;

tangle_data_t *tangle_wheel;   // Note that CAPACITY-1 is the actual capacity

int writeIndx = 0;
int readIndx = 0;


void init_tangle_wheel (void)
{
   int i;

   tangle_wheel = (tangle_data_t*)calloc(CAPACITY,  sizeof(tangle_data_t));

   if (tangle_wheel == NULL){
      printf ("\n[%s:%d] Error: Unable to allocate memory for tangle wheel", __FILE__, __LINE__);
   }
   return;
}
   


int add (tangle_data_t *item)
{
  
  tangle_wheel[writeIndx].raw_clock_time = item->raw_clock_time;
  tangle_wheel[writeIndx].utc_time = item->utc_time;
 
  writeIndx = (writeIndx + 1) % CAPACITY;
  if (writeIndx == 0)
  {
     // Condition that signals ring full of records
     readIndx = -1;
  }
}

tangle_data_t *retrieve (int index)
{
  return (&tangle_wheel[index]);
}

void showWheel(void)
{
   int i;
   for (i = 0; i < CAPACITY; i++)
   {
      printf ("[%s:%d] Info: RAW CLK TIME: %lld.%.9ld; UTC Time: %ld\n", __FILE__, __LINE__,
                    (long long)tangle_wheel[i].raw_clock_time.tv_sec, tangle_wheel[i].raw_clock_time.tv_nsec,
                    tangle_wheel[i].utc_time);

   }
}

void * keepTime (void *arg)
{
   static unsigned int poll_interval = 0;
   time_t t;
   struct tm *utc;
   tangle_data_t d_item;
   while (1)
   {
      time(&d_item.utc_time);

      clock_gettime(CLOCK_MONOTONIC_RAW, &d_item.raw_clock_time);

      add (&d_item);

#ifdef DEBUG_WHEEL
      if (poll_interval++ % CAPACITY == 0)
        showWheel();
#endif
   

      sleep(1);
   }
   
}

int getclosematch_utc(int s, int e, u64_t sec, u64_t nsec)
{
   int i, min_i, num_entries;

   u64_t delta, min_delta;
  
   min_delta = delta = abs(sec - tangle_wheel[s].raw_clock_time.tv_sec);

   num_entries = (e - s + CAPACITY) % CAPACITY;

   for ( i = s, min_i = s; num_entries > 0 ; num_entries--)
   {
      delta = abs(sec - tangle_wheel[i].raw_clock_time.tv_sec);
      if (delta < min_delta)
      {
	 min_delta = delta;
	  
         min_i = i;
      }
         

      i = (i + 1) %CAPACITY;
   }
   return (min_i);
}

int getclosematch_raw(int s, int e, u32_t epoch)
{
   int i, min_i, num_entries;

   u64_t delta, min_delta;
  
   min_delta = delta = abs(epoch - tangle_wheel[s].utc_time);

   num_entries = (e - s + CAPACITY) % CAPACITY;

   for ( i = s, min_i = s; num_entries > 0; num_entries--)
   {
      delta = abs(epoch - tangle_wheel[i].utc_time);
      if (delta < min_delta)
      {
	 min_delta = delta;
	  
         min_i = i;
      }
         

      i = (i + 1) %CAPACITY;
   }
   return (min_i);
}

void printmsg(tangle_api_t *api){
  printf ("\nInfo: MSG_TYPE = %d\n", api->msg);
  if (api->msg == GET_RAW_FROM_UTC_REQ)
  {
     printf ("\tInfo: utc time = %d\n", api->val.utc_time.epoch);
  }
  else if(api->msg == GET_UTC_FROM_RAW_REQ)
  {
     printf ("\tInfo: raw time(s) = %llu\n", api->val.raw_time.sec);
     printf ("\tInfo: raw time(ns) = %llu\n", api->val.raw_time.nsec);
  }
  else
  {
     printf ("\tInfo: unknown api\n");
  }
}

int constructAndSendUtcTimeResponse(u32_t epoch, int fd)
{
   tangle_api_t api;
   int nBytes = 0;

   api.msg = GET_UTC_FROM_RAW_RES;
   api.val.utc_time.epoch = epoch;

   
   if ((nBytes = write(fd, &api, sizeof(api))) < 0)
   {
      printf("\n[%s:%d] Error: Write error\n", __FILE__, __LINE__);
      return 0;
   }
   return nBytes;
}

int constructAndSendRawTimeResponse(u64_t sec, u64_t nsec, int fd)
{
   tangle_api_t api;
   int nBytes = 0;

   api.msg = GET_RAW_FROM_UTC_RES;
   api.val.raw_time.sec = sec;
   api.val.raw_time.nsec = nsec;

   if ((nBytes = write(fd, &api, sizeof(api))) < 0)
   {
      printf("\n[%s:%d] Error: Write error\n", __FILE__, __LINE__);
      return 0;
   }
   return nBytes;
}


int getIndexFromUtc (u32_t epoch)
{
    int idx_s, idx_e, idx_m;

    // Index pointed by writeIndx is the oldest record.
    // We pick 3 rd oldest record begin the search
    u32_t utc_time_from_oldest_record = tangle_wheel[(writeIndx + 2) % CAPACITY].utc_time;
    u32_t utc_time_from_recent_record = tangle_wheel[(writeIndx + CAPACITY - 1) % CAPACITY].utc_time;

    if (readIndx == 0)
    {
       utc_time_from_oldest_record = tangle_wheel[0].utc_time;
    }

    if ((epoch < utc_time_from_oldest_record) || epoch > utc_time_from_recent_record ) 
    {
       printf ("\n[%s: %d] Error: Time information provided [%ld] is outside the " \
               "tracking range [%ld, %ld]\n", __FILE__, __LINE__, 
                epoch, utc_time_from_oldest_record, utc_time_from_recent_record);
       return (-1);
    }

    idx_m = (writeIndx + CAPACITY - (utc_time_from_recent_record - epoch)) % CAPACITY;
    idx_s = (idx_m + CAPACITY -5) % CAPACITY;
    idx_e = (idx_m + 5) % CAPACITY;
    if ((idx_s > idx_e) && (readIndx == 0)){
        idx_s = 0;
    }

    return (getclosematch_raw(idx_s, idx_e, epoch));
}


int getIndexFromRaw (raw_time_t reqtime)
{
    int idx_s, idx_e, idx_m;

    // Index pointed by writeIndx is the oldest record.
    // We pick 3 rd oldest record begin the search
    struct timespec raw_time_from_oldest_record = tangle_wheel[(writeIndx + 2) % CAPACITY].raw_clock_time;
    struct timespec raw_time_from_recent_record = tangle_wheel[(writeIndx + CAPACITY - 1) % CAPACITY].raw_clock_time;
    if (readIndx == 0)
    {
       raw_time_from_oldest_record = tangle_wheel[0].raw_clock_time;
    }
    printf ("[%s:%d] Info: write->[%d]: req time = %llu, oldrec time = %llu, recentrec time = %llu\n", __FILE__, __LINE__, 
           writeIndx, reqtime.sec, raw_time_from_oldest_record.tv_sec, raw_time_from_recent_record.tv_sec); 

    if ((reqtime.sec < raw_time_from_oldest_record.tv_sec) || (reqtime.sec > raw_time_from_recent_record.tv_sec)) 
    {

       printf ("\n[%s:%d] Error: Time information provided [%llu.%llu] is outside the "\
               "tracking range [%llu.%llu, %llu.%llu]\n", __FILE__, __LINE__,
                reqtime.sec, reqtime.nsec,
                raw_time_from_oldest_record.tv_sec, raw_time_from_oldest_record.tv_nsec,
                raw_time_from_recent_record.tv_sec, raw_time_from_recent_record.tv_nsec);
       return (-1);
    }

    idx_m = (writeIndx + CAPACITY - (raw_time_from_recent_record.tv_sec - reqtime.sec)) % CAPACITY;
    idx_s = (idx_m + CAPACITY -5) % CAPACITY;
    idx_e = (idx_m + 5) % CAPACITY;
  
    if ((idx_s > idx_e) && (readIndx == 0)){
        idx_s = 0;
    }

    return (getclosematch_utc(idx_s, idx_e, reqtime.sec, reqtime.nsec));
}

int handlerequest (tangle_api_t *api, int wfd)
{
#ifdef DEBUG_API
    printmsg(api);
#endif
    switch (api->msg)
    {
        case GET_RAW_FROM_UTC_REQ:
        {
           int index;

           index = getIndexFromUtc (api->val.utc_time.epoch);
           if (index >= 0 )
           {
              constructAndSendRawTimeResponse(tangle_wheel[index].raw_clock_time.tv_sec, tangle_wheel[index].raw_clock_time.tv_nsec, wfd);

              printf ("[%s: %d] Info: Best Match@[%d]: req UTC time = %ld maps to RAW time = %llu.%llu \n", __FILE__, __LINE__,
                  index, api->val.utc_time.epoch, tangle_wheel[index].raw_clock_time.tv_sec, tangle_wheel[index].raw_clock_time.tv_nsec);

           }
           else
           {
              constructAndSendRawTimeResponse(0, 0, wfd);
           }
           return (index);
        }

        case GET_UTC_FROM_RAW_REQ:
        {
           int index;
           time_t ticks;

           index = getIndexFromRaw (api->val.raw_time);

           if (index >= 0 )
           {
               constructAndSendUtcTimeResponse(tangle_wheel[index].utc_time, wfd);

               printf ("[%s: %d] Info: Best Match@[%d]: req RAW time = %llu.%llu maps UTC time = %ld \n", __FILE__, __LINE__,
                   index, api->val.raw_time.sec, api->val.raw_time.nsec, tangle_wheel[index].utc_time);

           }
           else
           {
               constructAndSendUtcTimeResponse(0, wfd);
           }
           return (index);

	}
  
        default:
           return (-1);
           printf("\n[%s:%d] Error. Unknown msg type %u\n", __FILE__, __LINE__, api->msg);
    }


}


int getmsg(int *fd)
{
    int bytesRead = 0;
    int result;
    int msg_len = sizeof(tangle_api_t);
    tangle_api_t buffer;

    while (bytesRead < msg_len)
    {
        result = recv(*fd, &buffer + bytesRead, msg_len - bytesRead, 0);
        if (result == 0)
        {
           close(*fd);
           *fd = -1;
           return (0);
           
        }
        else if (result < 0 )
        {
           printf("\n[%s:%d] Error: unable to read\n", __FILE__, __LINE__);
           printf("close client fd =  %d\n", *fd);
           close(*fd);
           return (-1);
        }

        bytesRead += result;
    }
    return handlerequest (&buffer, *fd);
}


void * tangleService (void *arg)
{

#define MAX_CONCURRENT_CLIENTS 10

    int srvfd = 0, connfd = -1, max_fd, index = 0, i, client_socket[MAX_CONCURRENT_CLIENTS];
    struct sockaddr_in serv_addr; 
    fd_set rdfds;


    for (i = 0; i < MAX_CONCURRENT_CLIENTS; i++)  
    {  
        client_socket[i] = -1;  
    }  

    time_t ticks; 

    srvfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5214); 

    bind(srvfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    listen(srvfd, 10); 


    while (1)
    {
       // Clear all FDs
       FD_ZERO(&rdfds);
       FD_SET(srvfd, &rdfds);  
       max_fd = srvfd;

        //add child sockets to set 
        for ( i = 0 ; i < MAX_CONCURRENT_CLIENTS ; i++)  
        {  
            int fd = client_socket[i];  
                 
            //if valid socket descriptor then add to read list 
            if(fd > 0)  
                FD_SET( fd , &rdfds);  
                 
            //highest file descriptor number, need it for the select function 
            if(fd > max_fd)  
                max_fd = fd;  
        }  

       if ( select( max_fd + 1 , &rdfds , NULL , NULL , NULL) < 0 ) 
            printf ("\n[%s:%d] Error: select \n", __FILE__, __LINE__);

      
       for (i = 0; i < MAX_CONCURRENT_CLIENTS ; i++)  
       {
           if (connfd && FD_ISSET( client_socket[i] , &rdfds))
           {
              if ( (index = getmsg(&client_socket[i])) < 0 ){
                 printf ("\n[%s:%d] Error: no match\n", __FILE__, __LINE__);
                 continue;
              }
           }
       }
       if (FD_ISSET(srvfd, &rdfds))  
       {  
          // New connection request from a client
          connfd = accept(srvfd, (struct sockaddr*)NULL, NULL); 

          FD_SET( connfd, &rdfds);

          if (connfd > max_fd) { 
              max_fd = connfd;
          }

           //add new socket to array of sockets 
            for (i = 0; i < MAX_CONCURRENT_CLIENTS ; i++)  
            {  
                if( client_socket[i] == -1 )  
                {  
                    // Unused entry, add connfd to list of client sockets
                    client_socket[i] = connfd;  
                    break;  
                }  
            } 
       }
    }
}

int main ()
{
  pthread_t  timeK_t;
  pthread_t  clisrvc_t;

  init_tangle_wheel();

  // Thread writing raw time to utc mapping data periofically into the ring buffer
  pthread_create(&timeK_t, NULL, keepTime , NULL);

  // Thread servicing clients with information of UTC time from raw time and vice versa
  pthread_create(&clisrvc_t, NULL, tangleService , NULL);

  pthread_join (timeK_t, NULL);
}
  
