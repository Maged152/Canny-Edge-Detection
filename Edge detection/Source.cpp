#include <iostream>
#include<string>
#include <SDL.h>
#include<SDL_image.h>

//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;



bool init_window(SDL_Window*& window,SDL_Surface* &screenSurface,std::string name,int W,int H,int flag)
{
    //Initialization flag
    bool success = true;
    // Initialize SDL
    if (SDL_Init(flag) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        success = false;
    }
    else
    {
        //Create window
        window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
            success = false;
        }
        else
        {
            //Get window surface
            screenSurface = SDL_GetWindowSurface(window);
        }
    }
    return success;
}

bool load_image(SDL_Surface* &img, SDL_Surface*& screenSurface, const char *path)
{
    //Loading success flag
    bool success = true;
    //initialize the image
    if(!IMG_Init(IMG_INIT_JPG |IMG_INIT_PNG))
    {
        std::cout << "Could not initiate SDL_image: " << IMG_GetError() << std::endl;
        success = false;
    }
    //load image
    img = IMG_Load(path);
    if (img == nullptr) {
        std::cout << "Can't load the image : " << IMG_GetError() << "\n";
        success = false;
    }
    img= SDL_ConvertSurface(img, screenSurface->format, 0);


    return success;

}

void update_window(SDL_Window*& window, SDL_Surface*& source, SDL_Surface*& dist,int mw,int mh )
{
    // Apply the image stretched
     SDL_Rect stretchRect;
    stretchRect.x = 0;
    stretchRect.y = 0;
    stretchRect.w = mw;
    stretchRect.h = mh;
    SDL_BlitScaled(source, NULL, dist, &stretchRect);
    SDL_UpdateWindowSurface(window);
}

void close(SDL_Window*& window, SDL_Surface* &screenSurfacee)
{

    //Deallocate surface
    SDL_FreeSurface(screenSurfacee);
    //Destroy window
    SDL_DestroyWindow(window);
    screenSurfacee = NULL;
    window = NULL;
    IMG_Quit();
    SDL_Quit();
}


