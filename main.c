#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL2/SDL.h>

#define PI 3.14159265f
#define MAP_WIDTH 8
#define MAP_HEIGHT 8

typedef struct {
    float x;
    float y;
} floatVec;

typedef struct {
    int x;
    int y;
} intVec;

typedef struct {
    float x;
    float y;
    float angle;
} ray;

const int WIN_WIDTH = 800, WIN_HEIGHT = 800;
const int LOG_WIDTH = 512, LOG_HEIGHT = 512;
const int BLOCK_WIDTH = 64, BLOCK_HEIGHT = 64;
const int GAME_WIDTH = 64, GAME_HEIGHT = 64;

int map[MAP_WIDTH * MAP_HEIGHT] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 0, 0, 0, 0, 1, 
    1, 0, 0, 0, 0, 1, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 0, 0, 0, 1,
    1, 0, 0, 0, 0, 0, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1
};

struct {
    /* data */
    floatVec charPos;
    floatVec charVel;
    floatVec charDir;
    floatVec rayDest;
    float angle;
    int colIndex;

    uint32_t *pixels;
} state;


floatVec normalizeVec(floatVec v) {
    float len = sqrtf((v.x * v.x) + (v.y * v.y));
    if (len != 0) {
        v.x /= len;
        v.y /= len;
    }
    return v;
}

int drawPointToScale(int x, int y, int scale, uint32_t color) {
    if (x > LOG_WIDTH - scale || x < 0 || y > LOG_HEIGHT - scale || y < 0) {
        return 1;
    }

    for (int i = 0; i < scale; i++) {
        for (int j = 0; j < scale; j++) {
            state.pixels[(y + i) * LOG_WIDTH + x + j] = color;
        }
    }
    return 0;
}

void drawColumn(int column, int idx, uint32_t color) {

}

void drawBlock(int row, int column, uint32_t color) {
    drawPointToScale(row * BLOCK_WIDTH, column * BLOCK_WIDTH, BLOCK_WIDTH, color);
}

int clampI(int i, int min, int max) {
    return i < min ? min : i > max ? max : i;
}

float clampF(float i, float min, float max) {
    return i < min ? min : i > max ? max : i;
}

void drawMap() {
    for (int i = 0; i < MAP_WIDTH; i++) {
        for (int j = 0; j < MAP_HEIGHT; j++) {
            if (map[j * MAP_WIDTH + i] != 0) {
                drawPointToScale((i * BLOCK_WIDTH) + 1, (j * BLOCK_WIDTH) + 1, BLOCK_WIDTH - 1, 0x00FF00AA);
            } else {
                drawPointToScale((i * BLOCK_WIDTH) + 1, (j * BLOCK_WIDTH) + 1, BLOCK_WIDTH - 1, 0xFFFFFFFF);
            }
        }
    }
}

intVec whichBlock(float x, float y) {
    intVec block;
    block.x = x / BLOCK_WIDTH;
    block.y = y / BLOCK_HEIGHT;

    block.x = block.x <= 0 ? 0 : block.x;
    block.x = block.x >= BLOCK_WIDTH ? BLOCK_WIDTH - 1 : block.x;

    block.y = block.y <= 0 ? 0 : block.y;
    block.y = block.y >= BLOCK_HEIGHT ? BLOCK_HEIGHT - 1 : block.y;

    return block;
}

float lineLength(float x1, float y1, float x2, float y2) {
    return sqrtf(((x2 - x1) * (x2 - x1)) + ((y2 - y1) * (y2 - y1)));
}

