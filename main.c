#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offset;
} BMPHeader;

typedef struct {
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bits;
    uint32_t compression;
    uint32_t imagesize;
    int32_t xresolution;
    int32_t yresolution;
    uint32_t ncolours;
    uint32_t importantcolours;
} BMPInfoHeader;
#pragma pack(pop)

typedef struct {
    int width;
    int height;
    uint8_t* data;
} Image;

Image loadBMP(const char* filename) {
    Image img = {0};
    FILE* file = fopen(filename, "rb");
    if (!file) {
        fprintf(stderr, "Unable to open file %s\n", filename);
        return img;
    }

    BMPHeader header;
    BMPInfoHeader infoHeader;

    fread(&header, sizeof(BMPHeader), 1, file);
    fread(&infoHeader, sizeof(BMPInfoHeader), 1, file);

    if (header.type != 0x4D42) { // "BM" in little endian
        fprintf(stderr, "Not a BMP file\n");
        fclose(file);
        return img;
    }

    if (infoHeader.bits != 24) {
        fprintf(stderr, "Only 24-bit BMPs are supported\n");
        fclose(file);
        return img;
    }

    img.width = infoHeader.width;
    img.height = abs(infoHeader.height); // Handle top-down BMPs
    int rowSize = (img.width * 3 + 3) & ~3; // Row size is padded to multiple of 4 bytes
    img.data = (uint8_t*)malloc(rowSize * img.height);

    fseek(file, header.offset, SEEK_SET);
    fread(img.data, rowSize, img.height, file);

    fclose(file);
    return img;
}

void saveBMP(const char* filename, Image img) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Unable to create file %s\n", filename);
        return;
    }

    int rowSize = (img.width * 3 + 3) & ~3;
    int fileSize = 54 + rowSize * img.height;

    BMPHeader header = {0x4D42, fileSize, 0, 0, 54};
    BMPInfoHeader infoHeader = {40, img.width, img.height, 1, 24, 0, rowSize * img.height, 0, 0, 0, 0};

    fwrite(&header, sizeof(BMPHeader), 1, file);
    fwrite(&infoHeader, sizeof(BMPInfoHeader), 1, file);
    fwrite(img.data, rowSize, img.height, file);

    fclose(file);
}

void averagePixel(Image img, int x, int y, uint8_t* pixel) {
    int sum[3] = {0};
    int count = 0;

    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int newX = x + i;
            int newY = y + j;
            if (newX >= 0 && newX < img.width && newY >= 0 && newY < img.height) {
                int index = (newY * img.width + newX) * 3;
                for (int c = 0; c < 3; c++) {
                    sum[c] += img.data[index + c];
                }
                count++;
            }
        }
    }

    for (int c = 0; c < 3; c++) {
        pixel[c] = (uint8_t)(sum[c] / count);
    }
}

Image applyFilter(Image input) {
    Image output = {input.width, input.height, malloc(input.width * input.height * 3)};

    for (int y = 0; y < input.height; y++) {
        for (int x = 0; x < input.width; x++) {
            uint8_t* pixel = &output.data[(y * input.width + x) * 3];
            averagePixel(input, x, y, pixel);
        }
    }

    return output;
}

int main() {
    const char* inputFile = "socks.bmp";
    const char* outputFile = "output.bmp";

    Image input = loadBMP(inputFile);
    if (!input.data) {
        fprintf(stderr, "Failed to load image\n");
        return 1;
    }

    Image output = applyFilter(input);
    saveBMP(inputFile, input);
    saveBMP(outputFile, output);

    free(input.data);
    free(output.data);

    printf("Image processed and saved as %s\n", outputFile);

    return 0;
}
