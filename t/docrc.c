#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

uint32_t crc32(uint32_t crc,const unsigned char *data, size_t length) {
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

int main(int argc, char *argv[])
{
   unsigned char data[256];
   size_t        len;
   uint32_t      crc = 0xFFFFFFFF;

   if (argc>1) {
       crc=crc32(crc,argv[1],(size_t)strlen(argv[1]));
   }
   while((len=read(0, data, 255)) > 0) {
        crc=crc32(crc,data,len);
   }
   printf("%08x\n",crc);
   return 0;
}
