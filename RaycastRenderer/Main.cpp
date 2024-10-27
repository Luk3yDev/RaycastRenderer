#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
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
#define renderHeight 480

#define texWidth 64
#define texHeight 64

int worldMap[mapWidth][mapHeight];

SDL_Window* window = NULL;
SDL_Surface* screenSurface = NULL;

// Player variables
double posX = 2, posY = 2;
double dirX = -1, dirY = 0;
double planeX = 0, planeY = 0.66;
double moveSpeed = 1.8f;
double rotSpeed = 0.6f;

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

// HUD

SDL_Surface* uibg;

int numGuns = 1;
SDL_Surface* gunTextures[255];
int gunTexture = 0;

int gunOffsetX = 0;
int gunOffsetY = 0;
bool gunSwayRight = true;

bool canFire = true;
double fireCooldown = 1.0;

int numFaces = 1;
SDL_Surface* faceTextures[255];
int faceTexture = 0;

TTF_Font* font = NULL;

// AUDIO
Mix_Music* music = NULL;
Mix_Chunk* fire = NULL;

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

    // Calculate the pixel position
    int bpp = surface->format->BytesPerPixel;
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

void setPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
    // Check if the coordinates are within the surface bounds
    if (x < 0 || x >= surface->w || y < 0 || y >= surface->h) {
        return; // Out of bounds
    }

    // Lock the surface if it is not already locked
    if (SDL_MUSTLOCK(surface)) {
        if (SDL_LockSurface(surface) < 0) {
            std::cerr << "Unable to lock surface: " << SDL_GetError() << std::endl;
            return;
        }
    }

    // Get the address of the pixel
    Uint32* pixels = (Uint32*)surface->pixels;
    pixels[(y * surface->w) + x] = color; // Set the pixel color

    // Unlock the surface
    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }
}

void loadMedia()
{
    std::string nameOfFile = "ui/uibg.bmp";
    uibg = SDL_LoadBMP(nameOfFile.c_str());

    // Gun UI
    for (int i = 0; i < numGuns*2; i++) {
        std::string fileName = "ui/gun_" + std::to_string(i) + ".bmp";
        gunTextures[i] = SDL_LoadBMP(fileName.c_str());
        if (!gunTextures[i]) {
            std::cerr << "Failed to load UI texture! SDL_Error: " << SDL_GetError() << std::endl;
        }
    }
    // Face UI
    for (int i = 0; i < numFaces; i++) {
        std::string fileName = "ui/face_" + std::to_string(i) + ".bmp";
        faceTextures[i] = SDL_LoadBMP(fileName.c_str());
        if (!faceTextures[i]) {
            std::cerr << "Failed to load UI texture! SDL_Error: " << SDL_GetError() << std::endl;
        }
    }

    // Audio
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        printf("SDL_mixer could not initialize! SDL_mixer Error: %s\n", Mix_GetError());
    }

    music = Mix_LoadMUS("audio/music/e1m1.wav");
    if (music == NULL)
    {
        printf("Failed to load music! SDL_mixer Error: %s\n", Mix_GetError());
    }

    fire = Mix_LoadWAV("audio/pew.wav");
    if (fire == NULL)
    {
        printf("Failed to load sound effect! SDL_mixer Error: %s\n", Mix_GetError());
    }

    font = TTF_OpenFont("font/VCR_OSD_MONO_1.001.ttf", 36);
    if (font == NULL)
    {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
    }
}

