#ifndef INT_CODING
#define INT_CODING

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <string.h>

static inline void int_cpy(uint8_t *p, int v)
{
  uint32_t tmp;
  
  tmp = htonl(v);
  memcpy(p, &tmp, 4);
}

static inline void int16_cpy(uint8_t *p, uint16_t v)
{
  uint16_t tmp;

  tmp = htons(v);
  memcpy(p, &tmp, 2);
}

static inline uint32_t int_rcpy(const uint8_t *p)
{
  uint32_t tmp;
  
  memcpy(&tmp, p, 4);
  tmp = ntohl(tmp);

  return tmp;
}

static inline uint16_t int16_rcpy(const uint8_t *p)
{
  uint16_t tmp;

  memcpy(&tmp, p, 2);
  tmp = ntohs(tmp);
  return tmp;
}

#endif	/* INT_CODING */
