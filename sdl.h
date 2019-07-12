#include "core.h"

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

    static void init_sdl();

    static SDL_Rect null_rect();
    static SDL_Rect create_rect(int x, int y, int w, int h);

    struct view {
      SDL_Texture* texture;
      render_window* parent;
      SDL_Rect dest;

      int _pitch;

      void* pixels;

      view(render_window* parent, SDL_Rect rect, uint32_t pixel_format);

      int pitch();
      void* lock();
      void unlock();

      void move(int x, int y);
      void scale(int w, int h);

      void render();
    };

    std::vector<std::shared_ptr<view> > views;

    std::unordered_map<int, bool> key_status;
    std::vector<std::function<void(int)>> listeners;

    render_window(int w, int h, const char* title = "CHIP8", uint32_t flags = 0);

    void register_listener(std::function<void(int)> l);
    void key_update(int keycode, bool down);
    bool get_key(int keycode);

    view* add_view(SDL_Rect dest = null_rect(), uint32_t pixel_format = SDL_PIXELFORMAT_BGRA8888);

    bool update(bool redraw = true);

    ~render_window();
  };

  template <typename addressable_t>
  struct sdl_runtime: debug_runtime {
    addressable_t* mem;
    render_window::view* view;

    int W = 64;
    int H = 32;

    std::vector<int> pixels;

    static const uint8_t digits[0x50];

    const int digit_base = 0;
    std::atomic<int> last;

    sdl_runtime(addressable_t* mem, render_window::view* view);

    constexpr uint32_t get_pixel(bool val);

    void clear();
    bool draw(int addr, int n, int x, int y);

    int digit_sprite(int digit);

    int to_keycode(int key);
    int from_keycode(int key);
    bool get_key(int key);
    void set_last_key(int key);
    int wait_key();
  };

}
