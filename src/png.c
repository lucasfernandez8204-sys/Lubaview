#include "png.h"



// -------------------- Image Functions -------------------- //



/* Create and initialize a PNG image with an empty rawData field. Data decomp not set
   until after bytes per pixel and bytes per scanline are calculated, so Decompress() and
   Unfilter() shall not be called until after SetDecomp() is called. */
struct Image * InitImage() {
    struct Image *png = malloc(sizeof(struct Image));
    png->dataRaw = NULL;
    png->dataRawSize = 0;

    return png;
}

/* Free data fields and image struct. */
void DeinitImage(struct Image * img) {
    free(img->dataRaw);
    img->dataRaw = NULL;

    free(img->dataDecomp);
    img->dataDecomp = NULL;

    free(img);

    return;
}

int SetChannels(struct Image * img) {
    switch(img->colorType) {
        case 0:
            img->numChannels = 1;
            break;

        case 2:
            img->numChannels = 3;
            break;

        case 3:
            img->numChannels = 1;
            break;

        case 4:
            img->numChannels = 2;
            break;

        case 6:
            img->numChannels = 4;
            break;

        default:
            return -1;
    }

    return 1;
}

/* Initialize the image's bytes per row and bytes per pixel field for use by SetDecomp() and
   Unfilter(). */
void SetBytePer(struct Image *img) {
    int bitsPerPixel = img->bitDepth * img->numChannels;
    int bytesPerPixel = (bitsPerPixel + 7) / 8;
    img->bpp = bytesPerPixel;

    int bitsPerRow = img->width * (bitsPerPixel);
    int bytesPerRow = (bitsPerRow + 7) / 8;
    img->bpr = bytesPerRow + 1;     // Includes filter byte.

    return;
}

/* Initialize the image's decompressed data field dataDecomp by calculating the bits and bytes
   per pixel as well as the bits and bytes per row. Additionally save bytes per pixel and bytes
   per row for */
void SetDecomp(struct Image * img) {
    img->dataDecompSize = (img->bpr) * img->height;
    img->dataDecomp = malloc(img->dataDecompSize);

    return;
}

int ValidateImage(FILE * file, struct Image * img) {
    if(ValidSig(file) == 0) {
        fprintf(stderr, "Error: Invalid file (MUST BE PNG).\n");
        return -1;
    }

    while(1) {
        int isValidChunk = ValidChunk(file, img);
        int canExit = 0;

        switch(isValidChunk) {
            case 1:
                break;
            
            case 0:
                int c = getc(file);

                if(c != EOF) {
                    fprintf(stderr, "Fatal error: Data after IEND.\n");
                    return -1;
                }

                canExit = 1;

                break;
            
            case -1:
                fprintf(stderr, "Fatal error: Incorrect compression method.\n");
                return -1;

            case -2:
                fprintf(stderr, "Fatal error: Premature end of file.\n");
                return -1;

            case -3:
                fprintf(stderr, "Fatal error: File read error.\n");
                return -1;

            case -4:
                fprintf(stderr, "Fatal error: Error allocating new image data.\n");
                return -1;

            case -5:
                fprintf(stderr, "Fatal error: CRC check failed, data may be corrupt.\n");
                return -1;

            default:
                ;
        }

        if(canExit) {
            break;
        }
    }

    return 1;
}


// -------------------- PNG Validity Functions -------------------- //



/* A valid PNG file must begin with the PNG signature, check that the first 8 bytes of the file
   match the signature. If the signature buffer is empty, has not been fully read, or does not
   match the PNG signature, return 0, indicating that there was an error processing the file. */
int ValidSig(FILE * file) {
    uint8_t buffer[8];
    size_t bytesRead = fread(buffer, 1, 8, file);

    // If the header is empty or the whole header has not been read, return 0
    if(buffer == NULL || bytesRead != 8) {
        return 0;
    }

    // If the header does not match the valid png header, return 0
    if(memcmp(buffer, (uint8_t[])PNG_SIG, 8) != 0) {
        return 0;
    }

    return 1;
}

