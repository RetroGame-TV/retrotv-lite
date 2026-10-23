#include <SDL.h>
int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) return 1;
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    auto* window = SDL_CreateWindow("RetroTV Lite", 0, 0, 1280, 720, SDL_WINDOW_FULLSCREEN_DESKTOP);
    auto* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { SDL_Quit(); return 2; }
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) running = false;
        SDL_SetRenderDrawColor(renderer, 16, 25, 33, 255); SDL_RenderClear(renderer);
        SDL_Rect focus{64,64,480,160}; SDL_SetRenderDrawColor(renderer,115,224,180,255); SDL_RenderDrawRect(renderer,&focus);
        SDL_RenderPresent(renderer); SDL_Delay(10);
    }
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit(); return 0;
}