void renderUI()
{   
    Uint32 colorKey = SDL_MapRGB(gunTextures[gunTexture]->format, 0x00, 0x00, 0x00); // Black color
    SDL_SetColorKey(gunTextures[gunTexture], SDL_TRUE, colorKey);

    //gunOffsetY = abs(gunOffsetX / 3);
    gunOffsetY = ((1.0f/200.0f) * (gunOffsetX * gunOffsetX));

    SDL_Rect gunRect = { screenWidth / 2 - (192/2) + gunOffsetX, 300 + gunOffsetY, 0, 0 };
    SDL_BlitSurface(gunTextures[gunTexture], NULL, screenSurface, &gunRect);

    //SDL_FillRect(screenSurface, UIBase, SDL_MapRGB(screenSurface->format, 0x14, 0x23, 0x14));
    SDL_Rect uibgRect = { 0, renderHeight, screenWidth, screenHeight - renderHeight };
    SDL_BlitSurface(uibg, NULL, screenSurface, &uibgRect);

    SDL_SetColorKey(faceTextures[faceTexture], SDL_TRUE, colorKey);

    SDL_Rect faceRect = { screenWidth / 2 - 72, screenHeight-160, 0, 0};
    SDL_BlitSurface(faceTextures[faceTexture], NULL, screenSurface, &faceRect); 

    // THIS CAUSES A MEMORY LEAK
    /*
    SDL_Color textColor = { 255, 0, 0 };

    SDL_Rect* ammoTextRect = new SDL_Rect{ 400, 500, 0, 0 };
    SDL_Surface* ammoTextSurface = TTF_RenderText_Solid(font, "AMMO: 100", textColor);

    SDL_BlitSurface(ammoTextSurface, NULL, screenSurface, ammoTextRect);
    */
}

void shoot()
{
    if (canFire) 
    {
        canFire = false;

        gunTexture = 1;
        Mix_PlayChannel(-1, fire, 0); 

        double rayDirX = dirX;
        double rayDirY = dirY;
        double rayPosX = posX;
        double rayPosY = posY;

        int hit = 0;
        int istep = 0;

        while (hit == 0)
        {
            rayPosX += rayDirX;
            rayPosY += rayDirY;

            for (int i = 0; i < numSprites; i++)
            {
                if ((int)rayPosX == sprite[i].x - 0.5f && (int)rayPosY == sprite[i].y - 0.5f)
                {
                    if (sprite[i].texture == spriteTextures[1]) sprite[i].texture = spriteTextures[8];
                    //printf("Hit sprite\n");
                    hit = 1;
                }
            }
            if (worldMap[(int)floor(rayPosX)][(int)floor(rayPosY)] != 0)
            {
                //worldMap[(int)floor(rayPosX)][(int)floor(rayPosY)] = 0;

                //printf("Hit wall\n");
                hit = 1;
            }
                
            if (istep > 100) hit = 1;
            istep++;
        }  
    }
}

SDL_Rect* floorRect = new SDL_Rect{ 0, renderHeight / 2, screenWidth, renderHeight / 2 };

