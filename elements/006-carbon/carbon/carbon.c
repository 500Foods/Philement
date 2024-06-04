// carbon.c
// 
// Part of the Philements project
// 
// Usage:
// carbon <image1> <image2> [precision] [tolerance] [profile]
//
// This will compare image1 to image2 and make an assessment as to
// whether a print error has occurred. The precision paramater is
// used to determine how many subdivisions to compare. The images
// are then divided into this many equal squares across, with as 
// many squares vertically as needed to fit the aspect ratio. The 
// tolerance is used as a factor in determining how similar the
// individual blocks are when compared. If images are considered
// a match, the assessment is 'pass'. If the assessment is 'fail'
// an image is output showing the blocks that contribted. The 
// profile is a text file describing the parts of the image that
// are assigned as excluded, print area or non-print area. The
// non-print area is scrutinized most, and excluded is ignored.
//
// Created By: Andrew Simard
// Created On: 2024-Jun-03
// Created At: https://


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <jpeglib.h>
#include <png.h>
#include <math.h>
#include <limits.h>
#include <stdbool.h>


#define DEFAULT_PRECISION 100
#define DEFAULT_TOLERANCE 10


typedef struct {
    unsigned char *data;
    int width;
    int height;
    int channels;
} Image;

typedef struct {
    int width;
    int height;
    char type[4];
} ImageInfo;

typedef struct {
    unsigned char r, g, b;
} RGBColor;

typedef struct {
    char color;
    char intensity[4];
} ColorAndIntensity;

typedef struct {
    char *grid;
    int width;
    int height;
} Profile;

typedef struct {
    int *scores;
    int width;
    int height;
} ComparisonGrid;


int max(int a, int b); 
int min(int a, int b); 
Image allocateImage(int width, int height, int channels);
void freeImage(Image image);
ImageInfo getJPGImageInfo(const char *filename);
ImageInfo getPNGImageInfo(const char *filename);
ImageInfo getImageInfo(const char *filename);
Image loadJPGImage(const char *filename);
Image loadPNGImage(const char *filename);
RGBColor *divideIntoBlocks(Image image, int precision);
void printBlocks(RGBColor *blocks, int numHorizontalBlocks, int numVerticalBlocks, Profile profile); 
ColorAndIntensity getColorAndIntensity(unsigned char r, unsigned char g, unsigned char b); 
Profile loadProfile(const char *filename, int width, int height);
void freeProfile(Profile profile);
ComparisonGrid allocateComparisonGrid(int width, int height);
void freeComparisonGrid(ComparisonGrid grid);
ComparisonGrid compareBlocks(RGBColor *blocks1, RGBColor *blocks2, int width, int height, Profile profile, int tolerance);
void printComparisonGrid(ComparisonGrid grid);
bool summarizeComparison(ComparisonGrid grid);
Profile createDefaultProfile(int width, int height); 


int max(int a, int b) {
    return (a > b) ? a : b;
}


int min(int a, int b) {
    return (a < b) ? a : b;
}


Image allocateImage(int width, int height, int channels) {
    Image image;
    image.width = width;
    image.height = height;
    image.channels = channels;
    image.data = (unsigned char *)malloc(width * height * channels * sizeof(unsigned char));
    return image;
}


void freeImage(Image image) {
    free(image.data);
}