/* */
int ValidChunk(FILE * file, struct Image * img) {
    uint8_t buffer4[4];     // Chunk length, type, crc buffer
    uint8_t *buffer;        // Chunk data buffer.
    int IEND_REACHED = 0;   // 1 if we have read the IEND chunk
    int n = 0;              // Number of bytes read after fread

    // Read and store chunk length (4 bytes)
    n = fread(buffer4, 1, 4, file);

    if(n != 4) {
        if(feof(file)) {
            return -2;  // Premature end of file reached
        } else {
            return -3;  // Error reading file.
        }
    }

    // Convert chunk length to an integer:
    uint32_t chunkLength = Hex2Int(buffer4, 4);

    // Read chunk type, if the chunk type is critical handle respectively.
    // - If a chunk is critical but the ancillary bit is incorrect, error.
    // - If a chunk is ancillary and but the ancillary bit is incorrect, ignore the chunk (it's unknown)
    // int criticalChunk = 0; // 0 if ancillary, 1 if critical

    // Read chunk type.
    n = fread(buffer4, 1, 4, file);

    // Save chunk type for CRC calculation
    uint8_t chunkType[4];
    memcpy(chunkType, buffer4, 4);

    if(n != 4) {
        if(feof(file)) {
            return -2;
        } else {
            return -3;
        }
    }

    // Initialize chunk data buffer and read chunk data.
    buffer = (uint8_t *)malloc(sizeof(uint8_t) * chunkLength);
    n = fread(buffer, 1, chunkLength, file);

    if(n != chunkLength) {
        if(feof(file)) {
            return -2;
        } else {
            return -3;
        }
    }

    if(memcmp(buffer4, (uint8_t[])PNG_IHDR, 4) == 0) {
        // For now only save the width and height of the image.
        memcpy(buffer4, buffer, sizeof(uint8_t) * 4);
        img->width = Hex2Int(buffer4, 4);
        
        memcpy(buffer4, buffer + 4, sizeof(uint8_t) * 4);
        img->height = Hex2Int(buffer4, 4);

        memcpy(buffer4, buffer + 8, sizeof(uint8_t) * 1);
        img->bitDepth = Hex2Int(buffer4, 1);

        memcpy(buffer4, buffer + 9, sizeof(uint8_t) * 1);
        img->colorType = Hex2Int(buffer4, 1);

        if(buffer[10] != 0) {
            return -1;  // Incorrect compression method.
        }
    } else if(memcmp(buffer4, (uint8_t[])PNG_IDAT, 4) == 0) {
        // First, create a new buffer newData of size dataSize + chunkLength, which holds the original image data.
        uint8_t *newData = realloc(img->dataRaw, img->dataRawSize + chunkLength);

        if(newData == NULL) {
            return -4;  // Error allocating new image data.
        }
        
        // Then, set img->dataRaw to this new buffer.
        img->dataRaw = newData;

        // Now, copy the new image data to the end of the original data in the array.
        memcpy(img->dataRaw + img->dataRawSize, buffer, chunkLength);

        // Update image size
        img->dataRawSize = img->dataRawSize + chunkLength;
    } else if(memcmp(buffer4, (uint8_t[])PNG_IEND, 4) == 0) {
        IEND_REACHED = 1;
    }

    // Now read the chunk CRC into buffer4
    n = fread(buffer4, 1, 4, file);
    if(n != 4) {
        if(feof(file)) {
            return -2;
        } else {
            return -3;
        }
    }

    uint32_t imgCRC = Hex2Int(buffer4, 4);

    // Now, perform the CRC check.
    // Initialize crc
    uint32_t crc = crc32(0L, Z_NULL, 0);
    
    // Run crc on chunk type
    crc = (uint32_t)crc32(crc, (Bytef *)chunkType, 4);

    // Run crc on chunk data;
    crc = (uint32_t)crc32(crc, (Bytef *)buffer, chunkLength);

    if(crc != imgCRC) {
        return -5;  // Invalid CRC, data corrupt
    }

    if(IEND_REACHED == 1) {
        return 0;   // IEND has been reached.
    }

    return 1;   // The chunk is valid
}



// -------------------- PNG Transformation Functions -------------------- //



/* Decompress the image data by initializing a z_stream and performing inflate decompresion.
   If any errors occur, return the respective error message. */
