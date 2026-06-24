#ifndef IMAGEHANDLER_H
#define IMAGEHANDLER_H

#include <iostream>
#include <iomanip>
#include <fstream>
using namespace std;

class Image {
public:
    int width;
    int height;
    int** pixels;

    Image(int w, int h) {
        width = w;
        height = h;
        pixels = new int*[height];
        for (int i = 0; i < height; i++)
            pixels[i] = new int[width];
    }

    ~Image() {
        for (int i = 0; i < height; i++)
            delete[] pixels[i];
        delete[] pixels;
    }

    int getPixel(int x, int y) {
        if (x >= 0 && x < width && y >= 0 && y < height)
            return pixels[y][x];
        return 0;
    }

    void setPixel(int x, int y, int value) {
        if (x >= 0 && x < width && y >= 0 && y < height)
            pixels[y][x] = value;
    }

    void display() {
        cout << "\nImage " << width << "x" << height << ":\n";
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int v = pixels[y][x];
                if      (v == 0)  cout << "  ";
                else if (v < 128) cout << "░░";
                else if (v < 200) cout << "▒▒";
                else              cout << "██";
            }
            cout << "\n";
        }
    }

    int getTotalPixels() { return width * height; }
};

// Portable file size reader — works on Windows, Linux, macOS
static long getFileSizeBytes(const char* filename) {
    ifstream f(filename, ios::binary | ios::ate);
    if (!f.is_open()) return -1;
    return (long)f.tellg();
}

struct CompressionResult {
    int    originalSize;
    int    compressedSize;
    double compressionRatio;
    double executionTime;

    void display(const char* algorithmName) {
        // Save stream state so our formatting doesn't leak to callers
        ios_base::fmtflags oldFlags = cout.flags();
        streamsize oldPrecision     = cout.precision();

        cout << fixed << setprecision(2);
        cout << "\n========================================\n";
        cout << "Algorithm: " << algorithmName          << "\n";
        cout << "========================================\n";
        cout << "Original Size:      " << originalSize   << " bytes\n";
        cout << "Compressed Size:    " << compressedSize << " bytes\n";
        cout << "Compression Ratio:  " << compressionRatio << "%\n";
        cout << "Execution Time:     " << executionTime    << " ms\n";
        cout << "========================================\n";

        cout.flags(oldFlags);
        cout.precision(oldPrecision);
    }
};

#endif