ImageInfo getJPGImageInfo(const char *filename) {
    ImageInfo info = {0};
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error: Failed to open JPG file '%s'\n", filename);
        return info;
    }

    unsigned short signature;
    if (fread(&signature, 1, 2, file) != 2) {
        printf("Error: Failed to read JPG signature from file '%s'\n", filename);
        fclose(file);
        return info;
    }

    signature = ((signature & 0xFF) << 8) | ((signature >> 8) & 0xFF);

    if (signature != 0xFFD8) {
        printf("Error: Invalid JPG signature '0x%04X' in file '%s'\n", signature, filename);
        fclose(file);
        return info;
    }

    strcpy(info.type, "JPG");

    unsigned char segment_marker;
    unsigned int segment_length;
    unsigned char length_bytes[2];

    while (fread(&segment_marker, 1, 1, file) == 1) {
        if (segment_marker != 0xFF) {
            printf("Error: Invalid marker '0x%02X' encountered while searching for SOF segment in JPG file '%s'\n", segment_marker, filename);
            break;
        }

        if (fread(&segment_marker, 1, 1, file) != 1) {
            printf("Error: Failed to read segment marker while searching for SOF segment in JPG file '%s'\n", filename);
            break;
        }

        if (segment_marker == 0xC0 || (segment_marker >= 0xC1 && segment_marker <= 0xCF && segment_marker != 0xC4 && segment_marker != 0xC8)) {
            if (fread(length_bytes, 1, 2, file) != 2) {
                printf("Error: Failed to read segment length bytes for SOF segment in JPG file '%s'\n", filename);
                break;
            }
            segment_length = (length_bytes[0] << 8) + length_bytes[1];

            unsigned char precision;
            if (fread(&precision, 1, 1, file) != 1) {
                printf("Error: Failed to read precision byte for SOF segment in JPG file '%s'\n", filename);
                break;
            }

            unsigned char height_bytes[2];
            if (fread(height_bytes, 1, 2, file) != 2) {
                printf("Error: Failed to read height bytes for SOF segment in JPG file '%s'\n", filename);
                break;
            }
            info.height = (height_bytes[0] << 8) + height_bytes[1];

            unsigned char width_bytes[2];
            if (fread(width_bytes, 1, 2, file) != 2) {
                printf("Error: Failed to read width bytes for SOF segment in JPG file '%s'\n", filename);
                break;
            }
            info.width = (width_bytes[0] << 8) + width_bytes[1];

            break;
        } else {
            if (fread(length_bytes, 1, 2, file) != 2) {
                printf("Error: Failed to read segment length bytes while searching for SOF segment in JPG file '%s'\n", filename);
                break;
            }
            segment_length = (length_bytes[0] << 8) + length_bytes[1];
            if (fseek(file, segment_length - 2, SEEK_CUR) != 0) {
                printf("Error: Failed to skip to the next segment while searching for SOF segment in JPG file '%s'\n", filename);
                break;
            }
        }
    }

    fclose(file);
    return info;
}


ImageInfo getPNGImageInfo(const char *filename) {
    ImageInfo info = {0};
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error: Failed to open PNG file '%s'\n", filename);
        return info;
    }

    unsigned char signature[8];
    if (fread(signature, 1, 8, file) != 8) {
        printf("Error: Failed to read PNG signature from file '%s'\n", filename);
        fclose(file);
        return info;
    }

    if (signature[0] != 0x89 || signature[1] != 0x50 || signature[2] != 0x4E || signature[3] != 0x47 ||
        signature[4] != 0x0D || signature[5] != 0x0A || signature[6] != 0x1A || signature[7] != 0x0A) {
        printf("Error: Invalid PNG signature in file '%s'\n", filename);
        fclose(file);
        return info;
    }

    strcpy(info.type, "PNG");

    unsigned char ihdr[25];
    if (fread(ihdr, 1, 25, file) != 25) {
        printf("Error: Failed to read IHDR chunk from PNG file '%s'\n", filename);
        fclose(file);
        return info;
    }

    if (ihdr[4] != 'I' || ihdr[5] != 'H' || ihdr[6] != 'D' || ihdr[7] != 'R') {
        printf("Error: Missing IHDR chunk in PNG file '%s'\n", filename);
        fclose(file);
        return info;
    }

    info.width = (ihdr[8] << 24) | (ihdr[9] << 16) | (ihdr[10] << 8) | ihdr[11];
    info.height = (ihdr[12] << 24) | (ihdr[13] << 16) | (ihdr[14] << 8) | ihdr[15];

    fclose(file);
    return info;
}


