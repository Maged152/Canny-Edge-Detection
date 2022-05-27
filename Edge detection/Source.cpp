#include <iostream>
#include<string>
#include<iomanip>
#include <algorithm>
#include <SDL.h>
#include<SDL_image.h>

//Screen dimension constants
const int SCREEN_WIDTH = 720;
const int SCREEN_HEIGHT = 500;


/**************sdl*************/
inline bool init_window(SDL_Window*& window,SDL_Surface* &screenSurface,std::string name,int W,int H,int flag)
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
        window = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_ALLOW_HIGHDPI);
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

inline bool load_image(SDL_Surface* &img, SDL_Surface*& screenSurface, const char *path)
{
    //Loading success flag
    bool success = true;
    //initialize the image
    if(!IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG))
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

inline void update_window(SDL_Window*& window, SDL_Surface*& source, SDL_Surface*& dist,int mw,int mh )
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

inline void close(SDL_Window*& window, SDL_Surface* &screenSurfacee)
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

/**********************************/
inline void RGBA2Gray(SDL_Surface* source, SDL_Surface* dist)
{
    SDL_Color col(0, 0, 0, 0);

    Uint32* image_pixels = (Uint32*)source->pixels;
    Uint32* result_pixels = (Uint32*)dist->pixels;
    Uint8 gray_scale_value = 0;

    for (int i = 0; i < dist->h * dist->w; i++)
    {
        SDL_GetRGBA(image_pixels[i], dist->format, &col.r, &col.g, &col.b, &col.a);
        gray_scale_value = 0.299 * col.r + 0.587 * col.g + 0.114 * col.b;
        result_pixels[i] = SDL_MapRGBA(dist->format, gray_scale_value, gray_scale_value, gray_scale_value, 255);
    }
}

inline void apply_Gauss_kernel_gray(SDL_Surface* source, SDL_Surface* dist, int*kernel,int size)
{
    SDL_Color col(0, 0, 0, 0);
    Uint32* image_pixels = (Uint32*)source->pixels;
    Uint32* result_pixels = (Uint32*)dist->pixels;
    int L = -1 * (size >> 1);
    int H = (size >> 1) + 1;
    int ration = 0;

    
    for (int i = 0; i < size*size; i++)
        ration += kernel[i];
    
    for (int x_img = 0; x_img < source->w; x_img++)
    {
        for (int y_img = 0; y_img < source->h; y_img++)
        {
            float fsum = 0.0f;
            //iterate through the kernel
            for (int x_k = L; x_k < H; x_k++)
            {
                for (int y_k = L; y_k < H; y_k++)
                {
                    int xloc = x_img + x_k;
                    int yloc = y_img + y_k;
                    //out of boundry check
                    if (xloc > -1 && xloc<source->w && yloc>-1 && yloc < source->h)
                    {
                        SDL_GetRGBA(image_pixels[yloc * source->w + xloc], source->format, &col.r, &col.g, &col.b, &col.a);
                        fsum += col.r * kernel[(y_k-L)*size+(x_k-L)]; 
                    }

                }
            }
            fsum /= ration;

            // apply result
            result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(source->format, (int)fsum, (int)fsum, (int)fsum, (int)fsum);

        }

    }
}

