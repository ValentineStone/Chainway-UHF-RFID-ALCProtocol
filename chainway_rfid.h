#ifndef CHAINWAY_RFID_H
#define CHAINWAY_RFID_H

#include <stdint.h>
#include <stdio.h>
#include "chainway_rfid_types.h"

#ifndef CHAINWAY_RFID_CHANNEL_COUNT
#define CHAINWAY_RFID_CHANNEL_COUNT 16
#endif

#ifndef CHAINWAY_RFID_MAX_MESSAGE_LEN
#define CHAINWAY_RFID_MAX_MESSAGE_LEN 65536
#endif

uint8_t calculate_bcc(uint8_t* buff, size_t len) {
  uint8_t bcc = 0;
  for (size_t i = 2; i < len - 3; i++)
    bcc ^= buff[i];
  return bcc;
}

int is_little_endian() {
  uint8_t tester[] = {0,1};
  return *(uint16_t*)(tester) == 1;
}

uint16_t message_frame_len(uint8_t* buff) {
  uint16_t len;
  if (is_little_endian()) {
    len = *(uint16_t*)(buff + 2);
  } else {
    ((uint8_t*)&len)[0] = buff[3];
    ((uint8_t*)&len)[1] = buff[2];
  }
  return len;
}

enum state_t {
  WAITING_FOR_MAGIC,
  WAITING_FOR_MAGIC_8C,
  WAITING_FOR_MAGIC_5A,
  WAITING_FOR_LEN1,
  WAITING_FOR_LEN2,
  WAITING_FOR_TYPE,
  WAITING_FOR_DATA,
  WAITING_FOR_BCC,
  WAITING_FOR_END_0D,
  WAITING_FOR_END_0A,
  
  WAITING_FOR_NEXT
};


enum state_t parse_state[CHAINWAY_RFID_CHANNEL_COUNT];
uint8_t buff[CHAINWAY_RFID_CHANNEL_COUNT][CHAINWAY_RFID_MAX_MESSAGE_LEN];
uint16_t waiting_for_data_n[CHAINWAY_RFID_CHANNEL_COUNT];
uint16_t waiting_for_data_i[CHAINWAY_RFID_CHANNEL_COUNT];

uint8_t* channel_buffer(uint8_t channel) {
  return buff[channel];
}

size_t parse(uint8_t ch, uint8_t channel) {
  int parsed_message = 0;
  enum state_t state_before = parse_state[channel];
  switch (parse_state[channel]) {
    case WAITING_FOR_MAGIC:
      if (ch == 0xc8) {
        buff[channel][0] = ch;
        parse_state[channel] = WAITING_FOR_MAGIC_8C;
      } else if (ch == 0xa5) {
        buff[channel][0] = ch;
        parse_state[channel] = WAITING_FOR_MAGIC_5A;
      }
      break;
    case WAITING_FOR_MAGIC_8C:
      if (ch == 0x8c) {
        buff[channel][1] = ch;
        parse_state[channel] = WAITING_FOR_LEN1;
      } else {
        parse_state[channel] = WAITING_FOR_MAGIC;
      }
      break;
    case WAITING_FOR_MAGIC_5A:
      if (ch == 0x5a) {
        buff[channel][1] = ch;
        parse_state[channel] = WAITING_FOR_LEN1;
      } else {
        parse_state[channel] = WAITING_FOR_MAGIC;
      }
      break;
    case WAITING_FOR_LEN1:
      buff[channel][2] = ch;
      parse_state[channel] = WAITING_FOR_LEN2;
      break;
    case WAITING_FOR_LEN2:
      buff[channel][3] = ch;
      parse_state[channel] = WAITING_FOR_TYPE;
      waiting_for_data_n[channel] = message_frame_len(buff[channel]);
      if (waiting_for_data_n[channel] < 8) {
        // impossibly short frame length, minimum is 8 bytes (no data)
        parse_state[channel] = WAITING_FOR_MAGIC;
      } else {
        waiting_for_data_n[channel] -= 8;
        waiting_for_data_i[channel] = 0;
      }
      break;
    case WAITING_FOR_TYPE:
      buff[channel][4] = ch;
      parse_state[channel] = WAITING_FOR_DATA;
      if (waiting_for_data_n[channel] == 0) {
        // no data, skip to BCC
        parse_state[channel] = WAITING_FOR_BCC;
      }
      break;
    case WAITING_FOR_DATA:
      if (waiting_for_data_i[channel] < waiting_for_data_n[channel]) {
        buff[channel][5 + waiting_for_data_i[channel]] = ch;
        waiting_for_data_i[channel] += 1;
      }
      if (waiting_for_data_i[channel] >= waiting_for_data_n[channel]) {
        parse_state[channel] = WAITING_FOR_BCC;
      }
      break;
    case WAITING_FOR_BCC:
      buff[channel][5 + waiting_for_data_n[channel]] = ch;
      if (1 /* TODO: Check BCC HERE */) {
        parse_state[channel] = WAITING_FOR_END_0D;
      } else {
        // BCC wrong, reset
        parse_state[channel] = WAITING_FOR_MAGIC;
      }
      break;
    case WAITING_FOR_END_0D:
      if (ch == 0x0d) {
        buff[channel][6 + waiting_for_data_n[channel]] = ch;
        parse_state[channel] = WAITING_FOR_END_0A;
      } else {
        // no 0x0D, reset
        parse_state[channel] = WAITING_FOR_MAGIC;
      }
      break;
    case WAITING_FOR_END_0A:
      if (ch == 0x0a) {
        buff[channel][7 + waiting_for_data_n[channel]] = ch;
        parse_state[channel] = WAITING_FOR_MAGIC;
        parsed_message = 1;
      } else {
        // no 0x0A, reset
        parse_state[channel] = WAITING_FOR_MAGIC;
      }
      break;   
  }
  
  //printf("PARSE: %02x state: %d -> %d%s\n", ch, state_before, parse_state[channel], parsed_message ? " PARSED" : "");
  if (parsed_message) {
    size_t len = 8 + waiting_for_data_n[channel];
    uint8_t bcc = calculate_bcc(buff[channel], len);
    if (bcc == buff[channel][len - 3]) {
      return len;
    } else {
      return 0;
    }
  }
  else { 
    return 0;
  }
}

#endif//CHAINWAY_RFID_H