int Decompress(struct Image * img) {
    // First, we'll use inflate decompression on the image data.
    z_stream stream = {0};
    stream.avail_in = img->dataRawSize;
    stream.next_in = (Bytef *)img->dataRaw;
    stream.avail_out = img->dataDecompSize;
    stream.next_out = (Bytef *)img->dataDecomp;

    if(inflateInit(&stream) != Z_OK) {
        return -1;  // Error intializing decompression.
    }

    int ret;
    do {
        ret = inflate(&stream, Z_NO_FLUSH);

        if(ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR || ret == Z_BUF_ERROR) {
            inflateEnd(&stream);
            return -2;  // Error performing decompression
        }

    } while(ret != Z_STREAM_END);


    if(stream.total_out != img->dataDecompSize) {
        return -3;  // Error after decompression
        inflateEnd(&stream);
    }
    
    printf("DONE!\n");

    inflateEnd(&stream);
    return 1;
}

/* Reverse the filter on each scanline of the image, where length is the length of the scanline.
   First, read the first byte to get the filter type. Switch on the filter type to perform unfilter.
   Also, save the scanline as the previous scanline. If the first scanline is being read, treat
   the previous scanline as a line of zeroes. If the second byte of the scanline is being processed,
   treat the previous byte as 0. */
int Unfilter(struct Image * img) {
    // Loop by the scanline, which means we are looping from 0 to the height of the image.
    for(int row = 0; row < img->height; row++) {
        // First, get the filter type
        uint8_t filterType = img->dataDecomp[row * img->bpr];

        uint8_t *prev = NULL;      // The previous scanline
        if(row > 0) {
            prev = &img->dataDecomp[(row - 1) * img->bpr];
        }

        uint8_t *curRow = &img->dataDecomp[row * img->bpr];

        // Now loop by each byte, starting at the first (dataDecomp[1])
        for(int byte = 1; byte < img->bpr; byte++) {
            uint8_t x = curRow[byte]; // Current byte
            uint8_t a;
            uint8_t b;
            uint8_t c;

            /* If x is the first byte, a = 0
               If x is not the first byte, a = &x - 1 */
            if(byte <= img->bpp) {
                a = 0;
            } else {
                a = curRow[byte - img->bpp];
            }

            /* If the previous scanline is null (or this scanline is the first), b = 0
               If the previous scanline exists, b = byte 'above' x */
            if(prev == NULL) {
                b = 0;
            } else {
                b = prev[byte];
            }

            /* If previous scanline null or x is the first byte, c = 0
               If previous scaline exists and x is not the first byte, c = byte 'above and left' of x */
            if(prev == NULL || byte <= img->bpp) {
                c = 0;
            } else {
                c = prev[byte - img->bpp];
            }

            uint8_t recon = 0;
            switch(filterType) {
                case 0:     // NONE
                    recon = (uint8_t)x;
                    break;
                
                case 1:     // SUB: Recon(x) = Filt(x) + Recon(a)
                    recon = (uint8_t)(x + a);
                    break;

                case 2:     // UP: Recon(x) = Filt(x) + Recon(b)
                    recon = (uint8_t)(x + b);
                    break;

                case 3:     // AVERAGE: Filt(x) + floor((Recon (a) + Recon(b)) / 2)
                    recon = (uint8_t)(x + ((a + b) / 2));
                    break;

                case 4:     // PAETH: Recon(x) = Filt(x) + PaethPredictor(Recon(a), Recon(b), Recon(c))
                    recon = (uint8_t)(x + PaethPredictor(a, b, c));
                    break;

                default:
                    return -1;
            }

            curRow[byte] = recon;
        }
    }

    return 1;
}



// -------------------- Math and Helper Functions -------------------- //



// Convert hex array entries to an integer
uint32_t Hex2Int(uint8_t * buffer, int length) {
    uint32_t result = 0;
    
    for(int i = 0; i < length; i++) {
        result = (result << 8) | buffer[i];
    }

    return result;
}

/* Calculates the Paeth Predictor given values a, b, and c. */
uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c) {
    uint8_t p = a + b - c;
    uint8_t pr;
    
    uint8_t pa = p - a;
    if(pa < 0) {
        pa = -pa;
    }

    uint8_t pb = p - b;
    if(pb < 0) {
        pb = -pb;
    }

    uint8_t pc = p - c;
    if(pc < 0) {
        pc = -pc;
    }

    if(pa <= pb && pa <= pc) {
        pr = a;
    } else if(pb <= pc) {
        pr = b;
    } else {
        pr = c;
    }

    return pr;
}