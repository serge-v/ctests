#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#pragma pack(push, 1)

typedef struct tagBITMAPFILEHEADER
{
	uint16_t bfType;  //specifies the file type
	uint32_t bfSize;  //specifies the size in bytes of the bitmap file
	uint16_t bfReserved1;  //reserved; must be 0
	uint16_t bfReserved2;  //reserved; must be 0
	uint32_t bOffBits;  //species the offset in bytes from the bitmapfileheader to the bitmap bits
}BITMAPFILEHEADER;

#pragma pack(pop)

#pragma pack(push, 1)

typedef struct tagBITMAPINFOHEADER
{
	uint32_t biSize;  //specifies the number of bytes required by the struct
	int32_t biWidth;  //specifies width in pixels
	int32_t biHeight;  //species height in pixels
	uint16_t biPlanes; //specifies the number of color planes, must be 1
	uint16_t biBitCount; //specifies the number of bit per pixel
	uint32_t biCompression;//spcifies the type of compression
	uint32_t biSizeImage;  //size of image in bytes
	int32_t biXPelsPerMeter;  //number of pixels per meter in x axis
	int32_t biYPelsPerMeter;  //number of pixels per meter in y axis
	uint32_t biClrUsed;  //number of colors used by th ebitmap
	uint32_t biClrImportant;  //number of colors that are important
}BITMAPINFOHEADER;

#pragma pack(pop)

unsigned char *LoadBitmapFile(char *filename, BITMAPINFOHEADER *bitmapInfoHeader)
{
    FILE *filePtr; //our file pointer
    BITMAPFILEHEADER bitmapFileHeader; //our bitmap file header
    unsigned char *bitmapImage;  //store image data
    int imageIdx=0;  //image index counter
    unsigned char tempRGB;  //our swap variable

    //open filename in read binary mode
    filePtr = fopen(filename,"rb");
    if (filePtr == NULL)
        return NULL;

    //read the bitmap file header
    fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER),1,filePtr);

    //verify that this is a bmp file by check bitmap id
    if (bitmapFileHeader.bfType !=0x4D42)
    {
        fclose(filePtr);
        return NULL;
    }

    //read the bitmap info header
    fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER),1,filePtr);

    //move file point to the begging of bitmap data
    fseek(filePtr, bitmapFileHeader.bOffBits, SEEK_SET);

    //allocate enough memory for the bitmap image data
    bitmapImage = malloc(bitmapInfoHeader->biSizeImage);

    //verify memory allocation
    if (!bitmapImage)
    {
        free(bitmapImage);
        fclose(filePtr);
        return NULL;
    }

    //read in the bitmap image data
    fread(bitmapImage, 1, bitmapInfoHeader->biSizeImage, filePtr);

    //make sure bitmap image data was read
    if (bitmapImage == NULL)
    {
        fclose(filePtr);
        return NULL;
    }
/*
    //swap the r and b values to get RGB (bitmap is BGR)
    for (imageIdx = 0; imageIdx < bitmapInfoHeader->biSizeImage; imageIdx+=3)
    {
        tempRGB = bitmapImage[imageIdx];
        bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
        bitmapImage[imageIdx + 2] = tempRGB;
    }
*/
    //close file and return bitmap iamge data
    fclose(filePtr);
    return bitmapImage;
}

int main()
{
	printf("header size: %lu\n", sizeof(BITMAPFILEHEADER));
	printf("info size: %lu\n", sizeof(BITMAPINFOHEADER));

	BITMAPINFOHEADER info;
	unsigned char* data = NULL;
	
	data = LoadBitmapFile("1.bmp", &info);
	printf("data: %p\n", data);
	
	int x, y;
	int idx = 0;

	for (y = 0; y < info.biHeight; y++)
	{
		putc('[', stdout);
		for (x = 0; x < info.biWidth; x++)
		{
			idx += 3;
			
			char c = ' ';
			
			if (data[idx] < 100 || data[idx+1] < 100 || data[idx+2] < 100)
				c = 'x';
				
			putc(c, stdout);
		}
		
		if (info.biWidth % 4 != 0)
			idx += info.biWidth % 4;
		puts("]");
	}
	
	return 0;
}
