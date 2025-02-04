#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#define mapWidth 25
#define mapHeight 25
#define screenWidth 640
#define screenHeight 640

#define texWidth 64
#define texHeight 64

int worldMap[mapWidth][mapHeight];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int bpp;

// Player variables
double posX = 2, posY = 2;
double dirX = -1, dirY = 0;
double planeX = 0, planeY = 0.66;
double moveSpeed = 1.8f;
double rotSpeed = 0.8f;

const int wallTextureSize = 64;
const int wallTypes = 10; // Must always be 1 higher than the actual amount of tile textures, as air (0) counts as a wall type
SDL_Surface* wallTextures[wallTypes];

struct Sprite
{
    double x;
    double y;
    SDL_Surface* texture;
};

int numSprites = 1;
int spriteTypes = 8;

Sprite sprite[255];
SDL_Surface* spriteTextures[255];

double ZBuffer[screenWidth];

int spriteOrder[255];
double spriteDistance[255];

void sortSprites(int* order, double* dist, int amount)
{
    std::vector<std::pair<double, int>> sprites(amount);
    for (int i = 0; i < amount; i++) {
        sprites[i].first = dist[i];
        sprites[i].second = order[i];
    }
    std::sort(sprites.begin(), sprites.end());
    // restore in reverse order to go from farthest to nearest
    for (int i = 0; i < amount; i++) {
        dist[i] = sprites[amount - i - 1].first;
        order[i] = sprites[amount - i - 1].second;
    }
}

void loadMap(const std::string& filename) {
    // Load wall textures
    for (int i = 1; i < wallTypes; i++) {
        std::string fileName = "walls/tile_" + std::to_string(i) + ".bmp";
        wallTextures[i] = SDL_LoadBMP(fileName.c_str());
        if (!wallTextures[i]) {
            std::cerr << "Failed to load wall texture! SDL_Error: " << SDL_GetError() << std::endl;
        }
    }
    // Load sprite textures
    for (int i = 1; i <= spriteTypes; i++) {
        std::string fileName = "sprites/sprite_" + std::to_string(i) + ".bmp";
        spriteTextures[i] = SDL_LoadBMP(fileName.c_str());
        if (!spriteTextures[i]) {
            std::cerr << "Failed to load sprite texture! SDL_Error: " << SDL_GetError() << std::endl;
        }
    }

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file for reading.\n";
        return;
    }

    std::string line;
    // Skip to the line containing the map data
    while (std::getline(file, line)) {
        if (line.find("int worldMap") != std::string::npos) {
            break;
        }
    }

    for (int y = 0; y < mapHeight; y++) {
        std::getline(file, line);
        std::stringstream ss(line);
        std::string temp;

        // Expecting a line starting with a brace
        std::getline(ss, temp, '{');
        for (int x = 0; x < mapWidth; x++) {
            int mapValue;
            ss >> mapValue;
            worldMap[y][x] = mapValue;

            // Read until the next comma or closing brace
            std::getline(ss, temp, (x < mapWidth - 1) ? ',' : '}');
        }
    }

    // Skip to the line containing the sprite data
    while (std::getline(file, line)) {
        if (line.find("int spriteMap") != std::string::npos) {
            break;
        }
    }

    for (int y = 0; y < mapHeight; y++) {
        std::getline(file, line);
        std::stringstream ss(line);
        std::string temp;

        // Expecting a line starting with a brace
        std::getline(ss, temp, '{');
        for (int x = 0; x < mapWidth; x++) {          
            // Read until the next comma or closing brace
            std::getline(ss, temp, (x < mapWidth - 1) ? ',' : '}');
            int spriteValue;
            ss >> spriteValue;
            if (spriteValue == 0) continue;
            numSprites++;

            // Apply sprite data
            sprite[numSprites].y = x + 1.5f;
            sprite[numSprites].x = y + 0.5f;
            sprite[numSprites].texture = spriteTextures[spriteValue];
        }
    }

    file.close();
    std::cout << "Loaded Map " << filename << "\n";
}

SDL_Color getPixelColor(SDL_Surface* surface, int x, int y) {
    SDL_Color color = { 0, 0, 0, 255 }; // Default color (black)

    // Check if the surface is valid
    if (surface == nullptr) {
        std::cerr << "Surface is null!" << std::endl;
        return color;
    }

    // Lock the surface if necessary
    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    Uint8* pixel = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;

    // Get the color based on the pixel format
    switch (bpp) {
    case 1: // 8-bit
        color.r = color.g = color.b = *pixel; // Grayscale
        break;
    case 2: // 16-bit
    {
        Uint16 p = *(Uint16*)pixel;
        SDL_GetRGB(p, surface->format, &color.r, &color.g, &color.b);
    }
    break;
    case 3: // 24-bit
        color.b = *pixel++;
        color.g = *pixel++;
        color.r = *pixel++;
        break;
    case 4: // 32-bit
        Uint32 p = *(Uint32*)pixel;
        SDL_GetRGBA(p, surface->format, &color.r, &color.g, &color.b, &color.a);
        break;
    }

    // Unlock the surface if it was locked
    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }

    return color;
}

