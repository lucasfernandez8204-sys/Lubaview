#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL_main.h>
#include "png.h"
#include "chunks.h"
#include "app.h"

int main(int argc, char** argv) {
    // Check args
    if(argc != 2) {
        printf("Usage: ./pngviewer <png_file>");
        return 0;
    }
    
    char *filepath = argv[1];

    // Get and check for valid file
    FILE *imgFile = fopen(filepath, "rb");

    if(imgFile == NULL) {
        fprintf(stderr, "Error, file not found.\n");
        return -1;
    }

    struct Image *png = InitImage();

    if(ValidateImage(imgFile, png) == -1) {
        return -1;
    }

    if(SetChannels(png) == -1) {
        fprintf(stderr, "Fatal error: Invalid color type.\n");
        return -1;
    }

    SetBytePer(png);
    SetDecomp(png);

    switch(Decompress(png)) {
        case -1:
            fprintf(stderr, "Error intializing decompression.\n");
            return -1;

        case -2:
            fprintf(stderr, "Error performing decompression.\n");
            return -1;

        case -3:
            fprintf(stderr, "Error after decompression, data may be corrupted.\n");
            return -1;
        
        default:
            break;
    }

    // Now that the data is decompressed, we must reverse the filter on each scanline.
    Unfilter(png);

    // printf("HERE!!\n");
    // printf("bytes in row: %i\n\n", png->bpr);
    // int bytes = 0;
    // for(int i = 0; i < png->height; i++) {
    //     uint8_t *curRow = &png->dataDecomp[i * (png->bpr)];
    //     printf("Row %i:\n", i);
    //     for(int j = 1; j < png->bpr; j++) {
    //         printf("%i\t", curRow[j]);
    //         bytes++;
    //     }

    //     printf("\n");
    // }

    // printf("Bytes: %i\n", bytes);

    struct App app = {0};
    app.running = 0;
    app.img = png;

    if(!AppInit(&app)) {
        fprintf(stderr, "An error occurred, exiting...\n");
        return -1;
    }

    app.running = 1;

    AppRun(&app);

    AppDeinit(&app);

    DeinitImage(png);
    png = NULL;
}