ImageInfo getImageInfo(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error: Failed to open file '%s'\n", filename);
        return (ImageInfo){0};
    }

    unsigned char signature[8];
    if (fread(signature, 1, 8, file) != 8) {
        printf("Error: Failed to read file signature from file '%s'\n", filename);
        fclose(file);
        return (ImageInfo){0};
    }

    fclose(file);

    ImageInfo info;

    if (signature[0] == 0xFF && signature[1] == 0xD8) {
        info = getJPGImageInfo(filename);
    } else if (signature[0] == 0x89 && signature[1] == 0x50 && signature[2] == 0x4E && signature[3] == 0x47) {
        info = getPNGImageInfo(filename);
    } else {
        printf("Error: Unsupported image format in file '%s'\n", filename);
        return (ImageInfo){0};
    }

    return info;
}


Image loadJPGImage(const char *filename) {
    Image image = {0};
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error: Failed to open JPG file '%s'\n", filename);
        return image;
    }

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, file);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    image.width = cinfo.output_width;
    image.height = cinfo.output_height;
    image.channels = cinfo.output_components;
    image.data = (unsigned char *)malloc(image.width * image.height * image.channels);

    JSAMPROW row_pointer[1];
    while (cinfo.output_scanline < cinfo.output_height) {
        row_pointer[0] = &image.data[cinfo.output_scanline * image.width * image.channels];
        jpeg_read_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(file);

    return image;
}


Image loadPNGImage(const char *filename) {
    Image image = {0};
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error: Failed to open PNG file '%s'\n", filename);
        return image;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        printf("Error: Failed to create PNG read struct\n");
        fclose(file);
        return image;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        printf("Error: Failed to create PNG info struct\n");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(file);
        return image;
    }

    if (setjmp(png_jmpbuf(png))) {
        printf("Error: Failed to set jump buffer for PNG\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(file);
        return image;
    }

    png_init_io(png, file);
    png_read_info(png, info);

    image.width = png_get_image_width(png, info);
    image.height = png_get_image_height(png, info);
    image.channels = png_get_channels(png, info);

    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * image.height);
    for (int y = 0; y < image.height; y++) {
        row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
    }

    png_read_image(png, row_pointers);

    image.data = (unsigned char *)malloc(image.width * image.height * image.channels);
    for (int y = 0; y < image.height; y++) {
        memcpy(&image.data[y * image.width * image.channels], row_pointers[y], png_get_rowbytes(png, info));
        free(row_pointers[y]);
    }
    free(row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(file);

    return image;
}


RGBColor *divideIntoBlocks(Image image, int precision) {
    int blockSize = (image.width + precision - 1) / precision;
    int numHorizontalBlocks = precision;
    int numVerticalBlocks = (image.height + blockSize - 1) / blockSize;
    int numBlocks = numHorizontalBlocks * numVerticalBlocks;

    printf("Block size: %d x %d\n", blockSize, blockSize);
    printf("Block array: %d x %d\n", numHorizontalBlocks, numVerticalBlocks);

    RGBColor *blocks = (RGBColor *)malloc(numBlocks * sizeof(RGBColor));
    if (blocks == NULL) {
        printf("Error: Failed to allocate memory for blocks\n");
        return NULL;
    }

    int blockIndex = 0;
    for (int y = 0; y < image.height; y += blockSize) {
        for (int x = 0; x < image.width; x += blockSize) {
            long long sumR = 0, sumG = 0, sumB = 0;
            int count = 0;

            for (int j = y; j < y + blockSize && j < image.height; j++) {
                for (int i = x; i < x + blockSize && i < image.width; i++) {
                    int index = (j * image.width + i) * image.channels;
                    if (index < 0 || index >= image.width * image.height * image.channels) {
                        printf("Error: Invalid pixel index %d (width=%d, height=%d, channels=%d)\n", index, image.width, image.height, image.channels);
                        free(blocks);
                        return NULL;
                    }
                    sumR += image.data[index];
                    sumG += image.data[index + 1];
                    sumB += image.data[index + 2];
                    count++;
                }
            }

            if (count == 0) {
                printf("Error: Zero pixels in block (%d, %d)\n", x, y);
                free(blocks);
                return NULL;
            }

            if (sumR > LLONG_MAX / count || sumG > LLONG_MAX / count || sumB > LLONG_MAX / count) {
                printf("Error: Overflow in block (%d, %d)\n", x, y);
                free(blocks);
                return NULL;
            }

            blocks[blockIndex].r = (unsigned char)(sumR / count);
            blocks[blockIndex].g = (unsigned char)(sumG / count);
            blocks[blockIndex].b = (unsigned char)(sumB / count);
            blockIndex++;
        }
    }

    return blocks;
}


