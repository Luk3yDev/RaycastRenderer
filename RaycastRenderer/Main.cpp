#include <SDL.h>
#include <stdio.h>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

#define mapWidth 24
#define mapHeight 24
#define screenWidth 640
#define screenHeight 480

int worldMap[mapWidth][mapHeight] =
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,6,6,5,5,6,6,0,0,0,2,2,2,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,1},
  {1,0,0,1,0,0,0,1,0,5,5,6,6,5,5,0,0,0,2,2,2,0,0,1},
  {1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,6,6,6,6,6,6,0,0,0,0,4,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,5,5,5,5,5,5,0,0,0,2,2,2,0,0,1},
  {1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,1},
  {1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,2,2,2,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,3,3,3,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,3,3,3,0,0,1},
  {1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,3,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

void loadMap(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open file for reading.\n";
        return;
    }

    std::string line;
    // Skip to the line containing the map data
    while (std::getline(file, line)) {
        if (line.find("int worldMap") != std::string::npos) {
            // Start reading the actual map
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
            int tileValue;
            ss >> tileValue;
            worldMap[y][x] = tileValue;

            // Read until the next comma or closing brace
            std::getline(ss, temp, (x < mapWidth - 1) ? ',' : '}');
        }
    }

    file.close();
    std::cout << "Map loaded from " << filename << "\n";
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

int main(int argc, char* args[])
{
    loadMap("maps/0.rmap");

	double posX = 22, posY = 12;	
    double dirX = -1, dirY = 0;
	double planeX = 0, planeY = 0.66;
    double moveSpeed = 6.0f;
    double rotSpeed = 2.0f;

    bool movingForward = false;
    bool movingBackward = false;
    bool turningRight = false;
    bool turningLeft = false;

	SDL_Window* window = NULL;
	SDL_Surface* screenSurface = NULL;

    SDL_Event event;

    const int textureSize = 64;
    const int wallTypes = 7; // Must always be higher than the actual amount of tile textures, as air (0) counts as a wall type
    SDL_Surface* wallTextures[wallTypes];

    for (int i = 1; i < wallTypes; i++) {
        std::string fileName = "walls/tile_" + std::to_string(i) + ".bmp";       
        wallTextures[i] = SDL_LoadBMP(fileName.c_str());
        if (!wallTextures[i]) {
            std::cerr << "Failed to load wall texture! SDL_Error: " << SDL_GetError() << std::endl;
        }
        else {
            printf(("Loaded " + fileName + "\n").c_str());
        }
    }

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

    Uint64 NOW = SDL_GetPerformanceCounter();
    Uint64 LAST = 0;
    double deltaTime = 0;

    bool done = false;
	while (!done)
	{
        LAST = NOW;
        NOW = SDL_GetPerformanceCounter();
        deltaTime = (double)((NOW - LAST) * 1 / (double)SDL_GetPerformanceFrequency());

        // Clear the screen
        SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00)); 

        // Create the floor
        SDL_Rect* floorRect = new SDL_Rect{ 0, screenHeight / 2, screenWidth, screenHeight / 2 }; 
        SDL_FillRect(screenSurface, floorRect, SDL_MapRGB(screenSurface->format, 0x12, 0x12, 0x12));
        delete(floorRect);

        // Raycast
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
                    if (hit > wallTypes) hit = 1;
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
            
            float verticleScale = (float)lineHeight / (float)textureSize;
            int sampleX = (int)floor((wallX * textureSize)) % textureSize;

            for (int y = 0; y < lineHeight; y++)
            {   
                if (y + (screenHeight / 2) - (lineHeight / 2) > screenHeight) continue;
                if (y + (screenHeight / 2) - (lineHeight / 2) < 0) continue;
                int sampleY = (int)floor(y / verticleScale);
                
                SDL_Color rgb = getPixelColor(wallTextures[hit], sampleX, sampleY);
                setPixel(screenSurface, x, y + (screenHeight / 2) - (lineHeight / 2), SDL_MapRGB(screenSurface->format, rgb.r, rgb.g, rgb.b));
            }           
        }

        SDL_UpdateWindowSurface(window);

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
                    break;
                case SDLK_DOWN:
                    movingBackward = true;
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
                    break;
                case SDLK_DOWN:
                    movingBackward = false;
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
	}

    SDL_DestroyWindow(window);
    SDL_Quit();

	return 0;
}