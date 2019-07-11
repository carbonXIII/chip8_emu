#ifndef __CHIP8_H__
#define __CHIP8_H__

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

  const char* debug_str[] = {
                             "OP_CLS",
                             "OP_RET",
                             "OP_SYS",
                             "OP_JP",
                             "OP_CALL",
                             "OP_SEb",
                             "OP_SNEb",
                             "OP_SEr",
                             "OP_LDb",
                             "OP_ADDb",
                             "OP_LDr",
                             "OP_ORr",
                             "OP_ANDr",
                             "OP_XORr",
                             "OP_ADDr",
                             "OP_SUBr",
                             "OP_SHR",
                             "OP_SUBN",
                             "OP_SHL",
                             "OP_SNEr",
                             "OP_LDi",
                             "OP_JPv",
                             "OP_RND",
                             "OP_DRW",
                             "OP_SKP",
                             "OP_SKNP",
                             "OP_LDdt",
                             "OP_LDk",
                             "OP_LDxdt",
                             "OP_LDxst",
                             "OP_ADDi",
                             "OP_LDf",
                             "OP_LDbcd",
                             "OP_backup_regs",
                             "OP_restore_regs"
  };

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

    cpu(int pc_start) {
      sp = 0;
      pc = pc_start;
    }

    int combine(uint16_t up, uint16_t lo) {
      int ret = lo;
      ret <<= 4;
      ret |= up;
      return ret;
    }

    uint16_t endian_swap(uint16_t in) {
      uint16_t ret = 0;
      for(int i = 0; i < 16; i++) {
        ret <<= 1;
        if(in & 1) ret |= 1;
        in >>= 1;
      }
      return ret;
    }

    template <typename addressable_t>
    instr fetch_and_decode(addressable_t* mem) {
      uint8_t up = mem->get(pc++);
      uint8_t lo = mem->get(pc++);
      printf("%03x: %02x%02x\n", pc-2, up, lo);

      instr ret;
      switch(up >> 4) {
      case 0:
        if(up == 0 && lo == 0xE0) { ret.op = OP_CLS; break; }
        if(up == 0 && lo == 0xEE) { ret.op = OP_RET; break; }
        { ret.op = OP_SYS; ret.arg0 = (up & 0xF) << 8 | lo; break; }
      case 1:
        { ret.op = OP_JP; ret.arg0 = (up & 0xF) << 8 | lo; break; }
      case 2:
        { ret.op = OP_CALL; ret.arg0 = (up & 0xF) << 8 | lo; break; }
      case 3:
        { ret.op = OP_SEb; ret.arg0 = up & 0xF; ret.arg1 = lo; break; }
      case 4:
        { ret.op = OP_SNEb; ret.arg0 = up & 0xF; ret.arg1 = lo; break; }
      case 5:
        { ret.op = OP_SEr; ret.arg0 = up & 0xF; ret.arg1 = lo >> 4; break; }
      case 6:
        { ret.op = OP_LDb; ret.arg0 = up & 0xF; ret.arg1 = lo; break; }
      case 7:
        { ret.op = OP_ADDb; ret.arg0 = up & 0xF; ret.arg1 = lo; break; }
      case 8:
        {
          ret.arg0 = up & 0xF; ret.arg1 = lo >> 4;
          if((lo & 0xF) == 0) { ret.op = OP_LDr; }
          if((lo & 0xF) == 1) { ret.op = OP_ORr; }
          if((lo & 0xF) == 2) { ret.op = OP_ANDr; }
          if((lo & 0xF) == 3) { ret.op = OP_XORr; }
          if((lo & 0xF) == 4) { ret.op = OP_ADDr; }
          if((lo & 0xF) == 5) { ret.op = OP_SUBr; }
          if((lo & 0xF) == 6) { ret.op = OP_SHR; }
          if((lo & 0xF) == 7) { ret.op = OP_SUBN; }
          if((lo & 0xF) == 0xE) { ret.op = OP_SHL; }
          break;
        }
      case 9:
        { ret.op = OP_SNEr; ret.arg0 = up & 0xF; ret.arg1 = lo >> 4; break; }
      case 0xA:
        { ret.op = OP_LDi; ret.arg0 = (up & 0xF) << 8 | lo; break; }
      case 0xB:
        { ret.op = OP_JPv; ret.arg0 = (up & 0xF) << 8 | lo; break; }
      case 0xC:
        { ret.op = OP_RND; ret.arg0 = up & 0xF; ret.arg1 = lo; break; }
      case 0xD:
        { ret.op = OP_DRW; ret.arg0 = up & 0xF; ret.arg1 = lo >> 4; ret.arg2 = lo & 0xF; break; }
      case 0xE:
        {
          ret.arg0 = up & 0xF;

          if(lo == 0x9E) { ret.op = OP_SKP; }
          if(lo == 0xA1) { ret.op = OP_SKNP; }
          break;
        }
      case 0xF:
        {
          ret.arg0 = up & 0xF;

          if(lo == 0x07) { ret.op = OP_LDdt; }
          if(lo == 0x0A) { ret.op = OP_LDk; }
          if(lo == 0x15) { ret.op = OP_LDxdt; }
          if(lo == 0x18) { ret.op = OP_LDxst; }
          if(lo == 0x1E) { ret.op = OP_ADDi; }
          if(lo == 0x29) { ret.op = OP_LDf; }
          if(lo == 0x33) { ret.op = OP_LDbcd; }
          if(lo == 0x55) { ret.op = OP_backup_regs; }
          if(lo == 0x65) { ret.op = OP_restore_regs; }

          break;
        }
      }

      return ret;
    }

    template <typename addressable_t, typename runtime_t>
    void update(addressable_t* mem,
                runtime_t* r) {
      instr i = fetch_and_decode(mem);

      std::cerr << i.to_string() << std::endl;

      switch(i.op) {
      case OP_CLS: { r->clear(); break; }
      case OP_RET: { pc = stack[--sp]; break; }
      case OP_SYS: { break; }
      case OP_JP: { pc = i.arg0; break; }
      case OP_CALL: { stack[sp++] = pc; pc = i.arg0; break; }
      case OP_SEb: { printf("%d == %d\n", v[i.arg0], i.arg1); if(v[i.arg0] == i.arg1) pc+=2; break; }
      case OP_SNEb: { if(v[i.arg0] != i.arg1) pc+=2; break; }
      case OP_SEr: { if(v[i.arg0] == v[i.arg1]) pc+=2; break; }
      case OP_LDb: { v[i.arg0] = i.arg1; break; }
      case OP_ADDb: { v[i.arg0] += i.arg1; break; }
      case OP_LDr: { v[i.arg0] = v[i.arg1]; break; }
      case OP_ORr: { v[i.arg0] |= v[i.arg1]; break; }
      case OP_ANDr: { v[i.arg0] &= v[i.arg1]; break; }
      case OP_XORr: { v[i.arg0] ^= v[i.arg1]; break; }
      case OP_ADDr:
        {
          int result = v[i.arg0];
          result += v[i.arg1];
          v[0xF] = result > 0xFF;
          v[i.arg0] = result;
          break;
        }

      case OP_SUBr: {
        v[0xF] = v[i.arg0] > v[i.arg1];
        v[i.arg0] -= v[i.arg1];
        break;
      }

      case OP_SHR: {
        v[0xF] = v[i.arg0] & 1;
        v[i.arg0] >>= 1;
        break;
      }

      case OP_SUBN: {
        v[0xF] = v[i.arg1] > v[i.arg0];
        v[i.arg0] = v[i.arg1] - v[i.arg0];
        break;
      }

      case OP_SHL: {
        v[0xF] = v[i.arg0] >> 7;
        v[i.arg0] <<= 1;
        break;
      }

      case OP_SNEr: { if(v[i.arg0] != v[i.arg1]) pc+=2; break; }
      case OP_LDi: { I = i.arg0; break; }
      case OP_JPv: { pc = v[0] + i.arg0; break; }
      case OP_RND: { v[i.arg0] = r->rand() & i.arg1; break; }
      case OP_DRW: { v[0xF] = r->draw(I, i.arg2, v[i.arg0], v[i.arg1]); break; }
      case OP_SKP: { if(r->get_key(v[i.arg0])) pc += 2; break; }
      case OP_SKNP: { if(!r->get_key(v[i.arg0])) pc += 2; break; }
      case OP_LDdt: { v[i.arg0] = r->delay_timer(); break; }
      case OP_LDk: { v[i.arg0] = r->wait_key(); break; }
      case OP_LDxdt: { r->delay_timer(v[i.arg0]); break; }
      case OP_LDxst: { r->sound_timer(v[i.arg0]); break; }
      case OP_ADDi: { I += v[i.arg0]; break; }
      case OP_LDf: { I = r->digit_sprite(v[i.arg0]); break; }
      case OP_LDbcd: { mem->write(I, r->bcd(v[i.arg0]), 3); break; }
      case OP_backup_regs: { mem->write(I, v, i.arg0); break; }
      case OP_restore_regs: { mem->read(I, v, i.arg0); break; }
      default:
        {
          std::cerr << "ignoring unknown instr: " << i.op << std::endl;
          break;
        };
      }
    }
  };

  struct dram: addressable {
    static const int SIZE = 4096;
    static const int ROM_START = 0x200;

    uint8_t* data;

    dram() {
      data = new uint8_t[SIZE];
    }

    void write(int addr, void* buf, int count) {
      assert(addr + count < SIZE);
      memcpy(data + addr, buf, count);
    }

    void read(int addr, void* buf, int count) {
      assert(addr + count < SIZE);
      memcpy(buf, data + addr, count);
    }

    template <typename itt>
    void write(int addr, itt* it, int count) {
      uint8_t* d = data + addr;
      while(count--) {
        *(d++) = (uint8_t)*(it++);
      }
    }

    uint8_t get(int addr) {
      assert(addr < SIZE);
      return data[addr];
    }
  };

  struct debug_runtime: runtime {
    uint8_t dt;
    uint8_t st;

    debug_runtime(): dt(0), st(0) {}

    void clear() {
      printf("clear\n");
    }

    uint8_t rand() {
      return ::rand();
    }

    bool draw(int addr, int n, int x, int y) {
      printf("draw %x, %d, %d, %d\n", addr, n, x, y);
      return 0;
    }

    bool get_key(int key) {
      printf("querying key: %d\n", key);
      return 0;
    }

    int wait_key() {
      printf("wait key\n");
      return 0;
    }

    uint8_t delay_timer() {
      return dt;
    }

    uint8_t delay_timer(uint8_t val) {
      return (dt = val);
    }

    uint8_t sound_timer(uint8_t val) {
      return (st = val);
    }

    int digit_sprite(int digit) {
      return digit * 16;
    }

    uint8_t* bcd(int digit) {
      static uint8_t temp[3];
      temp[2] = digit % 10; digit /= 10;
      temp[1] = digit % 10; digit /= 10;
      temp[0] = digit % 10; digit /= 10;
      return temp;
    }

    void update_timers(int t = 1) {
      if(dt)dt--;
      if(st)st--;
      dt = dt > t ? dt - t : 0;
      st = st > t ? st - t : 0;
    }
  };
}

#endif //__CHIP8_H__
