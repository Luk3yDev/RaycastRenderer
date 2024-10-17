#include <SDL.h>
#include <stdio.h>
#include <cmath>
#include <string>
#include <vector>
#include <iostream>

#define mapWidth 24
#define mapHeight 24
#define screenWidth 640
#define screenHeight 480

int worldMap[screenWidth][screenHeight] =
{
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,1,0,0,0,0,1,1,0,0,0,0,1,1,1,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,1},
  {1,0,0,1,0,0,0,1,0,0,0,0,1,1,0,0,0,0,1,1,1,0,0,1},
  {1,0,0,1,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

SDL_Color UintToColor(Uint32 color)
{
    SDL_Color tempcol;
    tempcol.a = 255;
    tempcol.r = (color >> 16) & 0xFF;
    tempcol.g = (color >> 8) & 0xFF;
    tempcol.b = color & 0xFF;
    return tempcol;
}

int main(int argc, char* args[])
{
	double posX = 22, posY = 12;	
    double dirX = -1, dirY = 0;
	double planeX = 0, planeY = 0.66;
    double moveSpeed = 0.5f;
    double rotSpeed = 0.1f;

	SDL_Window* window = NULL;
	SDL_Surface* screenSurface = NULL;

    SDL_Event event;

    const int textureSize = 128;
    SDL_Surface* bricks = SDL_LoadBMP("brick.bmp");
    //SDL_BlitSurface(bricks, NULL, screenSurface, NULL);

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

    bool done = false;

	while (!done)
	{
        SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0x00, 0x00, 0x00));
        SDL_Rect* floorRect = new SDL_Rect{ 0, screenHeight / 2, screenWidth, screenHeight / 2 };
        SDL_FillRect(screenSurface, floorRect, SDL_MapRGB(screenSurface->format, 0x1A, 0x12, 0x10));

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

                if (worldMap[mapX][mapY] > 0) hit = 1;
            }

            if (side == 0) perpWallDist = (sideDistX - deltaDistX);
            else           perpWallDist = (sideDistY - deltaDistY);

            int lineHeight = (int)(screenHeight / perpWallDist);

            int drawStart = -lineHeight / 2 + screenHeight / 2;
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + screenHeight / 2;
            if (drawEnd >= screenHeight) drawEnd = screenHeight - 1;

            int distColor = perpWallDist * -8;

            int sampleX = floor(((stepX + stepY) % 1) * textureSize);
            float verticleScale = lineHeight / textureSize;

            /*
            for (int y = 0; y < lineHeight; y++)
            {
                SDL_Rect *slice = new SDL_Rect{ x, y + (screenHeight / 2) - (lineHeight / 2), 1, 1};

                uint32_t *pix = (uint32_t*) bricks->pixels;
                SDL_Color rgb = UintToColor(pix[ sampleX + (int)(floor((float)y * verticleScale) * bricks->w) ]);

                SDL_FillRect(screenSurface, slice, SDL_MapRGB(screenSurface->format, rgb.r, rgb.g, rgb.b));

                delete(slice);
            }
            */

            SDL_Color rgb = {distColor, distColor, distColor};
            SDL_Rect* slice = new SDL_Rect{ x, drawStart, 1, lineHeight };
            SDL_FillRect(screenSurface, slice, SDL_MapRGB(screenSurface->format, rgb.r, rgb.g, rgb.b));
            delete(slice);
        }

        SDL_UpdateWindowSurface(window);

        double oldDirX = dirX;
        double oldPlaneX = planeX;

        // Input
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                /* Look for a keypress */
            case SDL_KEYDOWN:
                /* Check the SDLKey values and move change the coords */
                switch (event.key.keysym.sym) {
                case SDLK_LEFT:     
                    dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
                    dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
                    planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
                    planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
                    break;
                case SDLK_RIGHT:
                    dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
                    dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
                    planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
                    planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
                    break;
                case SDLK_UP:
                    if (worldMap[int(posX + dirX * moveSpeed)][int(posY)] == false) posX += dirX * moveSpeed;
                    if (worldMap[int(posX)][int(posY + dirY * moveSpeed)] == false) posY += dirY * moveSpeed;
                    break;
                case SDLK_DOWN:
                    if (worldMap[int(posX - dirX * moveSpeed)][int(posY)] == false) posX -= dirX * moveSpeed;
                    if (worldMap[int(posX)][int(posY - dirY * moveSpeed)] == false) posY -= dirY * moveSpeed;
                    break;
                case SDLK_ESCAPE:
                    done = true;
                default:
                    break;
                }
            }
        }
	}

    SDL_DestroyWindow(window);
    SDL_Quit();

	return 0;
}