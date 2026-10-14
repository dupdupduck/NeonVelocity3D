#include <android/native_activity.h>
#include <android/native_window.h>
#include <android/input.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

#include "GameEngine.hpp"

#define LOG_TAG "NeonVelocity"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

struct NativeAppState {
    ANativeActivity* activity = nullptr;
    ANativeWindow* window = nullptr;
    AInputQueue* inputQueue = nullptr;

    std::thread renderThread;
    std::mutex mtx;
    std::condition_variable cv;

    std::atomic<bool> isRunning{false};
    std::atomic<bool> hasWindow{false};
    std::atomic<bool> windowDestroyedAck{false};

    Game::GameEngine engine;
};

static void ProcessInputEvents(NativeAppState* state) {
    if (!state->inputQueue) return;

    AInputEvent* event = nullptr;
    while (AInputQueue_getEvent(state->inputQueue, &event) >= 0) {
        if (AInputQueue_preDispatchEvent(state->inputQueue, event)) {
            continue;
        }

        int32_t handled = 0;
        int32_t eventType = AInputEvent_getType(event);

        if (eventType == AINPUT_EVENT_TYPE_MOTION) {
            int32_t action = AMotionEvent_getAction(event);
            int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;
            float x = AMotionEvent_getX(event, 0);
            float y = AMotionEvent_getY(event, 0);

            if (actionMasked == AMOTION_EVENT_ACTION_DOWN) {
                state->engine.HandleTouchDown(x, y);
                handled = 1;
            } else if (actionMasked == AMOTION_EVENT_ACTION_MOVE) {
                state->engine.HandleTouchMove(x, y);
                handled = 1;
            } else if (actionMasked == AMOTION_EVENT_ACTION_UP || actionMasked == AMOTION_EVENT_ACTION_CANCEL) {
                state->engine.HandleTouchUp();
                handled = 1;
            }
        }

        AInputQueue_finishEvent(state->inputQueue, event, handled);
    }
}

static void GameRenderLoop(NativeAppState* state) {
    LOGI("Neon Velocity 3D Render Thread Started");

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    EGLConfig config;
    EGLint numConfigs;

    bool glInitialized = false;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();

    while (state->isRunning) {
        // Wait until native window is available
        {
            std::unique_lock<std::mutex> lock(state->mtx);
            state->cv.wait(lock, [&]() {
                return !state->isRunning || state->hasWindow;
            });
        }

        if (!state->isRunning) break;

        // Initialize EGL Display & Context if needed
        if (display == EGL_NO_DISPLAY) {
            display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (display == EGL_NO_DISPLAY) {
                LOGE("eglGetDisplay failed");
                continue;
            }

            EGLint major = 0, minor = 0;
            if (!eglInitialize(display, &major, &minor)) {
                LOGE("eglInitialize failed");
                continue;
            }
            LOGI("EGL Initialized (v%d.%d)", major, minor);

            const EGLint attribs[] = {
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_DEPTH_SIZE, 16,
                EGL_NONE
            };

            if (!eglChooseConfig(display, attribs, &config, 1, &numConfigs) || numConfigs <= 0) {
                LOGE("eglChooseConfig failed");
                continue;
            }

            const EGLint contextAttribs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE
            };
            context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
            if (context == EGL_NO_CONTEXT) {
                LOGE("eglCreateContext failed");
                continue;
            }
        }

        // Create EGL Window Surface
        if (surface == EGL_NO_SURFACE && state->window != nullptr) {
            EGLint format;
            eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
            ANativeWindow_setBuffersGeometry(state->window, 0, 0, format);

            surface = eglCreateWindowSurface(display, config, state->window, nullptr);
            if (surface == EGL_NO_SURFACE) {
                LOGE("eglCreateWindowSurface failed");
                continue;
            }

            if (!eglMakeCurrent(display, surface, surface, context)) {
                LOGE("eglMakeCurrent failed");
                continue;
            }

            EGLint width = 0, height = 0;
            eglQuerySurface(display, surface, EGL_WIDTH, &width);
            eglQuerySurface(display, surface, EGL_HEIGHT, &height);

            if (!glInitialized) {
                state->engine.InitGL(width, height);
                glInitialized = true;
            } else {
                glViewport(0, 0, width, height);
                state->engine.screenWidth = width;
                state->engine.screenHeight = height;
            }
            LOGI("EGL Window Surface Ready: %dx%d", width, height);
            lastFrameTime = std::chrono::high_resolution_clock::now();
        }

        // Calculate delta time
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - lastFrameTime;
        lastFrameTime = now;
        float dt = elapsed.count();
        if (dt > 0.1f) dt = 0.1f; // Clamp to avoid physics explosion

        // Handle touch inputs
        ProcessInputEvents(state);

        // Update 3D Game Simulation
        state->engine.Update(dt);

        // Render 3D Scene
        state->engine.Render();

        // Swap buffers to display on screen
        eglSwapBuffers(display, surface);

        // Window destruction synchronization
        std::unique_lock<std::mutex> lock(state->mtx);
        if (!state->hasWindow && surface != EGL_NO_SURFACE) {
            LOGI("Cleaning up EGL surface on window destroy");
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroySurface(display, surface);
            surface = EGL_NO_SURFACE;

            state->windowDestroyedAck = true;
            state->cv.notify_all();
        }
    }

    // Terminate EGL cleanly
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        if (context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        eglTerminate(display);
    }
    LOGI("Neon Velocity 3D Render Thread Terminated");
}