void printBlocks(RGBColor *blocks, int numHorizontalBlocks, int numVerticalBlocks, Profile profile) {
    for (int i = 0; i < numVerticalBlocks; i++) {
        for (int j = 0; j < numHorizontalBlocks; j++) {
            int index = i * numHorizontalBlocks + j;
            char profileChar = profile.grid ? profile.grid[index] : 'P';  // Default to 'P' if no profile

            if (profileChar == 'X') {
                printf(" ");  // Print two spaces for ignored blocks
            } else {
                unsigned char r = blocks[index].r;
                unsigned char g = blocks[index].g;
                unsigned char b = blocks[index].b;

                ColorAndIntensity ci = getColorAndIntensity(r, g, b);
                printf("\033[1;%dm%s\033[0m", 30 + ci.color, ci.intensity);
            }
        }
        printf("\n");
    }
}


ColorAndIntensity getColorAndIntensity(unsigned char r, unsigned char g, unsigned char b) {
    ColorAndIntensity ci;

    // Determine the color based on the dominant RGB values
    int threshold = 32;
    if (abs(r - g) < threshold && abs(g - b) < threshold && abs(b - r) < threshold) {
        int brightness = (r + g + b) / 3;
        if (brightness < 64)
            ci.color = 'K';  // Black
        else if (brightness > 192)
            ci.color = 'W';  // White
        else
            ci.color = 'G';  // Gray
    } else {
        int maxValue = max(max(r, g), b);
        int minValue = min(min(r, g), b);
        int delta = maxValue - minValue;

        if (delta < threshold) {
            ci.color = 'G';  // Gray
        } else if (r == maxValue) {
            if (g == minValue)
                ci.color = 'R';  // Red
            else
                ci.color = 'Y';  // Yellow (Red + Green)
        } else if (g == maxValue) {
            if (b == minValue)
                ci.color = 'G';  // Green
            else
                ci.color = 'C';  // Cyan (Green + Blue)
        } else {  // b == maxValue
            if (r == minValue)
                ci.color = 'B';  // Blue
            else
                ci.color = 'M';  // Magenta (Red + Blue)
        }
    }

    // Determine the intensity based on the average RGB value
    int intensity = (r + g + b) / 3;
    if (intensity < 64)
        strcpy(ci.intensity, " ");  // ASCII 32 (Space)
    else if (intensity < 128)
        strcpy(ci.intensity, "\xE2\x96\x91");  // UTF-8 encoding for Light Shade (Unicode U+2591)
    else if (intensity < 192)
        strcpy(ci.intensity, "\xE2\x96\x92");  // UTF-8 encoding for Medium Shade (Unicode U+2592)
    else if (intensity < 224)
        strcpy(ci.intensity, "\xE2\x96\x93");  // UTF-8 encoding for Dark Shade (Unicode U+2593)
    else
        strcpy(ci.intensity, "\xE2\x96\x88");  // UTF-8 encoding for Full Block (Unicode U+2588)

    return ci;
}


Profile loadProfile(const char *filename, int width, int height) {
    Profile profile = createDefaultProfile(width, height);
    if (profile.grid == NULL) return profile;

    if (filename == NULL) {
        printf("Info: Using default profile (all 'P' blocks)\n");
        return profile;
    }

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Warning: Failed to open profile file '%s', using default profile\n", filename);
        return profile;
    }

    int count = 0;
    char c;
    while (fscanf(file, " %c", &c) == 1) {
        if (c != 'X' && c != 'P' && c != 'N') {
            printf("Warning: Invalid character '%c' in profile file, using default profile\n", c);
            fclose(file);
            freeProfile(profile);
            return createDefaultProfile(width, height);
        }
        if (count < width * height) {
            profile.grid[count] = c;
        }
        count++;
    }

    fclose(file);

    if (count != width * height) {
        printf("Warning: Profile file has %d characters, expected %d; using default profile\n", count, width * height);
        freeProfile(profile);
        return createDefaultProfile(width, height);
    }

    return profile;
}


