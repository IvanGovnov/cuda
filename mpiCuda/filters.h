#include <time.h>
#include <wand/magick_wand.h>

#define KERNEL_SIZE 3
#define MAX_PIXEL_VALUE 255

typedef signed int pixel_channel;
typedef unsigned long resolution;


pixel_channel kernel[KERNEL_SIZE][KERNEL_SIZE] =		{ -1, -1, -1,
								  -1,  9, -1, 
								  -1, -1, -1 };

#define ThrowWandException(wand) \
{ \
        char *description; \
        ExceptionType severity; \
        description=MagickGetException(wand,&severity); \
        printf("\n\n-----\n%s %s %lu %s\n",GetMagickModule(),description); \
        description=(char *) MagickRelinquishMemory(description); \
        exit(-1);\
}

typedef struct
{
  resolution height;
  resolution width;
  pixel_channel *arrayR;
  pixel_channel *arrayG;
  pixel_channel *arrayB;  
} wind_image;

void free_image(wind_image **img)
{
  if((*img) != NULL)
  {
    if((*img)->height > 0 && (*img)->width > 0)
    {
      free((*img)->arrayR);
      free((*img)->arrayG);
      free((*img)->arrayB);
      (*img)->height = 0;
      (*img)->width = 0; 
    }
    free((*img));
    (*img) = NULL;
  }
}

int new_image(char *image_full_name, wind_image **img)
{
  free_image(img);
  (*img) = (wind_image*)malloc(sizeof(wind_image));
  FILE *f = NULL;
  if((f = fopen(image_full_name, "rb")) == NULL)		// open image file descriptor
  {
    fprintf(stderr, "error open file %s", image_full_name);
    perror(": ");
    return -1;
  }
  MagickWandGenesis();						// initial MagikWand lib
  MagickWand *mw = NULL;					// image object
  mw = NewMagickWand();						// create a wand  
  if(MagickReadImageFile(mw, f) != MagickFalse)			// read image from open descriptor
  {
    (*img)->height = MagickGetImageHeight(mw);
    (*img)->width = MagickGetImageWidth(mw);			// allocate memory for RGB channels
    (*img)->arrayR = (pixel_channel*)malloc((*img)->width * (*img)->height * sizeof(pixel_channel));
    (*img)->arrayG = (pixel_channel*)malloc((*img)->width * (*img)->height * sizeof(pixel_channel));
    (*img)->arrayB = (pixel_channel*)malloc((*img)->width * (*img)->height * sizeof(pixel_channel));
  } 
  else
  {
    mw = DestroyMagickWand(mw);					// free memory magick_wand object
    fclose(f);
    ThrowWandException(mw);
  }  
  resolution ind_height, ind_width, tmp_width;
  PixelIterator *iterator = NULL;
  if((iterator = NewPixelIterator(mw)) == NULL)
  {   
    mw = DestroyMagickWand(mw);					// free memory magick_wand object
    fclose(f);
    ThrowWandException(mw);    
  }
  PixelWand **pixels = NULL;
  for(ind_height = 0; ind_height < (*img)->height; ind_height++)	// get RGB channels for each pixel
  {
    pixels = PixelGetNextIteratorRow(iterator, &tmp_width);	// get row pixels
    for(ind_width = 0; ind_width < (*img)->width; ind_width++)	// get RGB channels each pixel in row
    {
      (*img)->arrayR[ind_width + (*img)->width * ind_height] = (pixel_channel)(MAX_PIXEL_VALUE * PixelGetRed(pixels[ind_width]));
      (*img)->arrayG[ind_width + (*img)->width * ind_height] = (pixel_channel)(MAX_PIXEL_VALUE * PixelGetGreen(pixels[ind_width]));
      (*img)->arrayB[ind_width + (*img)->width * ind_height] = (pixel_channel)(MAX_PIXEL_VALUE * PixelGetBlue(pixels[ind_width]));
    }  
  }  
  iterator = DestroyPixelIterator(iterator);			// free memory magick_wand iterator
  mw = DestroyMagickWand(mw);					// free memory magick_wand object
  MagickWandTerminus();						// end work with MagikWand lib
  fclose(f);							// close file descriptor  
  return 0;
}