void Update(float deltaTime)
{
    // Clear the screen
    SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));

    // Create the floor
    SDL_FillRect(screenSurface, floorRect, SDL_MapRGB(screenSurface->format, 0x12, 0x12, 0x12));

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

        // DDA
        while (hit == 0)
        {
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

        // Door?
        if (hit == 9)
        {
            mapX += dirX;
            mapY += dirY;

            if (side == 0)
            {
                sideDistX += deltaDistX / 2;

                if (worldMap[mapX][mapY] != 9)
                {
                    sideDistX -= deltaDistX / 2;
                }
            }
            else
            {
                sideDistY += deltaDistY / 2;

                if (worldMap[mapX][mapY] != 9)
                {
                    sideDistY -= deltaDistY / 2;
                }
            }
        }

        if (side == 0) perpWallDist = (sideDistX - deltaDistX);
        else           perpWallDist = (sideDistY - deltaDistY);

        int lineHeight = (int)(renderHeight / perpWallDist);

        int drawStart = -lineHeight / 2 + renderHeight / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + renderHeight / 2;
        if (drawEnd >= renderHeight) drawEnd = renderHeight - 1;

        double wallX; // Exactly where the wall was hit
        if (side == 0) wallX = posY + perpWallDist * rayDirY;
        else           wallX = posX + perpWallDist * rayDirX;
        wallX -= floor((wallX));

        double verticleScale = (double)lineHeight / (double)wallTextureSize;
        int sampleX = (int)floor((wallX * wallTextureSize)) % wallTextureSize;

        for (int y = 0; y < lineHeight; y++)
        {
            if (y + (renderHeight / 2) - (lineHeight / 2) > renderHeight) continue;
            if (y + (renderHeight / 2) - (lineHeight / 2) < 0) continue;
            int sampleY = (int)floor(y / verticleScale);

            SDL_Color rgb = getPixelColor(wallTextures[hit], sampleX, sampleY);
            if (side == 1)
            {
                rgb.r = rgb.r >> 1;
                rgb.g = rgb.g >> 1;
                rgb.b = rgb.b >> 1;
            }
            setPixel(screenSurface, x, y + (renderHeight / 2) - (lineHeight / 2), SDL_MapRGB(screenSurface->format, rgb.r, rgb.g, rgb.b));
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

        int spriteHeight = abs(int(renderHeight / (transformY)));

        int drawStartY = -spriteHeight / 2 + renderHeight / 2;
        if (drawStartY < 0) drawStartY = 0;
        int drawEndY = spriteHeight / 2 + renderHeight / 2;
        if (drawEndY >= renderHeight) drawEndY = renderHeight - 1;

        int spriteWidth = abs(int(renderHeight / (transformY)));
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
                    int d = (y) * 256 - renderHeight * 128 + spriteHeight * 128; // 256 and 128 factors avoids using floats
                    int texY = ((d * texHeight) / spriteHeight) / 256;
                    SDL_Color color = getPixelColor(sprite[spriteOrder[i]].texture, texX / 2, texY / 2);
                    
                    if (!SDL_MapRGB(screenSurface->format, color.r, color.b, color.g) == 0x00)
                    {
                        setPixel(screenSurface, slice, y, SDL_MapRGB(screenSurface->format, color.r, color.g, color.b));
                    }
                }
        }
    }

    renderUI();

    SDL_UpdateWindowSurface(window);
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
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else
        {
            screenSurface = SDL_GetWindowSurface(window);
            
            SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));

            SDL_UpdateWindowSurface(window);
        }
    }
    if (TTF_Init() == -1)
    {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
    }

    loadMap("maps/coolmap.rmap");
    loadMedia();

    //Mix_PlayMusic(music, -1);

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

        Update(deltaTime);

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
                case SDLK_LCTRL:
                    shoot();
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
            if (worldMap[int(posX + dirX * moveSpeed * deltaTime)][int(posY)] == false) posX += dirX * moveSpeed * deltaTime;
            if (worldMap[int(posX)][int(posY + dirY * moveSpeed * deltaTime)] == false) posY += dirY * moveSpeed * deltaTime; 
        }
        if (movingBackward)
        {
            if (worldMap[int(posX - dirX * moveSpeed * deltaTime)][int(posY)] == false) posX -= dirX * moveSpeed * deltaTime;
            if (worldMap[int(posX)][int(posY - dirY * moveSpeed * deltaTime)] == false) posY -= dirY * moveSpeed * deltaTime;
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
        if (moving)
        {
            if (gunSwayRight)
            {
                gunOffsetX += 300 * deltaTime;
                if (gunOffsetX > 80)
                {
                    gunSwayRight = false;
                }
            }
            else
            {
                gunOffsetX -= 300 * deltaTime;
                if (gunOffsetX < -80)
                {
                    gunSwayRight = true;
                }
            }
        }
        else
        {
            if (gunOffsetX > 0) gunOffsetX -= 300 * deltaTime;
            if (gunOffsetX < 0) gunOffsetX += 300 * deltaTime;
        }

        if (!canFire) fireCooldown -= deltaTime;
        if (fireCooldown <= 0)
        {
            canFire = true;
            gunTexture = 0;
            fireCooldown = 0.5f;
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}