void freeProfile(Profile profile) {
    free(profile.grid);
}


ComparisonGrid allocateComparisonGrid(int width, int height) {
    ComparisonGrid grid = {NULL, width, height};
    grid.scores = (int *)malloc(width * height * sizeof(int));
    if (grid.scores == NULL) {
        printf("Error: Failed to allocate memory for comparison grid\n");
    }
    return grid;
}


void freeComparisonGrid(ComparisonGrid grid) {
    free(grid.scores);
}


ComparisonGrid compareBlocks(RGBColor *blocks1, RGBColor *blocks2, int width, int height, Profile profile, int tolerance) {
    ComparisonGrid grid = allocateComparisonGrid(width, height);
    if (grid.scores == NULL) return grid;

    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int index = i * width + j;
            char profileChar = profile.grid ? profile.grid[index] : 'P';  // Default to 'P' if no profile

            if (profileChar == 'X') {
                grid.scores[index] = 'X';  // Use 'X' character instead of 0
            } else {
                int dr = blocks1[index].r - blocks2[index].r;
                int dg = blocks1[index].g - blocks2[index].g;
                int db = blocks1[index].b - blocks2[index].b;
                double distance = sqrt(dr * dr + dg * dg + db * db);

                int score;
                if (profileChar == 'P') {
                    score = (int)(distance / (2.0 * tolerance) + 0.5);  // Less sensitive for 'P' blocks
                } else {  // 'N'
                    score = (int)(distance / tolerance + 0.5);  // More sensitive for 'N' blocks
                }

                grid.scores[index] = min(score, 9) + '0';  // Convert to character '0'-'9'
            }
        }
    }

    return grid;
}


void printComparisonGrid(ComparisonGrid grid) {
    for (int i = 0; i < grid.height; i++) {
        for (int j = 0; j < grid.width; j++) {
            int index = i * grid.width + j;
            char score = grid.scores[index];

            if (score == 'X') {
                printf(" "); 
            } else {
                //printf("\033[1;%dm%c\033[0m ", 30 + ((score > '0') ? 31 : 37), score);  // Red for non-zero, white for zero
                printf("%c", score); 
            }
        }
        printf("\n");
    }
}


bool summarizeComparison(ComparisonGrid grid) {
    int counts[11] = {0};  // 0-9 for scores, 10 for 'X'
    bool shouldFail = false;

    for (int i = 0; i < grid.height; i++) {
        for (int j = 0; j < grid.width; j++) {
            int index = i * grid.width + j;
            char score = grid.scores[index];
            if (score == 'X')
                counts[10]++;
            else
                counts[score - '0']++;
        }
    }

    printf("Summary:\n");
    for (int i = 0; i < 10; i++) {
        if (counts[i] > 0) {
            printf("- %d-blocks: %d\n", i, counts[i]);
        }
    }
    if (counts[10] > 0) {
        printf("- X-blocks: %d\n", counts[10]);
    }

    // Apply the failure criteria
    if (counts[9] > 0) {
        printf("Reason: More than 0 '9' blocks\n");
        shouldFail = true;
    }
    if (counts[8] > 10) {
        printf("Reason: More than 10 '8' blocks\n");
        shouldFail = true;
    }
    if (counts[7] > 50) {
        printf("Reason: More than 50 '7' blocks\n");
        shouldFail = true;
    }

    printf("Assessment: %s\n", shouldFail ? "Fail" : "Pass");
    return shouldFail;
}


Profile createDefaultProfile(int width, int height) {
    Profile profile = {NULL, width, height};
    profile.grid = (char *)malloc(width * height * sizeof(char));
    if (profile.grid == NULL) {
        printf("Error: Failed to allocate memory for default profile grid\n");
        return profile;
    }

    memset(profile.grid, 'P', width * height);
    return profile;
}