int save_image(char *image_full_name, wind_image **img)
{
  MagickWandGenesis();						// initial MagikWand lib
  MagickWand *mw = NULL;					// image object
  mw = NewMagickWand();						// create a wand
  PixelWand *tmp_pixel = NewPixelWand();
  PixelSetColor(tmp_pixel, "rgb(255,255,255)");			// set white pixel
  
  if(MagickNewImage(mw, (*img)->width, (*img)->height, tmp_pixel) == MagickFalse)
  {
    mw = DestroyMagickWand(mw);					// free memory magick_wand object
    tmp_pixel = DestroyPixelWand(tmp_pixel);
    ThrowWandException(mw);
  }
  PixelIterator *iterator = NULL;  
  if((iterator = NewPixelIterator(mw)) == NULL)
  {   
    mw = DestroyMagickWand(mw);					// free memory magick_wand object
    tmp_pixel = DestroyPixelWand(tmp_pixel);
    ThrowWandException(mw);    
  }  
  PixelWand **pixels = NULL;
  resolution ind_height, ind_width, tmp_width;  
  for(ind_height = 0; ind_height < (*img)->height; ind_height++)// get RGB channels for each pixel
  {
    pixels = PixelGetNextIteratorRow(iterator, &tmp_width);	// get row pixels
    for(ind_width = 0; ind_width < (*img)->width; ind_width++)	// get RGB channels each pixel in row
    {
      PixelSetRed(tmp_pixel,   ((double)((unsigned char)(*img)->arrayR[ind_width + (*img)->width * ind_height]))/MAX_PIXEL_VALUE);
      PixelSetGreen(tmp_pixel, ((double)((unsigned char)(*img)->arrayG[ind_width + (*img)->width * ind_height]))/MAX_PIXEL_VALUE);
      PixelSetBlue(tmp_pixel,  ((double)((unsigned char)(*img)->arrayB[ind_width + (*img)->width * ind_height]))/MAX_PIXEL_VALUE);
      pixels[ind_width] = ClonePixelWand(tmp_pixel);     
    }
    PixelSyncIterator(iterator);
  }    
  if(MagickWriteImage(mw, image_full_name) == MagickFalse)
  {
    tmp_pixel = DestroyPixelWand(tmp_pixel);
    mw = DestroyMagickWand(mw);					// free memory magick_wand object
    iterator = DestroyPixelIterator(iterator);
    ThrowWandException(mw);
  }
/*
 * this code is commented because there are bugs if free MagikWand memory objects and end work with MagikWand lib
 * with images more 1024x768 resolution (program will hangs and do not respond)
 */  
/*
  printf("freeMem start\n");
  tmp_pixel = DestroyPixelWand(tmp_pixel);
  iterator = DestroyPixelIterator(iterator);	
  mw = DestroyMagickWand(mw);					// free memory magick_wand object
  MagickWandTerminus();						// end work with MagikWand lib
  printf("freeMem end\n");
*/
  return 0;
}

double cpu_filter(wind_image **src_img, wind_image **result_img)
{
  resolution width = (*src_img)->width;
  resolution height = (*src_img)->height;
  resolution x, y, i, j, pixelPosX, pixelPosY;
  pixel_channel r, g, b, rSum, gSum, bSum;
  clock_t begin, end;
  free_image(result_img);
  (*result_img) = (wind_image*)malloc(sizeof(wind_image));
  (*result_img)->height = height;
  (*result_img)->width = width;
  (*result_img)->arrayR = (pixel_channel*)malloc(width * height * sizeof(pixel_channel));
  (*result_img)->arrayG = (pixel_channel*)malloc(width * height * sizeof(pixel_channel));
  (*result_img)->arrayB = (pixel_channel*)malloc(width * height * sizeof(pixel_channel));  
  begin = clock();
  for(x = 0; x < width; x++)
  for(y = 0; y < height; y++)
  {
    rSum = 0, gSum = 0, bSum = 0;
    for(i = 0; i < KERNEL_SIZE; i++)
      for(j = 0; j < KERNEL_SIZE; j++)
      {
	pixelPosX = x + i - 1;
	pixelPosY = y + j - 1;
	if ((pixelPosX < 0) || (pixelPosX >= width) | (pixelPosY < 0) || (pixelPosY >= height))
	  continue;
	r = (*src_img)->arrayR[(width * pixelPosY + pixelPosX)];
	g = (*src_img)->arrayG[(width * pixelPosY + pixelPosX)];
	b = (*src_img)->arrayB[(width * pixelPosY + pixelPosX)];
	rSum += r * kernel[i][j];
	gSum += g * kernel[i][j];
	bSum += b * kernel[i][j];
      }
      if (rSum < 0) rSum = 0;
      if (rSum > 255) rSum = 255;
      if (gSum < 0) gSum = 0;
      if (gSum > 255) gSum = 255;
      if (bSum < 0) bSum = 0;
      if (bSum > 255) bSum = 255;
      (*result_img)->arrayR[(width * y + x)] = (pixel_channel)rSum;
      (*result_img)->arrayG[(width * y + x)] = (pixel_channel)gSum;
      (*result_img)->arrayB[(width * y + x)] = (pixel_channel)bSum;
  }
  end = clock();
  return ((double)(end - begin) / (CLOCKS_PER_SEC/1000));
}

double cuda_shared_memory(wind_image **src_img)
{
  clock_t begin, end;
  begin = clock();
  Shared_Memory_Convolution(&((*src_img)->arrayR), (*src_img)->width, (*src_img)->height, kernel);
  Shared_Memory_Convolution(&((*src_img)->arrayG), (*src_img)->width, (*src_img)->height, kernel);
  Shared_Memory_Convolution(&((*src_img)->arrayB), (*src_img)->width, (*src_img)->height, kernel);
  end = clock();
  return ((double)(end - begin) / (CLOCKS_PER_SEC/1000));
}

double async_cuda_filter(wind_image **src_img)
{
  clock_t begin, end;  
  pixel_channel* RGB[3];
  RGB[0] = (*src_img)->arrayR;
  RGB[1] = (*src_img)->arrayG;
  RGB[2] = (*src_img)->arrayB;
  begin = clock();
  asyncConvolution(RGB, (*src_img)->width, (*src_img)->height);
  end = clock();
  (*src_img)->arrayR = RGB[0];
  (*src_img)->arrayG = RGB[1];
  (*src_img)->arrayB = RGB[2];
  return ((double)(end - begin) / (CLOCKS_PER_SEC/1000));  
}
