#ifndef __UD_LINK_H
#define __UD_LINK_H

void *updi_datalink_init(const char *port, int baud, int gaurd);
void updi_datalink_deinit(void *link_ptr);
int link_set_init(void *link_ptr, int baud, int gaurd);
int link_check(void *link_ptr);
int _link_ldcs(void *link_ptr, u8 address, u8 *val);
u8 link_ldcs(void *link_ptr, u8 address);
int link_stcs(void *link_ptr, u8 address, u8 value);
int _link_ld(void *link_ptr, u16 address, u8 *val);
u8 link_ld(void *link_ptr, u16 address);
int _link_ld16(void *link_ptr, u16 address, u16 *val);
u16 link_ld16(void *link_ptr, u16 address);
int link_st(void *link_ptr, u16 address, u8 value);
int link_st16(void *link_ptr, u16 address, u16 value);
int link_ld_ptr_inc(void *link_ptr, u8 *data, int len);
int link_ld_ptr_inc16(void *link_ptr, u8 *data, int len);
int link_st_ptr(void *link_ptr, u16 address);
int link_st_ptr_inc(void *link_ptr, const u8 *data, int len);
int link_st_ptr_inc16(void *link_ptr, const u8 *data, int len);
int link_repeat(void *link_ptr, u8 repeats);
int link_repeat16(void *link_ptr, u16 repeats);
int link_read_sib(void *link_ptr, u8 *data, int len);
int link_key(void *link_ptr, u8 size_k, const char *key);

#endif