#include <SDL.h>
#include <SDL_ttf.h>
#include <opencv2/opencv.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr int kControlWidth = 520;
constexpr int kControlHeight = 720;
constexpr int kVideoWidth = 960;
constexpr int kVideoHeight = 720;
constexpr int kMaxCamerasToProbe = 10;
constexpr int kButtonHeight = 38;
constexpr int kButtonGap = 10;
constexpr int kPanelPadding = 14;

struct CameraInfo {
    int index = -1;
    std::string label;
};

enum class Action {
    PrevCamera,
    NextCamera,
    RescanCameras,
    TogglePause,
    ToggleGray,
    ToggleMirror,
    ToggleFlipH,
    ToggleFlipV,
    ToggleBlur,
    ToggleEdge,
    SelectCamera,
};

struct Button {
    SDL_Rect rect{};
    std::string label;
    Action action = Action::TogglePause;
    int value = -1;
};

struct AppState {
    std::vector<CameraInfo> cameras;
    int selected_camera = 0;
    bool paused = false;
    bool grayscale = false;
    bool mirror = false;
    bool flip_h = false;
    bool flip_v = false;
    bool blur = false;
    bool edge = false;
    bool rescan_requested = false;
    std::string status = "Ready";
    std::string camera_status = "No camera open";
};

struct SdlWindow {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
    int width = 0;
    int height = 0;
};

bool file_exists(const std::string& path) {
    return std::filesystem::exists(path);
}

std::string find_system_font() {
    const std::vector<std::string> candidates = {
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/System/Library/Fonts/Supplemental/Arial Unicode.ttf",
        "/System/Library/Fonts/Supplemental/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    };

    for (const auto& path : candidates) {
        if (file_exists(path)) {
            return path;
        }
    }
    return {};
}

void set_message(AppState& state, const std::string& message) {
    state.status = message;
}

std::vector<CameraInfo> probe_cameras(int max_index) {
    std::vector<CameraInfo> cameras;
    for (int index = 0; index < max_index; ++index) {
        cv::VideoCapture probe(index);
        if (!probe.isOpened()) {
            continue;
        }
        cameras.push_back({index, "Camera " + std::to_string(index)});
        probe.release();
    }
    return cameras;
}

bool open_camera(int camera_index, cv::VideoCapture& capture, std::string& error) {
    capture.release();
    if (!capture.open(camera_index)) {
        error = "Failed to open camera " + std::to_string(camera_index);
        return false;
    }

    capture.set(cv::CAP_PROP_BUFFERSIZE, 1.0);
    error.clear();
    return true;
}

bool ensure_camera(AppState& state, cv::VideoCapture& capture) {
    if (state.cameras.empty()) {
        capture.release();
        state.camera_status = "No cameras detected";
        return false;
    }

    state.selected_camera = std::clamp(state.selected_camera, 0, static_cast<int>(state.cameras.size()) - 1);
    std::string error;
    if (!open_camera(state.cameras[state.selected_camera].index, capture, error)) {
        state.camera_status = error;
        return false;
    }

    state.camera_status = "Opened " + state.cameras[state.selected_camera].label;
    return true;
}

cv::Mat apply_effects(const cv::Mat& input, const AppState& state) {
    cv::Mat frame = input.clone();

    if (state.blur) {
        cv::GaussianBlur(frame, frame, cv::Size(15, 15), 0.0);
    }

    if (state.flip_h && state.flip_v) {
        cv::flip(frame, frame, -1);
    } else if (state.flip_h) {
        cv::flip(frame, frame, 1);
    } else if (state.flip_v) {
        cv::flip(frame, frame, 0);
    }

    if (state.edge) {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::Canny(gray, gray, 80, 160);
        cv::cvtColor(gray, frame, cv::COLOR_GRAY2BGR);
    } else if (state.grayscale) {
        cv::cvtColor(frame, frame, cv::COLOR_BGR2GRAY);
        cv::cvtColor(frame, frame, cv::COLOR_GRAY2BGR);
    }

    return frame;
}

void destroy_window(SdlWindow& window) {
    if (window.texture) {
        SDL_DestroyTexture(window.texture);
        window.texture = nullptr;
    }
    if (window.renderer) {
        SDL_DestroyRenderer(window.renderer);
        window.renderer = nullptr;
    }
    if (window.window) {
        SDL_DestroyWindow(window.window);
        window.window = nullptr;
    }
}

bool create_window(SdlWindow& window, const char* title, int width, int height, Uint32 flags) {
    window.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
    if (!window.window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
        return false;
    }

    window.renderer = SDL_CreateRenderer(window.window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!window.renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
        return false;
    }

    window.width = width;
    window.height = height;
    return true;
}

struct UiResources {
    TTF_Font* font = nullptr;
    TTF_Font* small_font = nullptr;
};

SDL_Texture* render_text(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color, SDL_Rect* out_size = nullptr) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture && out_size) {
        out_size->w = surface->w;
        out_size->h = surface->h;
    }

    SDL_FreeSurface(surface);
    return texture;
}

