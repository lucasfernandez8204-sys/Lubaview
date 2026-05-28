#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>
#include <math.h>
#include "chunks.h"

typedef unsigned char uint8_t;
typedef unsigned int uint32_t;

struct Image {
    // Dimensions
    int width;
    int height;

    // Properties
    int bitDepth;
    int colorType;
    int numChannels;

    int bpp;    // Bytes per pixel
    int bpr;    // Bytes per row

    // Data
    uint8_t *dataRaw;
    int dataRawSize;

    uint8_t *dataDecomp;
    int dataDecompSize;
};

// Image functions
struct Image * InitImage();
void DeinitImage(struct Image * img);
int SetChannels(struct Image * img);
void SetBytePer(struct Image * img);
void SetDecomp(struct Image * img);
int ValidateImage(FILE * file, struct Image * img);

// PNG validity functions
int ValidSig(FILE * file);
int ValidChunk(FILE * file, struct Image * img);

// PNG transformation functions
int Decompress(struct Image * img);
int Unfilter(struct Image * img);

// Math and helper functions.
uint32_t Hex2Int(uint8_t * buffer, int length);
uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c);