void canny(SDL_Surface* source, SDL_Surface* dist, int* Gauss_kernel, int Gauss_size, int* Sobel_kernel_x, int* Sobel_kernel_y, int S_size)
{
    SDL_Color col(0, 0, 0, 0);
    Uint32* image_pixels = (Uint32*)source->pixels;
    Uint32* result_pixels = (Uint32*)dist->pixels;

    //1- convert image to gray scale
    RGBA2Gray(source, dist);

    //2- apply gaussian blur kernel
    apply_Gauss_kernel_gray(dist,dist, Gauss_kernel, Gauss_size);

    //3- sobel operation
    int* Gx = new int[source->w * source->h];  // gradient in x direction
    int* Gy = new int[source->w * source ->h];  // gradient in y direction
    int* G = new int[source->w * source->h];  // gradient
    float* theta = new float[source->w * source->h];  // oriantation of the edge
    int L = -1 * (S_size >> 1);
    int H = (S_size >> 1) + 1;
    Uint32* temp_pixels = (Uint32*)dist->pixels;
    
    for (int x_img = 0; x_img < source->w; x_img++)
    {
        for (int y_img = 0; y_img < source->h; y_img++)
        {
            float fsumx = 0.0f;
            float fsumy = 0.0f;
            float fsum = 0.0f;
            //iterate through the kernel
            for (int x_k =L; x_k < H; x_k++)
            {
                for (int y_k = L; y_k < H; y_k++)
                {
                    int xloc = x_img + x_k;
                    int yloc = y_img + y_k;
                    //out of boundry check
                    if (xloc > -1 && xloc<source->w && yloc>-1 && yloc < source->h)
                    {
                        SDL_GetRGBA(image_pixels[yloc * source->w + xloc], source->format, &col.r, &col.g, &col.b, &col.a);
                        fsumx += col.r * Sobel_kernel_x[(y_k - L) * S_size + (x_k - L)];
                        fsumy += col.r * Sobel_kernel_y[(y_k - L) * S_size + (x_k - L)];
                    }

                }
            }
            fsum = std::clamp((int)sqrtf(powf(fsumx, 2) + powf(fsumy, 2)),0,255);
            
            Gx[y_img * source->w + x_img] = (int)fsumx;
            Gy[y_img * source->w + x_img] = (int)fsumy;
            G[y_img * source->w + x_img] = (int)fsum;
            theta[y_img * source->w + x_img] = atanf(fsumy / fsumx) * 180.0f / M_PI;
            // apply result
            result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, (int)fsum, (int)fsum, (int)fsum, (int)fsum);

        }

    }
    // 4- non-maximum suppression
    // discritize the angle to neareast 45 degree
   
    for (int i = 0; i < source->h * source->w; i++)
    {
        if (theta[i] > -22.5 && theta[i] <= 22.5)
            theta[i] = 0;
        else if (theta[i] > 22.5 && theta[i] <= 67.5)
            theta[i] = 45;
        else if ((theta[i] > 67.5 && theta[i] <= 90) || (theta[i] > -90 && theta[i] <= -67.5))
            theta[i] = 90;
        else
            theta[i] = 135;
    }
    //local max check
    for (int x_img = 0; x_img < source->w; x_img++)
    {
        for (int y_img = 0; y_img < source->h; y_img++)
        {
            if (theta[y_img * source->w + x_img] == 0)
            {
                //check (x-1,y) and (x+1,y)
                if (x_img + 1 < source->w)
                {
                    if (G[y_img * source->w + x_img] < G[y_img * source->w + x_img + 1])
                    {
                        G[y_img * source->w + x_img] = 0;
                        result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                        continue;
                    }
                }
                if (x_img - 1 > -1)
                {
                    if (G[y_img * source->w + x_img] < G[y_img * source->w + x_img - 1])
                    {
                        G[y_img * source->w + x_img] = 0;
                        result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                    }
                }

            }
            else if (theta[y_img * source->w + x_img] == 45)
            {
                //check (x-1,y-1) and (x+1,y+1)
                if (x_img + 1 < source->w && y_img + 1 < source->h)
                {
                    if (G[y_img * source->w + x_img] < G[(y_img + 1) * source->w + x_img + 1])
                    {
                        G[y_img * source->w + x_img] = 0;
                        result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                        continue;
                    }
                }
                if (x_img - 1 > -1 && y_img - 1 > -1)
                {
                    if (G[y_img * source->w + x_img] < G[(y_img - 1) * source->w + x_img - 1])
                    {
                        G[y_img * source->w + x_img] = 0;
                        result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                    }
                }
            }
            else if (theta[y_img * source->w + x_img] == 90)
            {
                //check (x,y-1) and (x,y+1)
                if (y_img + 1 < source->h)
                {
                    if (G[y_img * source->w + x_img] < G[(y_img + 1) * source->w + x_img])
                    {
                        G[y_img * source->w + x_img] = 0;
                        result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                        continue;
                    }
                }
                if (y_img - 1 > -1)
                {
                    if (G[y_img * source->w + x_img] < G[(y_img - 1) * source->w + x_img])
                    {
                        G[y_img * source->w + x_img] = 0;
                        result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                    }
                }
            }
            else
            {
                //135
                //check (x-1,y+1) and (x+1,y-1)
                if (x_img + 1 < source->w && y_img - 1 > -1)
                {
                    if (G[y_img * source->w + x_img] < G[(y_img - 1) * source->w + x_img + 1])
                    {
                        G[y_img * source->w + x_img] = 0;
                        result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                        continue;
                    }
                }
                if (x_img - 1 > -1 && y_img + 1 < source->h)
                {
                    if (G[y_img * source->w + x_img] < G[(y_img + 1) * source->w + x_img - 1])
                    {
                        G[y_img * source->w + x_img] = 0;
                        result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                    }
                }
            }


        }

    }
    // 5- Hysteresis thresholding
    //calculate the mean of gradient
    float mean = 0.0f;
    int no_of_pixels = 0;
    #pragma omp simd reduction(+:mean,no_of_pixels)
    for (int i = 0; i < source->w * source->h; i++)
    {
        if (G[i] > 17)
        {
            mean += G[i];
            no_of_pixels++;
        }
    }
    mean /= no_of_pixels;
    // adaptive threshold depending on the mean
    int low_threshold = std::clamp(int(mean - 0.4 * mean), 0, 255);
    int high_threshold = std::clamp((int)(mean + mean * 1.2), 0, 255);
    for (int x_img = 0; x_img < source->w; x_img++)
    {
        for (int y_img = 0; y_img < source->h; y_img++)
        {
            if (G[y_img * source->w + x_img] > high_threshold)
            {
                //keep it
                result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 255, 255, 255, 255);
                G[y_img * source->w + x_img] = 255;
            }
            else if (G[y_img * source->w + x_img] < low_threshold)
            {
                // discard this edge
                result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                G[y_img * source->w + x_img] = 0;
            }
            else
            {
                // chaeck 8-neighbours
                bool is_edge = false;
                for (int x_n = -1; x_n < 2; x_n++)
                {
                    for (int y_n = -1; y_n < 2; y_n++)
                    {
                        int xloc = x_img + x_n;
                        int yloc = y_img + y_n;
                        //out of boundry check
                        if (xloc > -1 && xloc<source->w && yloc>-1 && yloc < source->h)
                        {
                            if (G[yloc * source->w + xloc] > high_threshold)
                            {
                                is_edge = true;
                                break;
                            }
                        }

                    }
                }
                if (is_edge)
                {
                    result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 255, 255, 255, 255);
                    G[y_img * source->w + x_img] = 255;
                }

            }

        }

    }
    //double check and eliminate weak edges
    for (int x_img = 0; x_img < source->w; x_img++)
    {
        for (int y_img = 0; y_img < source->h; y_img++)
        {
            if (G[y_img * source->w + x_img]<high_threshold && G[y_img * source->w + x_img]>low_threshold)
            {
                // chaeck 8-neighbours
                bool is_edge = false;
                for (int x_n = -1; x_n < 2; x_n++)
                {
                    for (int y_n = -1; y_n < 2; y_n++)
                    {
                        int xloc = x_img + x_n;
                        int yloc = y_img + y_n;
                        //out of boundry check
                        if (xloc > -1 && xloc<source->w && yloc>-1 && yloc < source->h)
                        {
                            if (G[yloc * source->w + xloc] > high_threshold)
                            {
                                is_edge = true;
                                break;
                            }
                        }

                    }
                }
                if (is_edge)
                {
                    result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 255, 255, 255, 255);
                    G[y_img * source->w + x_img] = 255;
                }
                else
                {
                    result_pixels[y_img * source->w + x_img] = SDL_MapRGBA(dist->format, 0, 0, 0, 0);
                    G[y_img * source->w + x_img] = 0;
                }
            }

        }
    }
    delete[]G;
    delete[]Gx;
    delete[]Gy;
    delete[]theta;

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
        system("pause");
        return 1;
    }
    if (!init_window(window_edge, Screensurface_edge, "Edge Detection ", SCREEN_WIDTH, SCREEN_HEIGHT, SDL_INIT_VIDEO))
    {
        std::cout << "failed to create window !!\n";
        system("pause");
        return 1;
    }
    //load image
    std::string path;
    std::cout << "enter the image path: ";
    std::cin >> path;
    if (!load_image(image, Screensurface_original, path.c_str()))
    {
        system("pause");
        return 1;
    }
    image_edge = SDL_CreateRGBSurfaceWithFormat(0, image->w, image->h, 8, SDL_PIXELFORMAT_RGBA8888);
    update_window(window_original, image, Screensurface_original,SCREEN_WIDTH,SCREEN_HEIGHT);
    SDL_LockSurface(Screensurface_edge);
    /********apply edge detection*******/
  
    int kernel_blur[9] =
    {
        1, 2, 1,
        2, 4, 2,
        1, 2, 1
    };

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


    canny(image, image_edge, kernel_blur, 3, kernel_Gx, kernel_Gy, 3);
    IMG_SaveJPG(image_edge, "out4.jpg",4000);

    /**************end of algorithm***************/
    SDL_UnlockSurface(Screensurface_edge);
    //show result image
    update_window(window_edge, image_edge, Screensurface_edge, SCREEN_WIDTH, SCREEN_HEIGHT);
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