int main(int argc, char** args) 
{
    //Main loop flag
    bool quit = false;
    //Event handler
    SDL_Event e;
    //The window we'll be rendering to
    SDL_Window* window_original = NULL;
    SDL_Window* window_edge = NULL;
    //The surface contained by the window
    SDL_Surface* image = NULL;
    SDL_Surface* image_edge = NULL;
    SDL_Surface* Screensurface_original = NULL;
    SDL_Surface* Screensurface_edge = NULL;
    //Starts up SDL and creates window
    if (!init_window(window_original, Screensurface_original, "Original Image", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_INIT_VIDEO))
    {
        std::cout << "failed to create window !!\n";
        return 1;
    }
    if (!init_window(window_edge, Screensurface_edge, "Edge Detection ", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_INIT_VIDEO))
    {
        std::cout << "failed to create window !!\n";
        return 1;
    }
    //load image
    std::string path;
    std::cout << "enter the image path: ";
    std::cin >> path;
    if (!load_image(image, Screensurface_original, path.c_str()))
    {
        std::cout << "failed to open image !!\n";
        return 1;
    }
    image_edge = SDL_CreateRGBSurfaceWithFormat(0, image->w, image->h, 8, SDL_PIXELFORMAT_RGBA8888);
    update_window(window_original, image, Screensurface_original,SCREEN_WIDTH,SCREEN_HEIGHT);
    SDL_LockSurface(Screensurface_edge);
    /********apply edge detection*******/

    //1- convert image to gray scale
    SDL_Color col(0, 0, 0, 0);
    
    Uint32* image_pixels = (Uint32*)image->pixels;
    Uint32* result_pixels= (Uint32*)image_edge->pixels;
    Uint8 gray_scale_value = 0;

    for (int i = 0; i < image->h * image->w; i++)
    {
        SDL_GetRGBA(image_pixels[i], image->format, &col.r, &col.g, &col.b, &col.a);
        gray_scale_value = 0.299 * col.r+0.587* col.g+0.114* col.b;
        result_pixels[i] = SDL_MapRGBA(image_edge->format, gray_scale_value, gray_scale_value, gray_scale_value,255);
    }
    
    //2- apply gaussian blur kernel
    int kernel_blur[9] =
    {
        1, 2, 1,
        2, 4, 2,
        1, 2, 1
    };
    for (int x_img = 0; x_img < image->w; x_img++)
    {
        for (int y_img = 0; y_img < image->h; y_img++)
        {
            float fsum = 0.0f;
            //iterate through the kernel
            for (int x_k = -1; x_k < 2; x_k++)
            {
                for (int y_k = -1; y_k < 2; y_k++)
                {
                    int xloc = x_img + x_k;
                    int yloc = y_img + y_k;
                    //out of boundry check
                    if (xloc>-1 && xloc<image->w  && yloc>-1 && yloc<image->h )
                    {
                        SDL_GetRGBA(image_pixels[yloc * image->w + xloc], image->format, &col.r, &col.g, &col.b, &col.a);
                        fsum += col.r * kernel_blur[3 * y_k + x_k + 4]; //(y+1)*3+(x+1) 
                    }

                }
            }
            fsum /= 16;
            
            // apply result
            result_pixels[y_img*image->w+x_img] = SDL_MapRGBA(image_edge->format,(int)fsum, (int)fsum, (int)fsum, 255);

        }

    }

    //3- sobel operation
    int kernel_Gx[9] =
    {
        -1, 0, 1,
        -2, 0, 2,
        -1, 0, 1
    };
    int kernel_Gy[9] =
    {
        -1, -2, -1,
        0, 0, 0,
        1, 2, 1
    };
    int* Gx = new int[image->w * image->h];  // gradient in x direction
    int* Gy = new int[image->w * image->h];  // gradient in y direction
    int* G = new int[image->w * image->h];  // gradient
    float* theta = new float[image->w * image->h];  // oriantation of the edge

    for (int x_img = 0; x_img < image->w; x_img++)
    {
        for (int y_img = 0; y_img < image->h; y_img++)
        {
            float fsumx = 0.0f;
            float fsumy = 0.0f;
            float fsum = 0.0f;
            //iterate through the kernel
            for (int x_k = -1; x_k < 2; x_k++)
            {
                for (int y_k = -1; y_k < 2; y_k++)
                {
                    int xloc = x_img + x_k;
                    int yloc = y_img + y_k;
                    //out of boundry check
                    if (xloc > -1 && xloc<image->w && yloc>-1 && yloc < image->h)
                    {
                        SDL_GetRGBA(image_pixels[yloc * image->w + xloc], image->format, &col.r, &col.g, &col.b, &col.a);
                        fsumx += col.r * kernel_Gx[3 * y_k + x_k + 4]; //(y+1)*3+(x+1) 
                        fsumy += col.r * kernel_Gy[3 * y_k + x_k + 4];
                    }

                }
            }
            fsum = sqrtf(powf(fsumx, 2) + powf(fsumy, 2));

            Gx[y_img * image->w + x_img] = (int)fsumx;
            Gy[y_img * image->w + x_img] = (int)fsumy;
            G[y_img * image->w + x_img] = (int)fsum;
            theta[y_img * image->w + x_img] = atanf(fsumy/fsumx);
            // apply result
            result_pixels[y_img * image->w + x_img] = SDL_MapRGBA(image_edge->format, (int)fsum, (int)fsum, (int)fsum, (int)fsum);

        }

    }
  
    // 4- non-maximum suppression

    SDL_UnlockSurface(Screensurface_edge);
    //show result image
    update_window(window_edge, image_edge, Screensurface_edge, SCREEN_WIDTH, SCREEN_HEIGHT);
    /*********************************/
    while (!quit)
    {
        //Handle events on queue
        while (SDL_PollEvent(&e))
        {
            switch (e.window.event)
            {
              //User requests quit
              case SDL_WINDOWEVENT_CLOSE:
                  quit = true;
                  break;
              case SDL_WINDOWEVENT_SIZE_CHANGED:
                  //int mWidth = e.window.data1;
                 // int mHeight = e.window.data2;
                 // update_window(window_edge, Screensurface_edge, Screensurface_edge, mWidth, mHeight);
                  break;

            }
               
        }

    }
 
    //close SDL
    SDL_FreeSurface(image);
    SDL_FreeSurface(image_edge);
    close(window_edge,Screensurface_edge);
    close(window_original, Screensurface_original);
        
    return 0;

 }