void drawRays(SDL_Renderer **renderer, float angle, int idx) {
    intVec rayHit;
    floatVec hVec, vVec;
    float distToBound, finalDist, offsetX, offsetY;
    int checkLim, mapValue;
    float hLength = 1000000;
    float vLength = 1000000;

    if (angle < 0) {
        angle = angle += 2 * PI;
    } else if (angle > 2 * PI) {
        angle = angle - 2 * PI;
    }

    // Check horizontal lines
    if (angle > PI) {
        distToBound = fmod(state.charPos.y, (float) BLOCK_HEIGHT);
        state.rayDest.y = state.charPos.y - distToBound;
        state.rayDest.x = (-1 / tan(angle) * distToBound) + state.charPos.x;

        offsetY = -BLOCK_HEIGHT;
        offsetX = (1 / tan(angle) * offsetY);
    } else {
        distToBound = BLOCK_HEIGHT - fmod(state.charPos.y, (float) BLOCK_HEIGHT);
        state.rayDest.y = state.charPos.y + distToBound;
        state.rayDest.x = (1 / tan(angle) * distToBound) + state.charPos.x;

        offsetY = BLOCK_HEIGHT;
        offsetX = (1 / tan(angle) * offsetY);
    }

    if (angle > PI) {
        rayHit = whichBlock(clampF(state.rayDest.x, 0, LOG_WIDTH), clampF(state.rayDest.y, 0, LOG_HEIGHT) - 64);
        //rayHit = whichBlock(state.rayDest.x, state.rayDest.y - 64);
    } else {
        rayHit = whichBlock(clampF(state.rayDest.x, 0, LOG_WIDTH), clampF(state.rayDest.y, 0, LOG_HEIGHT));
        //rayHit = whichBlock(state.rayDest.x, state.rayDest.y);
    }

    checkLim = 0;
    mapValue = rayHit.y * MAP_WIDTH + rayHit.x;
    mapValue = mapValue < 0 ? 0 : mapValue;
    mapValue = mapValue >= MAP_WIDTH * MAP_HEIGHT ? MAP_WIDTH * MAP_HEIGHT - 1 : mapValue;
    while (checkLim < 8 && map[mapValue] != 1) {
        state.rayDest.x += offsetX;
        state.rayDest.y += offsetY;
        rayHit = whichBlock(clampF(state.rayDest.x, 0, LOG_WIDTH), clampF(state.rayDest.y, 0, LOG_HEIGHT));
        //rayHit = whichBlock(state.rayDest.x, state.rayDest.y);
        if (angle > PI) {
            rayHit.y -= 1;
        }
        checkLim++;
        mapValue = rayHit.y * MAP_WIDTH + rayHit.x;
        mapValue = mapValue < 0 ? 0 : mapValue;
        mapValue = mapValue >= MAP_WIDTH * MAP_HEIGHT ? MAP_WIDTH * MAP_HEIGHT - 1 : mapValue;
    }

    hVec = state.rayDest;
    hLength = lineLength(state.charPos.x, state.charPos.y, hVec.x, hVec.y);

    // Check vertical lines
    if (angle > 3 * PI / 2 || angle < PI / 2) {
        distToBound = fmod(state.charPos.x, (float) BLOCK_WIDTH) - BLOCK_WIDTH;
        state.rayDest.x = state.charPos.x - distToBound;
        state.rayDest.y = -(tan(angle) * distToBound) + state.charPos.y;
        offsetX = BLOCK_WIDTH;
        offsetY = (tan(angle) * offsetX);
    } else {
        distToBound = fmod(state.charPos.x, (float) BLOCK_WIDTH);
        state.rayDest.x = state.charPos.x - distToBound;
        state.rayDest.y = -(tan(angle) * distToBound) + state.charPos.y;
        offsetX = -(BLOCK_WIDTH);
        offsetY = -(tan(angle) * BLOCK_WIDTH);
    }
    //rayHit = whichBlock(state.rayDest.x, state.rayDest.y);
    rayHit = whichBlock(clampF(state.rayDest.x, 0, LOG_WIDTH), clampF(state.rayDest.y, 0, LOG_HEIGHT));
    if (angle < 3 * PI / 2 && angle > PI / 2) {
        rayHit.x -= 1;
    }
    checkLim = 0;
    mapValue = rayHit.y * MAP_WIDTH + rayHit.x;
    mapValue = mapValue < 0 ? 0 : mapValue;
    mapValue = mapValue >= MAP_WIDTH * MAP_HEIGHT ? MAP_WIDTH * MAP_HEIGHT - 1 : mapValue;
    while (checkLim < 8 && angle != 0.0f && map[mapValue] == 0) {
        state.rayDest.x += offsetX;
        state.rayDest.y += offsetY;
        //rayHit = whichBlock(state.rayDest.x, state.rayDest.y);
        rayHit = whichBlock(clampF(state.rayDest.x, 0, LOG_WIDTH), clampF(state.rayDest.y, 0, LOG_HEIGHT));
        if (angle < 3 * PI / 2 && angle > PI / 2) {
            rayHit.x -= 1;
        }
        checkLim++;
        mapValue = rayHit.y * MAP_WIDTH + rayHit.x;
        mapValue = mapValue < 0 ? 0 : mapValue;
        mapValue = mapValue >= MAP_WIDTH * MAP_HEIGHT ? MAP_WIDTH * MAP_HEIGHT - 1 : mapValue;
    }

    vVec = state.rayDest;
    vLength = lineLength(state.charPos.x, state.charPos.y, vVec.x, vVec.y);

    uint32_t colColor;
    if (vLength > hLength) {
        finalDist = hLength;
        colColor = 0xFF0000AA;
    } else {
        finalDist = vLength;
        colColor = 0x00FF00AA;
    }

    state.rayDest = hLength < vLength ? hVec : vVec;

    float maxDist = lineLength(BLOCK_WIDTH, BLOCK_HEIGHT, LOG_WIDTH - BLOCK_WIDTH - 1, LOG_HEIGHT - BLOCK_HEIGHT - 1);


    int unitsHit = (maxDist - finalDist) / 12;
    int remainingUnits = (BLOCK_HEIGHT - unitsHit) / 2;

    for (int j = 0; j < remainingUnits; j++) {
        drawPointToScale((idx * 8) + 1, (j * 8) + 8, 8, 0xFFFAD2AA);
    }

    for (int j = remainingUnits; j < remainingUnits + unitsHit; j++) {
        drawPointToScale((idx * 8) + 1, (j * 8) + 8, 8, colColor);
    }

    for (int j = remainingUnits + unitsHit; j < 64; j++) {
        drawPointToScale((idx * 8) + 1, (j * 8) + 8, 8, 0xAB7E4CAA);
    }

    // SDL_SetRenderDrawColor(*renderer, 0xFF, 0x00, 0x00, 0xFF );
    // SDL_RenderDrawLineF(*renderer, state.charPos.x, state.charPos.y,
    //     state.rayDest.x, state.rayDest.y);
}