void overlayColor(Image *image, int x, int y, int width, int height, unsigned char r, unsigned char g, unsigned char b, double alpha) {
    for (int j = y; j < y + height && j < image->height; j++) {
        for (int i = x; i < x + width && i < image->width; i++) {
            int index = (j * image->width + i) * image->channels;
            image->data[index]     = (unsigned char)(alpha * r + (1 - alpha) * image->data[index]);
            image->data[index + 1] = (unsigned char)(alpha * g + (1 - alpha) * image->data[index + 1]);
            image->data[index + 2] = (unsigned char)(alpha * b + (1 - alpha) * image->data[index + 2]);
        }
    }
}


void highlightDifferences(Image *image, ComparisonGrid grid, int blockSize) {
    for (int i = 0; i < grid.height; i++) {
        for (int j = 0; j < grid.width; j++) {
            int index = i * grid.width + j;
            char score = grid.scores[index];

            int x = j * blockSize;
            int y = i * blockSize;

            if (score == 'X') {
                overlayColor(image, x, y, blockSize, blockSize, 0, 0, 0, 0.5);  // Black overlay with 50% opacity
            } else if (score >= '7') {
                overlayColor(image, x, y, blockSize, blockSize, 255, 0, 0, 0.7);  // Red overlay with 70% opacity
            }
        }
    }
}


char *generateFailureFilename(const char *originalFilename) {
    const char *extension = strrchr(originalFilename, '.');
    if (extension == NULL) {
        printf("Error: No file extension found in '%s'\n", originalFilename);
        return NULL;
    }

    size_t baseLength = extension - originalFilename;
    size_t newLength = baseLength + strlen("-failure") + strlen(extension) + 1;
    char *newFilename = (char *)malloc(newLength);
    if (newFilename == NULL) {
        printf("Error: Failed to allocate memory for failure filename\n");
        return NULL;
    }

    strncpy(newFilename, originalFilename, baseLength);
    strcpy(newFilename + baseLength, "-failure");
    strcpy(newFilename + baseLength + strlen("-failure"), extension);

    return newFilename;
}


void saveImage(Image image, const char *filename) {
    const char *extension = strrchr(filename, '.');
    if (extension == NULL) {
        printf("Error: No file extension found in '%s'\n", filename);
        return;
    }

    if (strcmp(extension, ".jpg") == 0 || strcmp(extension, ".jpeg") == 0) {
        struct jpeg_compress_struct cinfo;
        struct jpeg_error_mgr jerr;

        FILE *file = fopen(filename, "wb");
        if (file == NULL) {
            printf("Error: Failed to open file '%s' for writing\n", filename);
            return;
        }

        cinfo.err = jpeg_std_error(&jerr);
        jpeg_create_compress(&cinfo);
        jpeg_stdio_dest(&cinfo, file);

        cinfo.image_width = image.width;
        cinfo.image_height = image.height;
        cinfo.input_components = image.channels;
        cinfo.in_color_space = JCS_RGB;

        jpeg_set_defaults(&cinfo);
        jpeg_set_quality(&cinfo, 90, TRUE);
        jpeg_start_compress(&cinfo, TRUE);

        JSAMPROW row_pointer[1];
        while (cinfo.next_scanline < cinfo.image_height) {
            row_pointer[0] = &image.data[cinfo.next_scanline * image.width * image.channels];
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        fclose(file);
    } else if (strcmp(extension, ".png") == 0) {
        FILE *file = fopen(filename, "wb");
        if (file == NULL) {
            printf("Error: Failed to open file '%s' for writing\n", filename);
            return;
        }

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info = png_create_info_struct(png);
        png_init_io(png, file);
        png_set_IHDR(png, info, image.width, image.height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);

        png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * image.height);
        for (int i = 0; i < image.height; i++) {
            row_pointers[i] = &image.data[i * image.width * image.channels];
        }

        png_write_image(png, row_pointers);
        png_write_end(png, NULL);

        free(row_pointers);
        fclose(file);
    } else {
        printf("Error: Unsupported file format '%s'\n", extension);
    }
}


