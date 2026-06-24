#ifndef SEAMCARVING_H
#define SEAMCARVING_H

#include "ImageHandler.h"
#include <ctime>
#include <cmath>

class SeamCarvingCompressor {
private:
    int getEnergy(Image* img, int x, int y) {
        // Calculate the "energy" (importance) of a pixel based on contrast with neighbors
        int left = (x == 0) ? img->getPixel(x, y) : img->getPixel(x - 1, y);
        int right = (x == img->width - 1) ? img->getPixel(x, y) : img->getPixel(x + 1, y);
        int up = (y == 0) ? img->getPixel(x, y) : img->getPixel(x, y - 1);
        int down = (y == img->height - 1) ? img->getPixel(x, y) : img->getPixel(x, y + 1);
        
        return abs(left - right) + abs(up - down);
    }

    int* findOptimalSeam(Image* img) {
        int w = img->width;
        int h = img->height;
        
        // DP Table to store minimum cumulative energy
        int** dp = new int*[h];
        for (int i = 0; i < h; i++) dp[i] = new int[w];

        // Initialize top row with base energy
        for (int x = 0; x < w; x++) {
            dp[0][x] = getEnergy(img, x, 0);
        }

        // STEP 1: DYNAMIC PROGRAMMING (Fill the table top to bottom)
        for (int y = 1; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int energy = getEnergy(img, x, y);
                
                int leftParent = (x == 0) ? 999999 : dp[y - 1][x - 1];
                int midParent  = dp[y - 1][x];
                int rightParent = (x == w - 1) ? 999999 : dp[y - 1][x + 1];

               
                dp[y][x] = energy + min(midParent, min(leftParent, rightParent));
            }
        }

        // STEP 2: BACKTRACKING (Find the lowest energy path from bottom to top)
        int* seam = new int[h];
        
        // Find minimum at the bottom row
        int minEnergy = dp[h - 1][0];
        int minX = 0;
        for (int x = 1; x < w; x++) {
            if (dp[h - 1][x] < minEnergy) {
                minEnergy = dp[h - 1][x];
                minX = x;
            }
        }
        
        seam[h - 1] = minX;

        // Trace back up the table
        for (int y = h - 1; y > 0; y--) {
            int x = seam[y];
            int leftParent = (x == 0) ? 999999 : dp[y - 1][x - 1];
            int midParent  = dp[y - 1][x];
            int rightParent = (x == w - 1) ? 999999 : dp[y - 1][x + 1];

            if (midParent <= leftParent && midParent <= rightParent) {
                seam[y - 1] = x;
            } else if (leftParent <= midParent && leftParent <= rightParent) {
                seam[y - 1] = x - 1;
            } else {
                seam[y - 1] = x + 1;
            }
        }

        // Clean up heap memory
        for (int i = 0; i < h; i++) delete[] dp[i];
        delete[] dp;

        return seam;
    }

    Image* removeSeam(Image* img, int* seam) {
        Image* newImg = new Image(img->width - 1, img->height);
        
        for (int y = 0; y < img->height; y++) {
            int targetX = seam[y];
            int newX = 0;
            for (int x = 0; x < img->width; x++) {
                if (x != targetX) { // Skip the deleted pixel
                    newImg->setPixel(newX, y, img->getPixel(x, y));
                    newX++;
                }
            }
        }
        return newImg;
    }

public:
    Image* finalReconstructedImage = NULL;

    ~SeamCarvingCompressor() {
        if (finalReconstructedImage != NULL) {
            delete finalReconstructedImage;
        }
    }

    CompressionResult compress(Image* img, int seamsToRemove) {
        CompressionResult result;
        clock_t start = clock();
        
        cout << "\n=== SEAM CARVING (DYNAMIC PROGRAMMING + BACKTRACKING) ===\n";
        cout << "Target: Removing " << seamsToRemove << " lowest-energy continuous pixel seams...\n";
        
        // Ensure we don't delete the whole image
        if (seamsToRemove >= img->width - 1) seamsToRemove = img->width - 2;

        Image* currentImg = new Image(img->width, img->height);
        // Copy original
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) currentImg->setPixel(x, y, img->getPixel(x, y));
        }

        for (int i = 0; i < seamsToRemove; i++) {
            if (i % 10 == 0) cout << "  Carving seam " << i << " of " << seamsToRemove << "...\n";
            int* optimalSeam = findOptimalSeam(currentImg);
            Image* shrunkImg = removeSeam(currentImg, optimalSeam);
            
            delete[] optimalSeam;
            delete currentImg;
            currentImg = shrunkImg;
        }

        clock_t end = clock();

        finalReconstructedImage = currentImg; // Save for export

        cout << "\nDP Statistics:\n";
        cout << "  Original Width: " << img->width << "px\n";
        cout << "  New Width: " << currentImg->width << "px\n";
        cout << "  Total Subproblems Solved: " << (img->width * img->height * seamsToRemove) << "\n";
        
        result.originalSize = img->getTotalPixels();
        result.compressedSize = currentImg->getTotalPixels();
        result.compressionRatio = (1.0 - (double)result.compressedSize / result.originalSize) * 100.0;
        result.executionTime = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        
        return result;
    }
};

#endif