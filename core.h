#ifndef __CORE_H__
#define __CORE_H__

#include <cstdint>
#include <iostream>
#include <cstring>
#include <cassert>

namespace chip8 {
  // contracts that should be fullfilled by object types, though they
  // are not enforced through CRTP

  struct addressable {
    void write(int addr, void* buf, int count) {}
    template <typename itt> void write(int addr, itt it, int count) {}

    void read(int addr, void* buf, int count) {}
    uint8_t get(int addr) {}
  };

  struct runtime {
    void clear() {} // clear the screen
    uint8_t rand() {} // generate a random byte
    bool draw(int addr, int n, int x, int y) {} // draw a sprite
    bool get_key(int key) {} // check if a key is pressed
    int wait_key() {} // wait for a key to be pressed
    uint8_t delay_timer() {} // get the delay timer
    uint8_t delay_timer(uint8_t val) {} // set the delay timer
    uint8_t sound_timer(uint8_t val) {} // set the sound timer
    int digit_sprite(int digit) {} // get the address of the sprite for the given digit
    uint8_t* bcd(int digit) {} // get an array with the bcd values for the given digit (3 byte array)
  };

  extern const char* debug_str[];

  struct cpu {
    struct instr {
      int op;
      uint16_t arg0, arg1, arg2;

      std::string to_string() {
        char buf[256];
        snprintf(buf, 256, "%x(%s): %x %x %x\n", op, debug_str[op], arg0, arg1, arg2);
        return std::string(buf);
      }
    };

    enum {
          OP_CLS,
          OP_RET,
          OP_SYS,
          OP_JP,
          OP_CALL,
          OP_SEb,
          OP_SNEb,
          OP_SEr,
          OP_LDb,
          OP_ADDb,
          OP_LDr,
          OP_ORr,
          OP_ANDr,
          OP_XORr,
          OP_ADDr,
          OP_SUBr,
          OP_SHR,
          OP_SUBN,
          OP_SHL,
          OP_SNEr,
          OP_LDi,
          OP_JPv,
          OP_RND,
          OP_DRW,
          OP_SKP,
          OP_SKNP,
          OP_LDdt,
          OP_LDk,
          OP_LDxdt,
          OP_LDxst,
          OP_ADDi,
          OP_LDf,
          OP_LDbcd,
          OP_backup_regs,
          OP_restore_regs
    };

    uint16_t pc, I;
    uint8_t v[16];

    uint16_t stack[16];
    uint16_t sp;

    cpu(int pc_start);

    template <typename addressable_t>
    instr fetch_and_decode(addressable_t* mem);

    template <typename addressable_t, typename runtime_t>
    void update(addressable_t* mem, runtime_t* r);
  };

  struct dram: addressable {
    static const int SIZE = 4096;
    static const int ROM_START = 0x200;

    uint8_t* data;

    dram();

    void write(int addr, void* buf, int count);
    void read(int addr, void* buf, int count);
    template <typename itt> void write(int addr, itt it, int count);
    uint8_t get(int addr);
  };

  struct debug_runtime: runtime {
    uint8_t dt;
    uint8_t st;

    debug_runtime();

    void clear();
    uint8_t rand();
    bool draw(int addr, int n, int x, int y);
    bool get_key(int key);
    int wait_key();
    uint8_t delay_timer();
    uint8_t delay_timer(uint8_t val);
    uint8_t sound_timer(uint8_t val);
    int digit_sprite(int digit);
    uint8_t* bcd(int digit);
    void update_timers(int t = 1);
  };
}

#endif //__CORE_H__
