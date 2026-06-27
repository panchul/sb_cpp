#include <SDL.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kInitialWidth = 640;
constexpr int kInitialHeight = 360;

struct WindowContext {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    std::vector<std::uint32_t> pixels;
    int width = kInitialWidth;
    int height = kInitialHeight;
    std::uint32_t window_id = 0;
    float phase_offset = 0.0f;
};

std::uint8_t to_u8(float v) {
    const float clamped = std::clamp(v, 0.0f, 255.0f);
    return static_cast<std::uint8_t>(clamped);
}

std::uint32_t pack_argb(std::uint8_t a, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    return (static_cast<std::uint32_t>(a) << 24U) |
           (static_cast<std::uint32_t>(r) << 16U) |
           (static_cast<std::uint32_t>(g) << 8U) |
           static_cast<std::uint32_t>(b);
}

bool create_texture(WindowContext& ctx, int width, int height) {
    if (ctx.texture) {
        SDL_DestroyTexture(ctx.texture);
        ctx.texture = nullptr;
    }

    ctx.width = std::max(1, width);
    ctx.height = std::max(1, height);
    ctx.pixels.assign(static_cast<std::size_t>(ctx.width) * static_cast<std::size_t>(ctx.height), 0U);

    ctx.texture = SDL_CreateTexture(
        ctx.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        ctx.width,
        ctx.height);

    if (!ctx.texture) {
        std::cerr << "SDL_CreateTexture failed: " << SDL_GetError() << '\n';
        return false;
    }

    return true;
}

bool create_window(WindowContext& ctx, const std::string& title, int x, int y, float phase_offset) {
    ctx.phase_offset = phase_offset;

    ctx.window = SDL_CreateWindow(
        title.c_str(),
        x,
        y,
        kInitialWidth,
        kInitialHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (!ctx.window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        return false;
    }

    ctx.renderer = SDL_CreateRenderer(ctx.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ctx.renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        return false;
    }

    ctx.window_id = SDL_GetWindowID(ctx.window);

    if (!create_texture(ctx, kInitialWidth, kInitialHeight)) {
        return false;
    }

    return true;
}

void destroy_window(WindowContext& ctx) {
    if (ctx.texture) {
        SDL_DestroyTexture(ctx.texture);
        ctx.texture = nullptr;
    }
    if (ctx.renderer) {
        SDL_DestroyRenderer(ctx.renderer);
        ctx.renderer = nullptr;
    }
    if (ctx.window) {
        SDL_DestroyWindow(ctx.window);
        ctx.window = nullptr;
    }
}

void draw_bitmap_frame(WindowContext& ctx, float t_seconds) {
    const int w = ctx.width;
    const int h = ctx.height;

    const float cx = static_cast<float>(w) * 0.5f;
    const float cy = static_cast<float>(h) * 0.5f;
    const float radius = std::min(w, h) * 0.16f;

    const float bx = cx + std::sin(t_seconds * 1.3f + ctx.phase_offset) * (cx - radius - 8.0f);
    const float by = cy + std::cos(t_seconds * 1.8f + ctx.phase_offset * 0.8f) * (cy - radius - 8.0f);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const float xf = static_cast<float>(x);
            const float yf = static_cast<float>(y);

            const float wave1 = std::sin((xf * 0.045f) + t_seconds * 2.2f + ctx.phase_offset);
            const float wave2 = std::cos((yf * 0.055f) - t_seconds * 1.6f + ctx.phase_offset * 0.7f);
            const float wave3 = std::sin((xf + yf) * 0.02f + t_seconds * 1.1f + ctx.phase_offset * 1.3f);

            const float r = 120.0f + 85.0f * wave1;
            const float g = 120.0f + 85.0f * wave2;
            const float b = 120.0f + 85.0f * wave3;

            const float dx = xf - bx;
            const float dy = yf - by;
            const float dist2 = dx * dx + dy * dy;

            std::uint32_t color = pack_argb(255, to_u8(r), to_u8(g), to_u8(b));

            if (dist2 < radius * radius) {
                color = pack_argb(255, 255, 255, 255);
            }

            ctx.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] = color;
        }
    }
}

bool upload_bitmap(WindowContext& ctx) {
    void* pixels = nullptr;
    int pitch = 0;
    if (SDL_LockTexture(ctx.texture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "SDL_LockTexture failed: " << SDL_GetError() << '\n';
        return false;
    }

    const std::size_t row_bytes = static_cast<std::size_t>(ctx.width) * sizeof(std::uint32_t);
    for (int y = 0; y < ctx.height; ++y) {
        const auto* src = reinterpret_cast<const std::uint8_t*>(ctx.pixels.data()) + static_cast<std::size_t>(y) * row_bytes;
        auto* dst = reinterpret_cast<std::uint8_t*>(pixels) + static_cast<std::size_t>(y) * static_cast<std::size_t>(pitch);
        std::memcpy(dst, src, row_bytes);
    }

    SDL_UnlockTexture(ctx.texture);
    return true;
}

void render_window(WindowContext& ctx) {
    SDL_RenderClear(ctx.renderer);
    SDL_RenderCopy(ctx.renderer, ctx.texture, nullptr, nullptr);
    SDL_RenderPresent(ctx.renderer);
}

}  // namespace

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    std::vector<WindowContext> windows(2);

    if (!create_window(windows[0], "Window A - Bitmap Animation", SDL_WINDOWPOS_CENTERED - 360, SDL_WINDOWPOS_CENTERED, 0.0f) ||
        !create_window(windows[1], "Window B - Bitmap Animation", SDL_WINDOWPOS_CENTERED + 360, SDL_WINDOWPOS_CENTERED, 2.2f)) {
        for (auto& w : windows) {
            destroy_window(w);
        }
        SDL_Quit();
        return 1;
    }

    const auto start = std::chrono::steady_clock::now();
    bool running = true;

    while (running && !windows.empty()) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false;
                break;
            }
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
                break;
            }
            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    const std::uint32_t closing_id = e.window.windowID;
                    windows.erase(
                        std::remove_if(
                            windows.begin(),
                            windows.end(),
                            [&](WindowContext& ctx) {
                                if (ctx.window_id == closing_id) {
                                    destroy_window(ctx);
                                    return true;
                                }
                                return false;
                            }),
                        windows.end());
                } else if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    const std::uint32_t resized_id = e.window.windowID;
                    for (auto& ctx : windows) {
                        if (ctx.window_id == resized_id) {
                            if (!create_texture(ctx, e.window.data1, e.window.data2)) {
                                running = false;
                            }
                            break;
                        }
                    }
                }
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<float> elapsed = now - start;

        for (auto& ctx : windows) {
            draw_bitmap_frame(ctx, elapsed.count());
            if (!upload_bitmap(ctx)) {
                running = false;
                break;
            }
            render_window(ctx);
        }
    }

    for (auto& w : windows) {
        destroy_window(w);
    }

    SDL_Quit();
    return 0;
}
