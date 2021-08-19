#ifndef __UD_PHYSICAL_H
#define __UD_PHYSICAL_H

#define UPDI_BAUTRATE_DEFAULT		115200
#define UPDI_BAUTRATE_DOUBLE_BREAK	300

#define UPDI_BAUTRATE_IN_CLK_4M_MAX		225000
#define UPDI_BAUTRATE_IN_CLK_8M_MAX		450000
#define UPDI_BAUTRATE_IN_CLK_16M_MAX	900000

void *updi_physical_init(const char *port, int baud);
void updi_physical_deinit(void *ptr_phy);
int phy_set_baudrate(void *ptr_phy, int baud);
int phy_send_break(void *ptr_phy);
int phy_send_double_break(void *ptr_phy);
int phy_send(void *ptr_phy, const u8 *data, int len);
int phy_send_byte(void *ptr_phy, u8 val);
int phy_receive(void *ptr_phy, u8 *data, int len);
u8 phy_receive_byte(void *ptr_phy);
int phy_transfer(void *ptr_phy, const u8 *wdata, int wlen, u8 *rdata, int rlen);
int phy_sib(void *ptr_phy, u8 *data, int len);

#endif