void draw_text(SDL_Renderer* renderer, TTF_Font* font, int x, int y, const std::string& text, SDL_Color color) {
    SDL_Rect bounds{};
    SDL_Texture* texture = render_text(renderer, font, text, color, &bounds);
    if (!texture) {
        return;
    }

    bounds.x = x;
    bounds.y = y;
    SDL_RenderCopy(renderer, texture, nullptr, &bounds);
    SDL_DestroyTexture(texture);
}

void draw_button(SDL_Renderer* renderer, TTF_Font* font, const Button& button, bool active, bool pressed) {
    SDL_Color fill = active ? SDL_Color{40, 140, 240, 255} : SDL_Color{70, 74, 82, 255};
    if (pressed) {
        fill = SDL_Color{240, 170, 70, 255};
    }

    SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(renderer, &button.rect);

    SDL_SetRenderDrawColor(renderer, 18, 18, 18, 255);
    SDL_RenderDrawRect(renderer, &button.rect);

    SDL_Rect text_bounds{};
    SDL_Texture* text = render_text(renderer, font, button.label, SDL_Color{255, 255, 255, 255}, &text_bounds);
    if (!text) {
        return;
    }

    const int text_x = button.rect.x + (button.rect.w - text_bounds.w) / 2;
    const int text_y = button.rect.y + (button.rect.h - text_bounds.h) / 2;
    text_bounds.x = text_x;
    text_bounds.y = text_y;
    SDL_RenderCopy(renderer, text, nullptr, &text_bounds);
    SDL_DestroyTexture(text);
}

