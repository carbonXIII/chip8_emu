#include "sdl.h"

#ifdef DEBUG
#define D
#else
#define D if(false)
#endif

namespace chip8 {
  render_window::view::view(render_window* parent,
                            SDL_Rect rect,
                            uint32_t pixel_format)
    : parent(parent), dest(rect), _pitch(-1) {
    texture = SDL_CreateTexture(parent->renderer,
                                pixel_format,
                                SDL_TEXTUREACCESS_STREAMING,
                                rect.w, rect.h);
  }

  int render_window::view::pitch() {
    return _pitch;
  }

  void* render_window::view::lock() {
    SDL_LockTexture(texture, 0, &pixels, &_pitch);
    return pixels;
  }

  void render_window::view::unlock() {
    SDL_UnlockTexture(texture);
  }

  void render_window::view::move(int x, int y) {
    dest.x = x;
    dest.y = y;
  }

  void render_window::view::scale(int w, int h) {
    dest.w = w;
    dest.h = h;
  }

  void render_window::view::render() {
    SDL_RenderCopy(parent->renderer, texture, 0, &dest);
  }

  void render_window::init_sdl() {
    static bool init = false;

    if(!init) {
      init = true;

      // TODO: we don't really need everything
      SDL_Init(SDL_INIT_EVERYTHING);
    }
  }

  SDL_Rect render_window::null_rect() {
    SDL_Rect ret;
    ret.x = ret.y = ret.w = ret.h = 0;
    return ret;
  }

  SDL_Rect render_window::create_rect(int x, int y, int w, int h) {
    SDL_Rect ret;
    ret.x = x;
    ret.y = y;
    ret.w = w;
    ret.h = h;
    return ret;
  }

  render_window::render_window(int w,
                int h,
                const char* title,
                uint32_t flags)
    : w(w), h(h) {
    init_sdl();

    flags |= SDL_WINDOW_SHOWN;

    window = SDL_CreateWindow(title,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              w, h, flags);

    renderer = SDL_CreateRenderer(window, -1, 0);
  }

  void render_window::register_listener(std::function<void(int)> l) {
    listeners.push_back(l);
  }

  void render_window::key_update(int keycode, bool down) {
    key_status[keycode] = down;

    if(down) {
      for(auto f: listeners) {
        f(keycode);
      }
    }
  }

  bool render_window::get_key(int keycode) {
    return key_status[keycode];
  }

  render_window::view* render_window::add_view(SDL_Rect dest, uint32_t pixel_format) {

    if(dest.w == 0 && dest.h == 0) {
      dest.w = w;
      dest.h = h;
    }

    view* ret = new view(this, dest, pixel_format);
    views.push_back(std::shared_ptr<view>(ret));
    return ret;
  }

  bool render_window::update(bool redraw) {
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

  render_window::~render_window() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
  }

  template<typename addressable_t>
  const uint8_t sdl_runtime<addressable_t>::digits[0x50] = {
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

  template <typename addressable_t>
  sdl_runtime<addressable_t>::sdl_runtime(addressable_t* mem,
                                          render_window::view* view)
    : mem(mem), view(view), last(-1), pixels(W * H, 0) {
    mem->write(digit_base, digits, 0x50);
    view->parent->register_listener(std::bind(&sdl_runtime::set_last_key, this, std::placeholders::_1));
  }

  template <typename addressable_t>
  constexpr uint32_t sdl_runtime<addressable_t>::get_pixel(bool val) {
    return (val ? -1 : 0) | 0xFF;
  }

  template <typename addressable_t>
  void sdl_runtime<addressable_t>::clear() {
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

  template <typename addressable_t>
  bool sdl_runtime<addressable_t>::draw(int addr, int n, int x, int y) {
    uint32_t* p = (uint32_t*)view->lock();
    int stride = view->pitch() / sizeof(uint32_t);

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

    D {
      std::cout << "drawn: " << std::endl;
      for(int i = 0; i < n; i++) {
        std::cout << std::bitset<8>(mem->get(addr + i)) << std::endl;
      }
    }

    view->unlock();
    return ret;
  }

  template <typename addressable_t>
  int sdl_runtime<addressable_t>::digit_sprite(int digit) {
    assert(digit < 16);
    return digit_base + digit * 5;
  }

  // TODO: keybind config file?
  template <typename addressable_t>
  int sdl_runtime<addressable_t>::to_keycode(int key) {
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
    return SDLK_F1; // this shouldn't happen
  }

  template <typename addressable_t>
  int sdl_runtime<addressable_t>::from_keycode(int key) {
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
    return -1; // ignore unmapped keys
  }

  template <typename addressable_t>
  bool sdl_runtime<addressable_t>::get_key(int key) {
    return view->parent->get_key(to_keycode(key));
  }

  template <typename addressable_t>
  void sdl_runtime<addressable_t>::set_last_key(int key) {
    last = from_keycode(key);
  }

  template <typename addressable_t>
  int sdl_runtime<addressable_t>::wait_key() {
    last = -1;
    while(last == -1);
    return last;
  }

  // force instantiate the template functions
  void FORCE_DEFINE__sdl() {
    sdl_runtime<dram> a(0, 0);
    { auto _ = &sdl_runtime<dram>::clear; }
    { auto _ = &sdl_runtime<dram>::draw; }
    { auto _ = &sdl_runtime<dram>::digit_sprite; }
    { auto _ = &sdl_runtime<dram>::get_key; }
    { auto _ = &sdl_runtime<dram>::wait_key; }
  }
}