static void onNativeWindowCreated(ANativeActivity* activity, ANativeWindow* window) {
    LOGI("Callback: onNativeWindowCreated");
    auto* state = static_cast<NativeAppState*>(activity->instance);
    {
        std::lock_guard<std::mutex> lock(state->mtx);
        state->window = window;
        state->hasWindow = true;
        state->windowDestroyedAck = false;
    }
    state->cv.notify_all();
}

static void onNativeWindowDestroyed(ANativeActivity* activity, ANativeWindow* window) {
    LOGI("Callback: onNativeWindowDestroyed");
    auto* state = static_cast<NativeAppState*>(activity->instance);
    {
        std::unique_lock<std::mutex> lock(state->mtx);
        state->hasWindow = false;
        state->cv.notify_all();
        // Wait until render thread has cleanly released EGL surface
        state->cv.wait(lock, [&]() { return state->windowDestroyedAck.load(); });
        state->window = nullptr;
    }
    LOGI("Native Window cleanly released");
}

static void onInputQueueCreated(ANativeActivity* activity, AInputQueue* queue) {
    LOGI("Callback: onInputQueueCreated");
    auto* state = static_cast<NativeAppState*>(activity->instance);
    state->inputQueue = queue;
}

static void onInputQueueDestroyed(ANativeActivity* activity, AInputQueue* queue) {
    LOGI("Callback: onInputQueueDestroyed");
    auto* state = static_cast<NativeAppState*>(activity->instance);
    state->inputQueue = nullptr;
}

static void onDestroy(ANativeActivity* activity) {
    LOGI("Callback: onDestroy");
    auto* state = static_cast<NativeAppState*>(activity->instance);
    if (state) {
        state->isRunning = false;
        state->hasWindow = false;
        state->cv.notify_all();
        if (state->renderThread.joinable()) {
            state->renderThread.join();
        }
        delete state;
        activity->instance = nullptr;
    }
}

// Android NDK NativeActivity Entrypoint
extern "C" void ANativeActivity_onCreate(ANativeActivity* activity, void* savedState, size_t savedStateSize) {
    LOGI("=== Neon Velocity 3D Starting ===");

    auto* state = new NativeAppState();
    state->activity = activity;
    state->isRunning = true;
    activity->instance = state;

    activity->callbacks->onDestroy = onDestroy;
    activity->callbacks->onNativeWindowCreated = onNativeWindowCreated;
    activity->callbacks->onNativeWindowDestroyed = onNativeWindowDestroyed;
    activity->callbacks->onInputQueueCreated = onInputQueueCreated;
    activity->callbacks->onInputQueueDestroyed = onInputQueueDestroyed;

    state->renderThread = std::thread(GameRenderLoop, state);
}