int main(int argc, char* args[])
{
    bool movingForward = false;
    bool movingBackward = false;
    bool turningRight = false;
    bool turningLeft = false;

    bool moving = false;
    
    SDL_Event event;

    // Init
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }       
        else if (renderer == NULL)
        {
            printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        }
    }

    loadMap("maps/2.rmap");

    SDL_Texture* frameBuffer = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        screenWidth,
        screenHeight);

    if (!frameBuffer) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;

    // Main loop
    bool done = false;
    while (!done)
    {
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = ((NOW - LAST) * 3 / (double)SDL_GetPerformanceFrequency());

        double oldDirX = dirX;
        double oldPlaneX = planeX;

        // Input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_KEYDOWN)
            {
                /* Check the SDLKey values and move change the coords */
                switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    turningLeft = true;
                    break;
                case SDLK_RIGHT:
                    turningRight = true;
                    break;
                case SDLK_UP:
                    movingForward = true;
                    moving = true;
                    break;
                case SDLK_DOWN:
                    movingBackward = true;
                    moving = true;
                    break;
                case SDLK_ESCAPE:
                    done = true;
                    break;
                default:
                    break;
                }
            }
            if (event.type == SDL_KEYUP)
            {
                /* Check the SDLKey values and move change the coords */
                switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    turningLeft = false;
                    break;
                case SDLK_RIGHT:
                    turningRight = false;
                    break;
                case SDLK_UP:
                    movingForward = false;
                    moving = false;
                    break;
                case SDLK_DOWN:
                    movingBackward = false;
                    moving = false;
                    break;
                default:
                    break;
                }
            }
            if (event.type == SDL_QUIT) done = true;
        }
        
        // Applying input
        if (movingForward)
        {
            if (worldMap[int(posX + dirX * moveSpeed*4 * deltaTime)][int(posY)] == 0) posX += dirX * moveSpeed * deltaTime;
            if (worldMap[int(posX)][int(posY + dirY * moveSpeed*4 * deltaTime)] == 0) posY += dirY * moveSpeed * deltaTime;
        }
        if (movingBackward)
        {
            if (worldMap[int(posX - dirX * moveSpeed*4 * deltaTime)][int(posY)] == 0) posX -= dirX * moveSpeed * deltaTime;
            if (worldMap[int(posX)][int(posY - dirY * moveSpeed*4 * deltaTime)] == 0) posY -= dirY * moveSpeed * deltaTime;
        }
        if (turningRight)
        {
            dirX = dirX * cos(-rotSpeed * deltaTime) - dirY * sin(-rotSpeed * deltaTime);
            dirY = oldDirX * sin(-rotSpeed * deltaTime) + dirY * cos(-rotSpeed * deltaTime);
            planeX = planeX * cos(-rotSpeed * deltaTime) - planeY * sin(-rotSpeed * deltaTime);
            planeY = oldPlaneX * sin(-rotSpeed * deltaTime) + planeY * cos(-rotSpeed * deltaTime);
        }
        if (turningLeft)
        {
            dirX = dirX * cos(rotSpeed * deltaTime) - dirY * sin(rotSpeed * deltaTime);
            dirY = oldDirX * sin(rotSpeed * deltaTime) + dirY * cos(rotSpeed * deltaTime);
            planeX = planeX * cos(rotSpeed * deltaTime) - planeY * sin(rotSpeed * deltaTime);
            planeY = oldPlaneX * sin(rotSpeed * deltaTime) + planeY * cos(rotSpeed * deltaTime);
        }

        uint32_t* pixels = nullptr;
        int pitch = 0;

        // Lock the texture for pixel manipulation
        if (SDL_LockTexture(frameBuffer, nullptr, (void**)&pixels, &pitch) != 0) {
            std::cerr << "SDL_LockTexture failed: " << SDL_GetError() << std::endl;
        }

        // RAYCAST
        for (int x = 0; x < screenWidth; x++)
        {
            double cameraX = 2 * x / (double)screenWidth - 1;
            double rayDirX = dirX + planeX * cameraX;
            double rayDirY = dirY + planeY * cameraX;

            int mapX = int(posX);
            int mapY = int(posY);

            double sideDistX;
            double sideDistY;

            double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
            double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);

            double perpWallDist;

            int stepX;
            int stepY;

            int hit = 0;
            int side;

            if (rayDirX < 0)
            {
                stepX = -1;
                sideDistX = (posX - mapX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (mapX + 1.0 - posX) * deltaDistX;
            }
            if (rayDirY < 0)
            {
                stepY = -1;
                sideDistY = (posY - mapY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (mapY + 1.0 - posY) * deltaDistY;
            }

            int distfade = 0;

            // DDA
            while (hit == 0)
            {
                if (distfade < 255) distfade += 10; // Higher value = view range shorter / 'darker room'
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                }
                else
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }

                if (worldMap[mapX][mapY] > 0)
                {
                    hit = worldMap[mapX][mapY];
                    if (hit >= wallTypes) hit = 1;
                }
            }

            if (side == 0) perpWallDist = (sideDistX - deltaDistX);
            else           perpWallDist = (sideDistY - deltaDistY);

            int lineHeight = (int)(screenHeight / perpWallDist);

            int drawStart = -lineHeight / 2 + screenHeight / 2;
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + screenHeight / 2;
            if (drawEnd >= screenHeight) drawEnd = screenHeight - 1;

            double wallX; // Exactly where the wall was hit
            if (side == 0) wallX = posY + perpWallDist * rayDirY;
            else           wallX = posX + perpWallDist * rayDirX;
            wallX -= floor((wallX));

            double verticleScale = (double)lineHeight / (double)wallTextureSize;
            int sampleX = (int)floor((wallX * wallTextureSize)) % wallTextureSize;

            for (int y = 0; y < lineHeight; y++)
            {
                if (y + (screenHeight / 2) - (lineHeight / 2) > screenHeight) continue;
                if (y + (screenHeight / 2) - (lineHeight / 2) < 0) continue;
                int sampleY = (int)floor(y / verticleScale);

                SDL_Color rgb = getPixelColor(wallTextures[hit], sampleX, sampleY);
                if (rgb.r - distfade < 0) rgb.r = 0;
                else rgb.r -= distfade;
                if (rgb.g - distfade < 0) rgb.g = 0;
                else rgb.g -= distfade;
                if (rgb.b - distfade < 0) rgb.b = 0;
                else rgb.b -= distfade;

                if (side == 1)
                {
                    rgb.r = rgb.r >> 1;
                    rgb.g = rgb.g >> 1;
                    rgb.b = rgb.b >> 1;
                }
                //setPixel(screenSurface, x, y + (screenHeight / 2) - (lineHeight / 2), SDL_MapRGB(screenSurface->format, rgb.r, rgb.g, rgb.b));
                pixels[y + (screenHeight/2 )-(lineHeight/2)* (pitch / 4) + x] = SDL_MapRGB(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), rgb.r, rgb.g, rgb.b);
            }

            ZBuffer[x] = perpWallDist;
        }

        // SPRITECAST
        // Sprite sorting
        for (int i = 0; i < numSprites; i++)
        {
            spriteOrder[i] = i;
            spriteDistance[i] = ((posX - sprite[i].x) * (posX - sprite[i].x) + (posY - sprite[i].y) * (posY - sprite[i].y)); //sqrt not taken, unneeded
        }
        sortSprites(spriteOrder, spriteDistance, numSprites);
        for (int i = 0; i < numSprites; i++)
        {
            double spriteX = sprite[spriteOrder[i]].x - posX;
            double spriteY = sprite[spriteOrder[i]].y - posY;

            double invDet = 1.0 / (planeX * dirY - dirX * planeY);

            double transformX = invDet * (dirY * spriteX - dirX * spriteY);
            double transformY = invDet * (-planeY * spriteX + planeX * spriteY);

            int spriteScreenX = int((screenWidth / 2) * (1 + transformX / transformY));

            int spriteHeight = abs(int(screenHeight / (transformY)));

            int drawStartY = -spriteHeight / 2 + screenHeight / 2;
            if (drawStartY < 0) drawStartY = 0;
            int drawEndY = spriteHeight / 2 + screenHeight / 2;
            if (drawEndY >= screenHeight) drawEndY = screenHeight - 1;

            int spriteWidth = abs(int(screenHeight / (transformY)));
            int drawStartX = -spriteWidth / 2 + spriteScreenX;
            if (drawStartX < 0) drawStartX = 0;
            int drawEndX = spriteWidth / 2 + spriteScreenX;
            if (drawEndX >= screenWidth) drawEndX = screenWidth - 1;

            for (int slice = drawStartX; slice < drawEndX; slice++)
            {
                int texX = int(256 * (slice - (-spriteWidth / 2 + spriteScreenX)) * texWidth / spriteWidth) / 256;

                if (transformY > 0 && slice > 0 && slice < screenWidth && transformY < ZBuffer[slice])
                    for (int y = drawStartY; y < drawEndY; y++)
                    {
                        int d = (y) * 256 - screenHeight * 128 + spriteHeight * 128; // 256 and 128 factors avoids using floats
                        int texY = ((d * texHeight) / spriteHeight) / 256;
                        SDL_Color color = getPixelColor(sprite[spriteOrder[i]].texture, texX / 2, texY / 2);

                        if (!SDL_MapRGB(SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888), color.r, color.b, color.g) == 0x00)
                        {
                            //setPixel(screenSurface, slice, y, SDL_MapRGB(screenSurface->format, color.r, color.g, color.b));
                        }
                    }
            }
        }

        SDL_UnlockTexture(frameBuffer);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, frameBuffer, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        SDL_UpdateWindowSurface(window);
    }

    SDL_DestroyTexture(frameBuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}