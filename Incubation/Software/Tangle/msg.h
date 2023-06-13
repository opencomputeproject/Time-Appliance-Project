/*
 *  Authors: 
 *       Lakshmi Pradeep <lpradeep@meta.com>
 *       Hari Prasad Kalavakunta <hkalavakunta@meta.com>
 */

typedef unsigned long long int u64_t;
typedef unsigned int u32_t;
typedef u32_t tangle_msg_type_t;

/* Request messages */
#define GET_UTC_FROM_RAW_REQ 2001
#define GET_RAW_FROM_UTC_REQ 2002

/* Response Messages */
#define  GET_UTC_FROM_RAW_RES 3001
#define  GET_RAW_FROM_UTC_RES 3002


typedef struct {
   u32_t epoch;
}utc_time_t;

typedef struct {
   u64_t sec;
   u64_t nsec;
}raw_time_t;

typedef union {
   utc_time_t utc_time;
   raw_time_t raw_time;
}  tangle_msg_input_t;

      
typedef struct tangle_api {
  tangle_msg_type_t msg;
  tangle_msg_input_t val;
} tangle_api_t;