std::vector<Button> build_buttons(const AppState& state, int width) {
    std::vector<Button> buttons;
    const int usable_width = width - 2 * kPanelPadding;
    const int two_col_width = (usable_width - kButtonGap) / 2;
    const int three_col_width = (usable_width - 2 * kButtonGap) / 3;

    auto push_button = [&buttons](int x, int y, int w, const std::string& label, Action action, int value = -1) {
        buttons.push_back(Button{SDL_Rect{x, y, w, kButtonHeight}, label, action, value});
    };

    int y = 120;
    push_button(kPanelPadding, y, two_col_width, "PREV CAMERA", Action::PrevCamera);
    push_button(kPanelPadding + two_col_width + kButtonGap, y, two_col_width, "NEXT CAMERA", Action::NextCamera);

    y += kButtonHeight + kButtonGap;
    push_button(kPanelPadding, y, two_col_width, "RESCAN", Action::RescanCameras);
    push_button(kPanelPadding + two_col_width + kButtonGap, y, two_col_width, state.paused ? "RESUME" : "PAUSE", Action::TogglePause);

    y += kButtonHeight + kButtonGap;
    push_button(kPanelPadding, y, three_col_width, state.grayscale ? "GRAY ON" : "GRAY OFF", Action::ToggleGray);
    push_button(kPanelPadding + three_col_width + kButtonGap, y, three_col_width, state.mirror ? "MIRROR ON" : "MIRROR OFF", Action::ToggleMirror);
    push_button(kPanelPadding + 2 * (three_col_width + kButtonGap), y, three_col_width, state.edge ? "EDGE ON" : "EDGE OFF", Action::ToggleEdge);

    y += kButtonHeight + kButtonGap;
    push_button(kPanelPadding, y, two_col_width, state.flip_h ? "FLIP H ON" : "FLIP H OFF", Action::ToggleFlipH);
    push_button(kPanelPadding + two_col_width + kButtonGap, y, two_col_width, state.flip_v ? "FLIP V ON" : "FLIP V OFF", Action::ToggleFlipV);

    y += kButtonHeight + kButtonGap;
    push_button(kPanelPadding, y, two_col_width, state.blur ? "BLUR ON" : "BLUR OFF", Action::ToggleBlur);

    y += kButtonHeight + 22;
    const int camera_columns = 2;
    const int camera_button_width = (usable_width - (camera_columns - 1) * kButtonGap) / camera_columns;
    for (std::size_t i = 0; i < state.cameras.size(); ++i) {
        const int row = static_cast<int>(i) / camera_columns;
        const int col = static_cast<int>(i) % camera_columns;
        const int x = kPanelPadding + col * (camera_button_width + kButtonGap);
        const int cy = y + row * (kButtonHeight + kButtonGap);
        push_button(x, cy, camera_button_width, state.cameras[i].label, Action::SelectCamera, state.cameras[i].index);
    }

    return buttons;
}

void draw_control_window(SDL_Renderer* renderer, TTF_Font* font, TTF_Font* small_font, const AppState& state, int width, int height) {
    SDL_SetRenderDrawColor(renderer, 28, 30, 35, 255);
    SDL_RenderClear(renderer);

    draw_text(renderer, font, kPanelPadding, 12, "CAMERA CONTROL", SDL_Color{240, 240, 240, 255});
    draw_text(renderer, small_font, kPanelPadding, 46, state.camera_status, SDL_Color{180, 190, 205, 255});
    draw_text(renderer, small_font, kPanelPadding, 70, state.status, SDL_Color{180, 190, 205, 255});

    const auto buttons = build_buttons(state, width);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

    for (const auto& button : buttons) {
        bool active = false;
        switch (button.action) {
            case Action::TogglePause: active = state.paused; break;
            case Action::ToggleGray: active = state.grayscale; break;
            case Action::ToggleMirror: active = state.mirror; break;
            case Action::ToggleFlipH: active = state.flip_h; break;
            case Action::ToggleFlipV: active = state.flip_v; break;
            case Action::ToggleBlur: active = state.blur; break;
            case Action::ToggleEdge: active = state.edge; break;
            case Action::SelectCamera:
                active = !state.cameras.empty() && button.value == state.cameras[state.selected_camera].index;
                break;
            default: active = false; break;
        }
        draw_button(renderer, font, button, active, false);
    }

    const int footer_y = height - 52;
    draw_text(renderer, small_font, kPanelPadding, footer_y, "Click a camera button to switch. Esc quits.", SDL_Color{160, 170, 180, 255});
    SDL_RenderPresent(renderer);
}

void draw_video_window(SDL_Renderer* renderer, SDL_Texture* texture, int width, int height, const std::string& status, TTF_Font* font) {
    SDL_SetRenderDrawColor(renderer, 14, 14, 16, 255);
    SDL_RenderClear(renderer);

    if (texture) {
        SDL_Rect target{0, 0, width, height};
        SDL_RenderCopy(renderer, texture, nullptr, &target);
    } else {
        SDL_Rect box{40, 40, width - 80, height - 80};
        SDL_SetRenderDrawColor(renderer, 60, 60, 72, 255);
        SDL_RenderDrawRect(renderer, &box);
        draw_text(renderer, font, 56, 60, status, SDL_Color{220, 220, 220, 255});
    }

    SDL_RenderPresent(renderer);
}