int main(int argc, char *argv[]) {
    clock_t start, end;
    double cpu_time_used;

    start = clock();

    if (argc < 4) {
        printf("Insufficient parameters.\nUsage: %s <image1> <image2> <precision> [tolerance] [profile]\n", argv[0]);
        return 1;
    }

    char *image1 = argv[1];
    char *image2 = argv[2];
    int precision = atoi(argv[3]);
    int tolerance = (argc > 4 && atoi(argv[4]) > 0) ? atoi(argv[4]) : 10;
    char *profileFile = (argc > 5) ? argv[5] : NULL;

    printf("Input Parameters:\n");
    printf("- Image 1: %s\n", image1);
    printf("- Image 2: %s\n", image2);
    printf("- Precision: %d\n", precision);
    printf("- Tolerance: %d %s\n", tolerance, (argc > 4) ? "" : "[Default]");
    printf("- Profile: %s\n", profileFile ? profileFile : "Default (all 'P' blocks)");

    ImageInfo image1Info = getImageInfo(image1);
    ImageInfo image2Info = getImageInfo(image2);

    if (image1Info.width == 0 || image1Info.height == 0) {
        printf("Error: Invalid image file '%s'\n", image1);
        return 1;
    }

    if (image2Info.width == 0 || image2Info.height == 0) {
        printf("Error: Invalid image file '%s'\n", image2);
        return 1;
    }

    printf("Image 1: %s (%d x %d)\n", image1Info.type, image1Info.width, image1Info.height);
    printf("Image 2: %s (%d x %d)\n", image2Info.type, image2Info.width, image2Info.height);

    Image loadedImage1;
    Image loadedImage2;

    if (strcmp(image1Info.type, "JPG") == 0) {
        loadedImage1 = loadJPGImage(image1);
    } else if (strcmp(image1Info.type, "PNG") == 0) {
        loadedImage1 = loadPNGImage(image1);
    }

    if (strcmp(image2Info.type, "JPG") == 0) {
        loadedImage2 = loadJPGImage(image2);
    } else if (strcmp(image2Info.type, "PNG") == 0) {
        loadedImage2 = loadPNGImage(image2);
    }
	
    int blockSize = (loadedImage1.width + precision - 1) / precision;
    int numHorizontalBlocks = precision;
    int numVerticalBlocks = (loadedImage1.height + blockSize - 1) / blockSize;

    RGBColor *image1Blocks = divideIntoBlocks(loadedImage1, precision);
    RGBColor *image2Blocks = divideIntoBlocks(loadedImage2, precision);


    Profile profile = loadProfile(profileFile, numHorizontalBlocks, numVerticalBlocks);
    if (profile.grid == NULL) {
        printf("Error: Failed to load profile\n");
        free(image1Blocks);
        free(image2Blocks);
        return 1;
    }

    printf("Image 1:\n");
    printBlocks(image1Blocks, numHorizontalBlocks, numVerticalBlocks, profile);

    printf("\nImage 2:\n");
    printBlocks(image2Blocks, numHorizontalBlocks, numVerticalBlocks, profile);
   
    ComparisonGrid comparison = compareBlocks(image1Blocks, image2Blocks, numHorizontalBlocks, numVerticalBlocks, profile, tolerance);
    if (comparison.scores == NULL) {
        printf("Error: Failed to allocate comparison grid\n");
        free(image1Blocks);
        free(image2Blocks);
        freeProfile(profile);
        freeImage(loadedImage1);
        freeImage(loadedImage2);
        return 1;
    }

    printf("\nComparison Grid:\n");
    printComparisonGrid(comparison);

    printf("\n");
    bool shouldAbort = summarizeComparison(comparison);

    freeImage(loadedImage1);

    if (shouldAbort) {
        printf("WARNING: Print failure detected! Consider aborting the print job.\n");

        char *failureFilename = generateFailureFilename(image2);
        if (failureFilename != NULL) {
            highlightDifferences(&loadedImage2, comparison, blockSize);
            saveImage(loadedImage2, failureFilename);
            printf("Failure Image: %s\n", failureFilename);
            free(failureFilename);
        }
    } else {
        printf("Print job appears to be progressing normally.\n");
    }

    free(image1Blocks);
    free(image2Blocks);
    freeProfile(profile);
    freeComparisonGrid(comparison);
    freeImage(loadedImage2);

    end = clock();
    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("Execution Time: %.2f ms\n", cpu_time_used);

    return 0;
}

