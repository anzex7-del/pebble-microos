#include "pebble_link.h"
#include <stdio.h>
#include <string.h>

/* LF terminates a frame. After any bad byte or overflow, discard the whole
 * frame so its suffix cannot be mistaken for another command. */
bool pb_usb_line_feed(pb_usb_line *line, uint8_t byte) {
    if (byte == '\n') {
        bool complete = !line->discard && line->length > 0;
        line->text[line->length] = 0;
        line->length = 0;
        line->discard = false;
        return complete;
    }
    if (byte < 32 || byte > 126 || line->length >= sizeof(line->text) - 1) line->discard = true;
    if (!line->discard) line->text[line->length++] = (char)byte;
    return false;
}

bool pb_command_parse(const uint8_t *data, size_t len, pb_command *out) {
    if (!data || !out || !len || len > PB_LINK_MAX) return false;
    pb_command c = {0}; c.kind = (char)data[0];
    if (c.kind == 'P') { if (len != 1) return false; *out = c; return true; }
    if (c.kind == 'N') {
        if (len < 2 || len > 17 || data[1] == ' ' || data[len-1] == ' ') return false;
        for (size_t i=1; i<len; i++) {
            uint8_t ch=data[i];
            bool alnum=(ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||(ch>='0'&&ch<='9');
            if (!alnum && (i==1 || (ch!=' '&&ch!='-'&&ch!='_'))) return false;
            c.name[i-1]=(char)ch;
        }
        *out=c; return true;
    }
    if (len < 2 || len > 12) return false;
    size_t start=1; int sign=1;
    if (c.kind=='Z' && data[1]=='-') { start=2; sign=-1; }
    if (start==len) return false;
    for (size_t i=start; i<len; i++) {
        if (data[i]<'0'||data[i]>'9') return false;
        c.value=c.value*10+(data[i]-'0');
    }
    c.value*=sign;
    switch(c.kind) {
    case 'C': if(c.value>3) return false; break;
    case 'E': if(c.value>2) return false; break;
    case 'F': if(c.value<5||c.value>60) return false; break;
    case 'Z': if(c.value < -720 || c.value > 840 || c.value%15) return false; break;
    case 'T': if(c.value<1704067200LL||c.value>4102444799LL) return false; break;
    default: return false;
    }
    *out=c; return true;
}

bool pb_ntp_parse(const uint8_t *data, size_t len, const uint8_t nonce[8], uint64_t *unix_time) {
    if(!data||!nonce||!unix_time||len<48) return false;
    unsigned version=(data[0]>>3)&7;
    if((data[0]>>6)==3 || (version!=3&&version!=4) || (data[0]&7)!=4 || data[1]<1 || data[1]>15) return false;
    if(memcmp(data+24,nonce,8)) return false;
    uint64_t seconds=((uint32_t)data[40]<<24)|((uint32_t)data[41]<<16)|((uint32_t)data[42]<<8)|data[43];
    if(seconds<2208988800ULL) seconds+=4294967296ULL; /* NTP era rollover in 2036. */
    seconds-=2208988800ULL;
    if(seconds<1704067200ULL||seconds>4102444799ULL) return false;
    *unix_time=seconds; return true;
}

bool pb_wifi_credentials_valid(const char *ssid, const char *password) {
    if(!ssid||!password) return false;
    size_t n=strlen(ssid), p=strlen(password);
    if(n<1||n>32||p>64||(p>0&&p<8)) return false;
    if(p==64) for(size_t i=0;i<p;i++) if(!((password[i]>='0'&&password[i]<='9')||(password[i]>='a'&&password[i]<='f')||(password[i]>='A'&&password[i]<='F'))) return false;
    return true;
}

void pb_clock_text(uint64_t epoch, int timezone_minutes, char time_text[9], char date_text[24]) {
    int64_t local=(int64_t)epoch+timezone_minutes*60LL;
    if(local<0) local=0;
    unsigned seconds=(unsigned)(local%86400);
    snprintf(time_text,9,"%02u:%02u:%02u",seconds/3600,seconds/60%60,seconds%60);
    /* Gregorian civil date from days since Unix epoch. */
    int64_t z=local/86400+719468;
    int era=(int)(z/146097); unsigned doe=(unsigned)(z-era*146097LL);
    unsigned yoe=(doe-doe/1460+doe/36524-doe/146096)/365;
    int year=(int)yoe+era*400; unsigned doy=doe-(365*yoe+yoe/4-yoe/100);
    unsigned mp=(5*doy+2)/153, day=doy-(153*mp+2)/5+1;
    int month=(int)mp+(mp<10?3:-9); year+=(month<=2);
    snprintf(date_text,24,"%04d-%02d-%02u",year,month,day);
}