Button* hit_test(std::vector<Button>& buttons, int x, int y) {
    for (auto& button : buttons) {
        if (x >= button.rect.x && x < button.rect.x + button.rect.w && y >= button.rect.y && y < button.rect.y + button.rect.h) {
            return &button;
        }
    }
    return nullptr;
}

}  // namespace

int main() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
        return 1;
    }

    if (TTF_Init() != 0) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << '\n';
        SDL_Quit();
        return 1;
    }

    const std::string font_path = find_system_font();
    if (font_path.empty()) {
        std::cerr << "Could not find a system font. Please install DejaVu Sans, Helvetica, or Arial.\n";
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    UiResources ui;
    ui.font = TTF_OpenFont(font_path.c_str(), 20);
    ui.small_font = TTF_OpenFont(font_path.c_str(), 16);
    if (!ui.font || !ui.small_font) {
        std::cerr << "TTF_OpenFont failed: " << TTF_GetError() << '\n';
        if (ui.font) TTF_CloseFont(ui.font);
        if (ui.small_font) TTF_CloseFont(ui.small_font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    SdlWindow video_window;
    SdlWindow control_window;
    if (!create_window(video_window, "Camera Viewer", kVideoWidth, kVideoHeight, SDL_WINDOW_RESIZABLE) ||
        !create_window(control_window, "Camera Control", kControlWidth, kControlHeight, SDL_WINDOW_RESIZABLE)) {
        destroy_window(video_window);
        destroy_window(control_window);
        TTF_CloseFont(ui.font);
        TTF_CloseFont(ui.small_font);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    cv::VideoCapture capture;
    AppState state;
    state.cameras = probe_cameras(kMaxCamerasToProbe);
    if (state.cameras.empty()) {
        set_message(state, "No cameras detected. Use Rescan after connecting one.");
    } else {
        ensure_camera(state, capture);
        set_message(state, "Ready");
    }

    SDL_Texture* video_texture = nullptr;
    bool running = true;
    bool dragging = false;
    int active_window_id = 0;
    const auto start_time = std::chrono::steady_clock::now();
    cv::Mat latest_frame;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
                break;
            }

            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    running = false;
                    break;
                }
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    if (event.window.windowID == SDL_GetWindowID(video_window.window)) {
                        video_window.width = event.window.data1;
                        video_window.height = event.window.data2;
                    } else if (event.window.windowID == SDL_GetWindowID(control_window.window)) {
                        control_window.width = event.window.data1;
                        control_window.height = event.window.data2;
                    }
                }
            }

            if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT && event.button.windowID == SDL_GetWindowID(control_window.window)) {
                dragging = true;
                active_window_id = static_cast<int>(event.button.windowID);
                auto buttons = build_buttons(state, control_window.width);
                if (Button* button = hit_test(buttons, event.button.x, event.button.y)) {
                    switch (button->action) {
                        case Action::PrevCamera:
                            if (!state.cameras.empty()) {
                                state.selected_camera = (state.selected_camera - 1 + static_cast<int>(state.cameras.size())) % static_cast<int>(state.cameras.size());
                                if (!ensure_camera(state, capture)) {
                                    set_message(state, "Failed to switch to previous camera");
                                } else {
                                    set_message(state, "Switched camera");
                                }
                            }
                            break;
                        case Action::NextCamera:
                            if (!state.cameras.empty()) {
                                state.selected_camera = (state.selected_camera + 1) % static_cast<int>(state.cameras.size());
                                if (!ensure_camera(state, capture)) {
                                    set_message(state, "Failed to switch to next camera");
                                } else {
                                    set_message(state, "Switched camera");
                                }
                            }
                            break;
                        case Action::RescanCameras:
                            state.rescan_requested = true;
                            set_message(state, "Rescanning cameras...");
                            break;
                        case Action::TogglePause: state.paused = !state.paused; set_message(state, state.paused ? "Paused" : "Running"); break;
                        case Action::ToggleGray: state.grayscale = !state.grayscale; set_message(state, state.grayscale ? "Grayscale enabled" : "Grayscale disabled"); break;
                        case Action::ToggleMirror: state.mirror = !state.mirror; set_message(state, state.mirror ? "Mirror enabled" : "Mirror disabled"); break;
                        case Action::ToggleFlipH: state.flip_h = !state.flip_h; set_message(state, state.flip_h ? "Horizontal flip enabled" : "Horizontal flip disabled"); break;
                        case Action::ToggleFlipV: state.flip_v = !state.flip_v; set_message(state, state.flip_v ? "Vertical flip enabled" : "Vertical flip disabled"); break;
                        case Action::ToggleBlur: state.blur = !state.blur; set_message(state, state.blur ? "Blur enabled" : "Blur disabled"); break;
                        case Action::ToggleEdge: state.edge = !state.edge; set_message(state, state.edge ? "Edge detection enabled" : "Edge detection disabled"); break;
                        case Action::SelectCamera:
                            for (std::size_t i = 0; i < state.cameras.size(); ++i) {
                                if (state.cameras[i].index == button->value) {
                                    state.selected_camera = static_cast<int>(i);
                                    if (!ensure_camera(state, capture)) {
                                        set_message(state, "Failed to open selected camera");
                                    } else {
                                        set_message(state, "Camera changed");
                                    }
                                    break;
                                }
                            }
                            break;
                    }
                }
            }

            if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
                dragging = false;
                active_window_id = 0;
            }
        }

        if (state.rescan_requested) {
            state.rescan_requested = false;
            state.cameras = probe_cameras(kMaxCamerasToProbe);
            if (state.cameras.empty()) {
                capture.release();
                state.camera_status = "No cameras detected";
                set_message(state, "No cameras found");
            } else {
                state.selected_camera = std::clamp(state.selected_camera, 0, static_cast<int>(state.cameras.size()) - 1);
                ensure_camera(state, capture);
                set_message(state, "Rescan complete");
            }
        }

        if (!state.paused && capture.isOpened()) {
            cv::Mat frame;
            capture >> frame;
            if (!frame.empty()) {
                latest_frame = frame;
            }
        }

        if (!latest_frame.empty()) {
            cv::Mat processed = apply_effects(latest_frame, state);
            if (!video_texture) {
                video_texture = SDL_CreateTexture(video_window.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, processed.cols, processed.rows);
            }

            int texture_width = 0;
            int texture_height = 0;
            if (video_texture) {
                SDL_QueryTexture(video_texture, nullptr, nullptr, &texture_width, &texture_height);
                if (texture_width != processed.cols || texture_height != processed.rows) {
                    SDL_DestroyTexture(video_texture);
                    video_texture = SDL_CreateTexture(video_window.renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, processed.cols, processed.rows);
                }
            }

            if (video_texture) {
                cv::Mat rgba;
                cv::cvtColor(processed, rgba, cv::COLOR_BGR2RGBA);
                if (SDL_UpdateTexture(video_texture, nullptr, rgba.data, static_cast<int>(rgba.step)) != 0) {
                    std::cerr << "SDL_UpdateTexture failed: " << SDL_GetError() << '\n';
                }
            }
        }

        draw_video_window(video_window.renderer, video_texture, video_window.width, video_window.height, state.camera_status, ui.font);
        draw_control_window(control_window.renderer, ui.font, ui.small_font, state, control_window.width, control_window.height);

        if (dragging && active_window_id == SDL_GetWindowID(control_window.window)) {
            SDL_Delay(1);
        }

        const auto elapsed = std::chrono::steady_clock::now() - start_time;
        (void)elapsed;
    }

    if (video_texture) {
        SDL_DestroyTexture(video_texture);
    }
    destroy_window(video_window);
    destroy_window(control_window);

    if (ui.font) {
        TTF_CloseFont(ui.font);
    }
    if (ui.small_font) {
        TTF_CloseFont(ui.small_font);
    }

    capture.release();
    TTF_Quit();
    SDL_Quit();
    return 0;
}