int init(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture) {
    *window = SDL_CreateWindow( "Hello SDL WORLD", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_ALLOW_HIGHDPI );
    if ( NULL == window ) {
        printf("Could not create window: ");
        printf(SDL_GetError());
        return 1;
    }
    *renderer = SDL_CreateRenderer(*window, -1, 0);
    if ( NULL == renderer ) {
        printf("Could not create renderer: ");
        printf(SDL_GetError());
        return 1;
    }
    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, LOG_WIDTH, LOG_HEIGHT);
    if ( NULL == texture ) {
        printf("Could not create texture: ");
        printf(SDL_GetError());
        return 1;
    }

    state.pixels = malloc(LOG_WIDTH * LOG_HEIGHT * sizeof(uint32_t));
    memset(state.pixels, 0, LOG_WIDTH * LOG_HEIGHT * sizeof(uint32_t));

    state.charPos.x = 188.786331;
    state.charPos.y = 235.212631;

    state.charVel.x = 0.0;
    state.charVel.y = 0.0;

    // state.charDir.x = 1.0;
    // state.charDir.y = 0.0;
    state.angle = 5.755182;
    state.colIndex = 0;
    
    return 0;
}

int main(int argc, char *argv[]) {
    SDL_Init( SDL_INIT_VIDEO );
    int quit = 0;
    bool adjustRays;

    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    int rtn = init(&window, &renderer, &texture);
    if (rtn == 1) {
        return 1;
    }

    SDL_RenderSetLogicalSize(renderer, LOG_WIDTH, LOG_HEIGHT);

    int charSize = 8;
    float moveSpeed = 0.7;
    float turnSpeed = 0.006;

    SDL_Event event;
    while (!quit) {
        SDL_UpdateTexture(texture, NULL, state.pixels, LOG_WIDTH * sizeof(uint32_t));
        adjustRays = false;

        while (SDL_PollEvent(&event)) {
            switch( event.type ){
                case SDL_QUIT:
                    quit = 1;
                    break;
            }
        }

        memset(state.pixels, 0, LOG_WIDTH * LOG_HEIGHT * sizeof(uint32_t));

        // drawMap();
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00 );

        const uint8_t *keystate = SDL_GetKeyboardState(NULL);
        if (keystate[SDL_SCANCODE_LEFT]) {
            state.angle -= turnSpeed;
            if (state.angle < 0) {
                state.angle += 2 * PI;
            }
            state.charDir.x = cos(state.angle);
            state.charDir.y = sin(state.angle);
            adjustRays = true;
        }

        if (keystate[SDL_SCANCODE_RIGHT]) {
            state.angle += turnSpeed;
            if (state.angle > 2 * PI) {
                state.angle -= 2 * PI;
            }
            state.charDir.x = cos(state.angle);
            state.charDir.y = sin(state.angle);
            adjustRays = true;
        }

        if (keystate[SDL_SCANCODE_UP]) {
            floatVec normVel = normalizeVec(state.charDir);
            state.charPos.x += (normVel.x * moveSpeed);
            state.charPos.y += (normVel.y * moveSpeed);
            adjustRays = true;
        }

        if (keystate[SDL_SCANCODE_DOWN]) {
            floatVec normVel = normalizeVec(state.charDir);
            state.charPos.x -= (normVel.x * moveSpeed);
            state.charPos.y -= (normVel.y * moveSpeed);
            adjustRays = true;
        }

        drawPointToScale((int) state.charPos.x, (int) state.charPos.y, charSize, 0x00000000);

        int colIndex = 0;
        for (float i = -0.64; i <= 0.63; i += 0.02) {
            drawRays(&renderer, state.angle + i, colIndex);
            colIndex++;
        }
        //drawRays(&renderer, state.angle);

        // SDL_SetRenderDrawColor(renderer, 0xFF, 0x00, 0x00, 0xFF );
        // SDL_RenderDrawLineF(renderer, state.charPos.x, state.charPos.y,
        //     state.rayDest.x, state.rayDest.y);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    free(state.pixels);

    SDL_Quit();

    return EXIT_SUCCESS;
}