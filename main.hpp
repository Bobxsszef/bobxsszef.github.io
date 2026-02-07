#pragma once
#ifndef MAIN_HPP_INCLUDED
#define MAIN_HPP_INCLUDED

// Minimalne includy
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <cstdint>
#include <iosfwd>

// Forward declarations
struct GLFWwindow;
struct ImFont;
typedef unsigned int GLuint;

// Windows (minimalny overhead)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

//
// Hermetyzowane stany aplikacji
//

/// Stan okna overlay (ImGui + GLFW)
struct OverlayState {
    HWND hwnd = nullptr;
    GLFWwindow* window = nullptr;
    int width = 1280;
    int height = 720;
    std::atomic<bool> visible{ false };

    /// Tekstura logo wyœwietlanego w UI
    struct LogoTexture {
        int width = 0;       ///< Szerokoœæ w pikselach
        int height = 0;      ///< Wysokoœæ w pikselach
        GLuint id = 0;       ///< OpenGL texture handle (0 = nie za³adowano)
    } logo;
};

/// Stan pod³¹czonej gry
struct GameState {
    std::atomic<uintptr_t> localplayer{ 0 };  ///< WskaŸnik do LocalPlayer w pamiêci gry
    HWND attach_window = nullptr;           ///< HWND okna gry
    RECT window_rect{};                     ///< Pozycja i rozmiar okna gry
};

extern OverlayState g_overlay;
extern GameState g_game;

//
// Fonts container
//

namespace fonts {
    struct Collection {
        ImFont* medium = nullptr;
        ImFont* bold = nullptr;
        ImFont* tab_icons = nullptr;
        ImFont* logo = nullptr;
        ImFont* tab_title = nullptr;
        ImFont* tab_title_icon = nullptr;
        ImFont* subtab_title = nullptr;
        ImFont* combo_arrow = nullptr;
    };

    extern Collection g_fonts;
}

//
// Logger (thread-safe, OutputDebugString + plik)
//

namespace util {
    enum class LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };

    /// Inicjalizuje logger (plik + OutputDebugString)
    void InitLogger(const std::string& path = "bobx.log");

    /// Zamyka logger i flush'uje dane
    void CloseLogger();

    /// Printf-style logging (thread-safe)
    /// @note Ta funkcja nie powinna rzucaæ wyj¹tków (noexcept)
    void Log(LogLevel level, const char* fmt, ...) noexcept;
}

//
// Exception handling macro
// Preferuj lokalne try-catch tam, gdzie potrzebujesz cleanup/retry logic
//

#define SAFE_EXECUTE(block) \
    do { \
        try { \
            block; \
        } \
        catch (const std::bad_alloc&) { \
            util::Log(util::LogLevel::FATAL, "Out of memory (%s:%d)", __FILE__, __LINE__); \
            throw; \
        } \
        catch (const std::exception& e) { \
            util::Log(util::LogLevel::ERROR, "Exception: %s (%s:%d)", e.what(), __FILE__, __LINE__); \
        } \
        catch (...) { \
            util::Log(util::LogLevel::FATAL, "Unknown exception (%s:%d)", __FILE__, __LINE__); \
            throw; \
        } \
    } while (0)

//
// Modu³y aplikacji (deklaracje)
//

namespace overlay {
    /// £aduje teksturê z pliku do OpenGL
    /// @return true jeœli sukces, false jeœli b³¹d
    [[nodiscard]] bool LoadTexture(const char* path, GLuint* tex, int* w, int* h);

    /// Ustawia motyw kolorystyczny ImGui
    void ApplyTheme();

    /// Inicjalizuje overlay (GLFW + ImGui)
    /// @return true jeœli sukces, false jeœli b³¹d
    [[nodiscard]] bool Setup();

    /// Zwalnia zasoby overlay
    void Cleanup();
}

namespace input {
    /// Obs³uguje wciœniêcia klawiszy (INSERT, HOME, etc.)
    void HandleKeyPresses();
}

namespace game {
    /// G³ówna pêtla monitoruj¹ca LocalPlayer
    /// @note Uruchamiana w osobnym w¹tku
    void ReallockLocalPlayer();
}

//
// Silent Aim configuration (extern declarations)
//
namespace silentaim_config {
    extern bool on;
    extern bool fov;
    extern int bone;
    extern float fov_value;
    extern float max_distance;
    extern float angle_threshold;
    extern bool enable_facing_check;
    extern float facing_threshold;
    extern bool rage_mode;
    extern int bind;
}

#endif // MAIN_HPP_INCLUDED