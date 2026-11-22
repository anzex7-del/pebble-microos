#include "pebble_link.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static bool command(const char *s) { pb_command c; return pb_command_parse((const uint8_t *)s,strlen(s),&c); }
static void clock_check(uint64_t epoch,int offset,const char *time,const char *date) {
    char t[9],d[24]; pb_clock_text(epoch,offset,t,d); assert(!strcmp(t,time)); assert(!strcmp(d,date));
}
static void ntp_reply(uint8_t *data,uint64_t epoch) {
    uint32_t stamp=(uint32_t)(epoch+2208988800ULL);
    for(unsigned i=0;i<4;i++) data[40+i]=(uint8_t)(stamp>>(24-i*8));
}
int main(void) {
    const char *valid[]={"P","NMilo","NAnz the Pebble","C0","C3","E0","E2","F5","F60","Z-720","Z840","Z345","T1704067200","T4102444799"};
    const char *bad[]={"","P1","N","N name","Nname ","N<script>","N12345678901234567","C4","E3","F4","F61","Z-735","Z855","Z1","Z-","Z+240","T1704067199","T4102444800","T999999999999999999999","X1","C-1","C1a"};
    for(unsigned i=0;i<sizeof(valid)/sizeof(*valid);i++) assert(command(valid[i]));
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++) assert(!command(bad[i]));
    pb_command c; assert(pb_command_parse((const uint8_t *)"NMilo",5,&c)&&!strcmp(c.name,"Milo"));
    assert(pb_command_parse((const uint8_t *)"Z-720",5,&c)&&c.value==-720);
    assert(!pb_command_parse(NULL,1,&c)); assert(!pb_command_parse((const uint8_t *)"P",1,NULL));
    assert(pb_wifi_credentials_valid("home","")); assert(pb_wifi_credentials_valid("home","12345678"));
    assert(!pb_wifi_credentials_valid("","12345678")); assert(!pb_wifi_credentials_valid("home","short"));
    assert(!pb_wifi_credentials_valid("123456789012345678901234567890123","password"));
    char key[65]; memset(key,'a',64); key[64]=0; assert(pb_wifi_credentials_valid("home",key)); key[50]='z'; assert(!pb_wifi_credentials_valid("home",key));
    clock_check(1704067200ULL,240,"04:00:00","2024-01-01");
    clock_check(1704067200ULL,-300,"19:00:00","2023-12-31");
    clock_check(1709164800ULL,0,"00:00:00","2024-02-29");
    clock_check(1709251200ULL,345,"05:45:00","2024-03-01");
    clock_check(4102444799ULL,0,"23:59:59","2099-12-31");
    uint8_t nonce[8]={1,2,3,4,5,6,7,8}, packet[48]={0}; uint64_t epoch=0;
    packet[0]=0x24; packet[1]=2; memcpy(packet+24,nonce,8); ntp_reply(packet,1704067200ULL);
    assert(pb_ntp_parse(packet,48,nonce,&epoch)&&epoch==1704067200ULL);
    ntp_reply(packet,2208988800ULL); assert(pb_ntp_parse(packet,48,nonce,&epoch)&&epoch==2208988800ULL);
    ntp_reply(packet,4102444799ULL); assert(pb_ntp_parse(packet,48,nonce,&epoch)&&epoch==4102444799ULL);
    assert(!pb_ntp_parse(packet,47,nonce,&epoch)); packet[24]^=1; assert(!pb_ntp_parse(packet,48,nonce,&epoch)); packet[24]^=1;
    packet[0]=0xe4; assert(!pb_ntp_parse(packet,48,nonce,&epoch)); packet[0]=0x23; assert(!pb_ntp_parse(packet,48,nonce,&epoch)); packet[0]=0x24;
    packet[1]=0; assert(!pb_ntp_parse(packet,48,nonce,&epoch)); packet[1]=16; assert(!pb_ntp_parse(packet,48,nonce,&epoch));
    puts("Clock, NTP validation, Wi-Fi credentials and BLE protocol tests passed.");
    return 0;
}
