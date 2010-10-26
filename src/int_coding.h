#ifndef INT_CODING
#define INT_CODING

static inline void int_cpy(uint8_t *p, int v)
{
  uint32_t tmp;
  
  tmp = htonl(v);
  memcpy(p, &tmp, 4);
}

static inline uint32_t int_rcpy(const uint8_t *p)
{
  uint32_t tmp;
  
  memcpy(&tmp, p, 4);
  tmp = ntohl(tmp);

  return tmp;
}
#endif	/* INT_CODING */
