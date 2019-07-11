#include "chip8.h"

#include "SDL2/SDL.h"

#include <vector>
#include <memory>
#include <unordered_map>
#include <bitset>
#include <functional>
#include <atomic>

namespace chip8 {
  struct render_window {
    SDL_Window* window;
    SDL_Renderer* renderer;

    int w, h;

    static void init_sdl() {
      static bool init = false;

      if(!init) {
        init = true;

        // TODO: we don't really need everything
        SDL_Init(SDL_INIT_EVERYTHING);
      }
    }

    static SDL_Rect null_rect() {
      SDL_Rect ret;
      ret.x = ret.y = ret.w = ret.h = 0;
      return ret;
    }

    static SDL_Rect create_rect(int x, int y, int w, int h) {
      SDL_Rect ret;
      ret.x = x;
      ret.y = y;
      ret.w = w;
      ret.h = h;
      return ret;
    }

    struct view {
      SDL_Texture* texture;
      render_window* parent;
      SDL_Rect dest;

      int _pitch;

      void* pixels;

      view(render_window* parent,
           SDL_Rect rect,
           uint32_t pixel_format)
        : parent(parent), dest(rect), _pitch(-1) {
        texture = SDL_CreateTexture(parent->renderer,
                                    pixel_format,
                                    SDL_TEXTUREACCESS_STREAMING,
                                    rect.w, rect.h);
      }

      int pitch() {
        return _pitch;
      }

      void* lock() {
        SDL_LockTexture(texture, 0, &pixels, &_pitch);
        return pixels;
      }

      void unlock() {
        SDL_UnlockTexture(texture);
      }

      void move(int x, int y) {
        dest.x = x;
        dest.y = y;
      }

      void scale(int w, int h) {
        dest.w = w;
        dest.h = h;
      }

      void render() {
        SDL_RenderCopy(parent->renderer, texture, 0, &dest);
      }
    };

    std::vector<std::shared_ptr<view> > views;

    render_window(int w,
                  int h,
                  const char* title = "CHIP8",
                  uint32_t flags = 0)
      : w(w), h(h) {
      init_sdl();

      flags |= SDL_WINDOW_SHOWN;

      window = SDL_CreateWindow(title,
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                w, h, flags);

      renderer = SDL_CreateRenderer(window, -1, 0);
    }

    std::unordered_map<int, bool> key_status;

    std::vector<std::function<void(int)>> listeners;

    void register_listener(std::function<void(int)> l) {
      listeners.push_back(l);
    }

    void key_update(int keycode, bool down) {
      key_status[keycode] = down;

      if(down) {
        for(auto f: listeners) {
          f(keycode);
        }
      }
    }

    bool get_key(int keycode) {
      return key_status[keycode];
    }

    view* add_view(SDL_Rect dest = null_rect(), uint32_t pixel_format = SDL_PIXELFORMAT_BGRA8888) {

      if(dest.w == 0 && dest.h == 0) {
        dest.w = w;
        dest.h = h;
      }

      view* ret = new view(this, dest, pixel_format);
      views.push_back(std::shared_ptr<view>(ret));
      return ret;
    }

    bool update(bool redraw = true) {
      bool ret = true;

      // handle events
      SDL_Event e;
      while(SDL_PollEvent(&e)) {
        switch(e.type) {
        case SDL_KEYDOWN:
          key_update(e.key.keysym.sym, true);
          break;
        case SDL_KEYUP:
          key_update(e.key.keysym.sym, false);
          break;
        case SDL_QUIT:
          ret = false;
        }
      }

      if(redraw) {
        for(auto& v: views) {
          v->render();
        }

        SDL_RenderPresent(renderer);
      }

      return ret;
    }

    ~render_window() {
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
    }
  };

  template <typename addressable_t>
  struct sdl_runtime: debug_runtime {
    addressable_t* mem;
    render_window::view* view;

    int W = 64;
    int H = 32;

    std::vector<int> pixels;

