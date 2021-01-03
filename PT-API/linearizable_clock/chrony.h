#pragma once

#include <arpa/inet.h>

#include <cmath>

#define PKT_TYPE_CMD_REQUEST 1
#define PKT_TYPE_CMD_REPLY 2
#define PROTO_VERSION_NUMBER 6
#define MAX_PADDING_LENGTH 396
#define DEFAULT_CANDM_PORT 323

typedef struct {
  int32_t f;
} Float;

typedef struct {
  uint32_t tv_sec_high;
  uint32_t tv_sec_low;
  uint32_t tv_nsec;
} Timespec;

typedef struct {
  union {
    uint32_t in4;
    uint8_t in6[16];
    uint32_t id;
  } addr;
  uint16_t family;
  uint16_t _pad;
} IPAddr;

typedef struct {
  int32_t EOR;
} REQ_Null;

typedef struct {
  uint8_t version;  /* Protocol version */
  uint8_t pkt_type; /* What sort of packet this is */
  uint8_t res1;
  uint8_t res2;
  uint16_t command;  /* Which command is being issued */
  uint16_t attempt;  /* How many resends the client has done(count up from zero
                        for same sequence number) */
  uint32_t sequence; /* Client's sequence number */
  uint32_t pad1;
  uint32_t pad2;

  union {
    REQ_Null null;
  } data; /* Command specific parameters */

  /* Padding used to prevent traffic amplification.  It only defines the
     maximum size of the packet, there is no hole after the data field. */
  uint8_t padding[MAX_PADDING_LENGTH];

} CMD_Request;

#define REQ_TRACKING 33

typedef struct {
  uint32_t ref_id;
  IPAddr ip_addr;
  uint16_t stratum;
  uint16_t leap_status;
  Timespec ref_time;
  Float current_correction;
  Float last_offset;
  Float rms_offset;
  Float freq_ppm;
  Float resid_freq_ppm;
  Float skew_ppm;
  Float root_delay;
  Float root_dispersion;
  Float last_update_interval;
  int32_t EOR;
} RPY_Tracking;

typedef struct {
  uint8_t version;
  uint8_t pkt_type;
  uint8_t res1;
  uint8_t res2;
  uint16_t command; /* Which command is being replied to */
  uint16_t reply;   /* Which format of reply this is */
  uint16_t status;  /* Status of command processing */
  uint16_t pad1;    /* Padding for compatibility and 4 byte alignment */
  uint16_t pad2;
  uint16_t pad3;
  uint32_t sequence; /* Echo of client's sequence number */
  uint32_t pad4;
  uint32_t pad5;

  union {
    RPY_Tracking tracking;
  } data; /* Reply specific parameters */
} CMD_Reply;

#define FLOAT_EXP_BITS 7
#define FLOAT_EXP_MIN (-(1 << (FLOAT_EXP_BITS - 1)))
#define FLOAT_EXP_MAX (-FLOAT_EXP_MIN - 1)
#define FLOAT_COEF_BITS ((int)sizeof(int32_t) * 8 - FLOAT_EXP_BITS)
#define FLOAT_COEF_MIN (-(1 << (FLOAT_COEF_BITS - 1)))
#define FLOAT_COEF_MAX (-FLOAT_COEF_MIN - 1)

inline double UTI_FloatNetworkToHost(Float f) {
  int32_t exp, coef;
  uint32_t x;

  x = ntohl(f.f);

  exp = x >> FLOAT_COEF_BITS;
  if (exp >= 1 << (FLOAT_EXP_BITS - 1)) exp -= 1 << FLOAT_EXP_BITS;
  exp -= FLOAT_COEF_BITS;

  coef = x % (1U << FLOAT_COEF_BITS);
  if (coef >= 1 << (FLOAT_COEF_BITS - 1)) coef -= 1 << FLOAT_COEF_BITS;

  return coef * pow(2.0, exp);
}

inline void UTI_NormaliseTimeval(struct timeval *x) {
  /* Reduce tv_usec to within +-1000000 of zero. JGH */
  if ((x->tv_usec >= 1000000) || (x->tv_usec <= -1000000)) {
    x->tv_sec += x->tv_usec / 1000000;
    x->tv_usec = x->tv_usec % 1000000;
  }

  /* Make tv_usec positive. JGH */
  if (x->tv_usec < 0) {
    --x->tv_sec;
    x->tv_usec += 1000000;
  }
}

inline void UTI_DoubleToTimeval(double a, struct timeval *b) {
  double frac_part;

  b->tv_sec = a;
  frac_part = 1.0e6 * (a - b->tv_sec);
  b->tv_usec = frac_part > 0 ? frac_part + 0.5 : frac_part - 0.5;
  UTI_NormaliseTimeval(b);
}