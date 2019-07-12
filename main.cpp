#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include "sdl.h"
#include "core.h"

using namespace std;

template <typename T>
T input(const char* msg) {
  cout << msg;
  T ret;
  cin >> ret;
  return ret;
}

int state = 0;
int print = 0;
int print_regs = 0;

void handle_key(int key) {
  switch(key) {
  case SDLK_RETURN: { state = (state < 0) ? 0 : -1; break; }
  case SDLK_f: { if(state < 0) state = 1; print = 1; break; }
  case SDLK_r: { print_regs = 1; }
  }
}

int main() {
  chip8::render_window win(1280,640);
  win.register_listener(handle_key);

  chip8::cpu cpu(chip8::dram::ROM_START);
  chip8::dram ram;
  // chip8::debug_runtime runtime;

  chip8::render_window::view* view = win.add_view(win.create_rect(0, 0, 64, 32));
  view->scale(1280,640);
  chip8::sdl_runtime<chip8::dram> runtime(&ram, view);

  runtime.clear();

  // load the rom
  {
    string rom_path = input<string>("ROM path: ");
    cout << endl;

    ifstream rom_in(rom_path.c_str(), ios::binary | ios::ate);

    int size = rom_in.tellg();
    rom_in.seekg(ios::beg);

    uint8_t* rom = new uint8_t[size];
    rom_in.read((char*)rom, size);
    ram.write(chip8::dram::ROM_START, rom, size);
    delete [] rom;
  }

  int acc = 0;
  clock_t last = clock();
  int timer_interval = CLOCKS_PER_SEC / 60;
  assert(timer_interval);

  while(win.update()) {
    if(state >= 0) {
      cpu.update(&ram, &runtime, print);
    }

    if(print_regs) {
      cpu.dump_regs(cout);
      print_regs = 0;
    }

    if(print) print = 0;
    if(state == 1) state = -1;

    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    acc += clock() - last;
    runtime.update_timers(acc / timer_interval);
    acc %= timer_interval;

    last = clock();
  }

  return 0;
}