    const uint8_t digits[0x50] = {
                                  0xF0, 0x90, 0x90, 0x90, 0xF0,
                                  0x20, 0x60, 0x20, 0x20, 0x70,
                                  0xF0, 0x10, 0xF0, 0x80, 0xF0,
                                  0xF0, 0x10, 0xF0, 0x10, 0xF0,
                                  0x90, 0x90, 0xF0, 0x10, 0x10,
                                  0xF0, 0x80, 0xF0, 0x10, 0xF0,
                                  0xF0, 0x80, 0xF0, 0x90, 0xF0,
                                  0xF0, 0x10, 0x20, 0x40, 0x40,
                                  0xF0, 0x90, 0xF0, 0x90, 0xF0,
                                  0xF0, 0x90, 0xF0, 0x10, 0xF0,
                                  0xF0, 0x90, 0xF0, 0x90, 0x90,
                                  0xE0, 0x90, 0xE0, 0x90, 0xE0,
                                  0xF0, 0x80, 0x80, 0x80, 0xF0,
                                  0xE0, 0x90, 0x90, 0x90, 0xE0,
                                  0xF0, 0x80, 0xF0, 0x80, 0xF0,
                                  0xF0, 0x80, 0xF0, 0x80, 0x80
    };

    const int digit_base = 0;
    std::atomic<int> last;

    sdl_runtime(addressable_t* mem,
                render_window::view* view)
      : mem(mem), view(view), last(-1), pixels(W * H, 0) {
      mem->write(digit_base, digits, 0x50);
      view->parent->register_listener(std::bind(&sdl_runtime::set_last_key, this, std::placeholders::_1));
    }


    constexpr uint32_t get_pixel(bool val) {
      return (val ? -1 : 0) | 0xFF;
    }

    void clear() {
      uint32_t* p = (uint32_t*)view->lock();
      int stride = view->pitch() / sizeof(uint32_t);

      for(int i = 0; i < H; i++) {
        for(int j = 0; j < W; j++) {
          p[i * stride + j] = get_pixel(0);
          pixels[i * 64 + j] = 0;
        }
      }
      view->unlock();
    }

    bool draw(int addr, int n, int x, int y) {
      uint32_t* p = (uint32_t*)view->lock();
      int stride = view->pitch() / sizeof(uint32_t);

      std::cout << x << ", " << y << std::endl;

      bool ret = false;
      for(int i = 0; i < n; i++) {
        auto sprite = std::bitset<8>(mem->get(addr + i));
        for(int j = 0; j < 8; j++) {
          int X = (j + x) % W;
          int Y = (i + y) % H;
          bool existing = pixels[Y * 64 + X];
          bool to_add = sprite[7 - j];

          if(existing && to_add)
            ret = true;

          pixels[Y * 64 + X] = existing ^ to_add;
          p[Y * stride + X] = get_pixel(pixels[Y * 64 + X]);
        }
      }

      std::cout << "drawn: " << std::endl;
      for(int i = 0; i < n; i++) {
        std::cout << std::bitset<8>(mem->get(addr + i)) << std::endl;
      }

      view->unlock();
      return ret;
    }

    int digit_sprite(int digit) {
      assert(digit < 16);
      return digit_base + digit * 5;
    }

    int to_keycode(int key) {
      switch(key) {
      case 0: return SDLK_0;
      case 1: return SDLK_UP;
      case 2: return SDLK_2;
      case 3: return SDLK_3;
      case 4: return SDLK_DOWN;
      case 5: return SDLK_5;
      case 6: return SDLK_RIGHT;
      case 7: return SDLK_7;
      case 8: return SDLK_8;
      case 9: return SDLK_9;
      case 0xA: return SDLK_a;
      case 0xB: return SDLK_b;
      case 0xC: return SDLK_c;
      case 0xD: return SDLK_d;
      case 0xE: return SDLK_e;
      case 0xF: return SDLK_f;
      }
      return SDLK_F1;//this shouldn't happen
    }

    int from_keycode(int key) {
      switch(key) {
      case SDLK_0: return 0;
      case SDLK_UP: return 1;
      case SDLK_2: return 2;
      case SDLK_3: return 3;
      case SDLK_DOWN: return 4;
      case SDLK_5: return 5;
      case SDLK_RIGHT: return 6;
      case SDLK_7: return 7;
      case SDLK_8: return 8;
      case SDLK_9: return 9;
      case SDLK_a: return 0xA;
      case SDLK_b: return 0xB;
      case SDLK_c: return 0xC;
      case SDLK_d: return 0xD;
      case SDLK_e: return 0xE;
      case SDLK_f: return 0xF;
      }
      return -1;// ignore unmapped keys
    }

    bool get_key(int key) {
      return view->parent->get_key(to_keycode(key));
    }

    void set_last_key(int key) {
      last = from_keycode(key);
    }

    int wait_key() {
      last = -1;
      while(last == -1);
      return last;
    }
  };

}
