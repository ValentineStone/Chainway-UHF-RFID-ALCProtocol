#ifndef CHAINWAY_RFID_TEST_H
#define CHAINWAY_RFID_TEST_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "chainway_rfid.h"
#include "chainway_rfid_types_desc.h"

const char TEST_BUFFER[] = {

  0xc8, 0x8c, 0x00, 0x08, 0x00, 0x08, 0x0d, 0x0a,
  0xa5, 0x5a, 0x00, 0x0b, 0x01, 0x02, 0x00, 0x01, 0x09, 0x0d, 0x0a,

};

size_t sprint_hex(const uint8_t *buff, const size_t len, char *dest,
	const size_t max, const char joiner)
{
	size_t pos = 0;
	for (int i = 0; i < len && pos < max; i++) {
		size_t count = snprintf(dest + pos, max - pos, "%02x", buff[i]);
		pos += count;
		if (joiner && i != len - 1) {
			dest[pos] = joiner;
			pos += 1;
		}
	}
	dest[max - 1] = 0;
	return pos < max ? pos : max;
}

#define PRINTBUFF_LEN 4096
char PRINTBUFF[PRINTBUFF_LEN];

void chainway_rfid_test()
{
  for (int i = 0; i < sizeof(TEST_BUFFER); i++) {
    char ch = TEST_BUFFER[i];
    uint16_t parsed_len = parse(ch,0);
    if (parsed_len) {
      uint8_t* parsed_buff = channel_buffer(0);
      sprint_hex(parsed_buff,parsed_len,PRINTBUFF,PRINTBUFF_LEN,' ');
      printf("PARSED MESSAGE: %s\n", PRINTBUFF);
      printf("PARSED TYPE: %s\n", rfid_message_type_desc[parsed_buff[4]]);
    }
  }
}

#endif//CHAINWAY_RFID_TEST_H