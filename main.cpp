#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

#include "sdl.h"
#include "chip8.h"

using namespace std;

template <typename T>
T input(const char* msg) {
  cout << msg;
  T ret;
  cin >> ret;
  return ret;
}

int main() {
  chip8::render_window win(1280,640);

  // auto view =  win.add_view();

  // uint8_t* pixels = (uint8_t*)view->lock();

  // cout << view->pitch() << endl;

  // for(int i = 0; i < 200; i++) {
  //   for(int j = 0; j < 200; j++) {
  //     pixels[i * view->pitch() + j * 4] = i;
  //     pixels[i * view->pitch() + j * 4 + 1] = j;
  //     pixels[i * view->pitch() + j * 4 + 2] = 127;
  //     pixels[i * view->pitch() + j * 4 + 3] = 127;
  //   }
  // }

  // view->unlock();

  // win.update();

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

  bool redraw = true;

  while(win.update(redraw)) {
    cpu.update(&ram, &runtime);

    std::this_thread::sleep_for(std::chrono::milliseconds(2));

    acc += clock() - last;
    runtime.update_timers(acc / timer_interval);
    acc %= timer_interval;


    redraw = true;

    last = clock();
  }

  return 0;
}
