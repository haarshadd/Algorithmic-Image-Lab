#ifndef QUADTREE_H
#define QUADTREE_H
#include <iostream>
#include "ImageHandler.h"
#include <ctime>
#include <cmath>
using namespace std;

struct QuadNode {
    bool isLeaf;       
    int color;         
    QuadNode* topLeft;
    QuadNode* topRight;
    QuadNode* bottomLeft;
    QuadNode* bottomRight;
    
    QuadNode() {
        isLeaf = false;
        color = 0;
        topLeft = NULL;
        topRight = NULL;
        bottomLeft = NULL;
        bottomRight = NULL;
    }
};

class QuadtreeCompressor {
private:
    Image* image;
    QuadNode* root;      //  Keep track of the root for reconstruction
    int rootSize;        //  Keep track of the starting size
    int totalNodes;
    int leafNodes;
    int threshold;  
    
    bool isUniform(int x, int y, int size) {
        int firstPixel = image->getPixel(x, y);
        for (int i = 0; i < size; i++) {
            for (int j = 0; j < size; j++) {
                int pixel = image->getPixel(x + j, y + i);
                if (abs(pixel - firstPixel) > threshold) //If difference > threshold then not uniform
                 {

                    return false;  
                }
            }
        }
        return true;   // If all pixels similar then uniform
    }
    
    QuadNode* buildQuadtree(int x, int y, int size, int depth) {
        QuadNode* node = new QuadNode();
        totalNodes++;
        
        if (isUniform(x, y, size) || size == 1) {
            node->isLeaf = true;
            node->color = image->getPixel(x, y);
            leafNodes++;
            return node;
        }
        
        node->isLeaf = false;
        int halfSize = size / 2;
        
        node->topLeft = buildQuadtree(x, y, halfSize, depth + 1);
        node->topRight = buildQuadtree(x + halfSize, y, halfSize, depth + 1);
        node->bottomLeft = buildQuadtree(x, y + halfSize, halfSize, depth + 1);
        node->bottomRight = buildQuadtree(x + halfSize, y + halfSize, halfSize, depth + 1);
        
        return node;
    }

    // Helper to paint the blocks back onto a canvas
    void paintReconstruction(QuadNode* node, Image* canvas, int x, int y, int size) {
        if (node == NULL) return;
        
        if (node->isLeaf) {
            // Paint the entire block this uniform color
            for (int i = 0; i < size; i++) {
                for (int j = 0; j < size; j++) {
                    // Make sure we don't paint outside the original image bounds
                    if (x + j < canvas->width && y + i < canvas->height) {
                        canvas->setPixel(x + j, y + i, node->color);
                    }
                }
            }
        } else {
            // Recursively paint the 4 quadrants
            int half = size / 2;
            paintReconstruction(node->topLeft, canvas, x, y, half);
            paintReconstruction(node->topRight, canvas, x + half, y, half);
            paintReconstruction(node->bottomLeft, canvas, x, y + half, half);
            paintReconstruction(node->bottomRight, canvas, x + half, y + half, half);
        }
    }
    
public:
    QuadtreeCompressor() {
        threshold = 5;  
        root = NULL;
        rootSize = 0;
    }
    
    void setThreshold(int t) {
        threshold = t;
    }
    
    CompressionResult compress(Image* img) {
        CompressionResult result;
        clock_t start = clock();
        
        image = img;
        totalNodes = 0;
        leafNodes = 0;
        
        cout << "\n=== QUADTREE (DIVIDE & CONQUER) ===\n";
        cout << "Uniformity threshold: " << threshold << "\n";
        
        rootSize = 1;
        while (rootSize < img->width || rootSize < img->height) {
            rootSize *= 2;
        }
        
        cout << "Building quadtree for " << rootSize << "x" << rootSize << " region...\n";
        
        root = buildQuadtree(0, 0, rootSize, 0);
        
        clock_t end = clock();
        
        cout << "\nQuadtree Statistics:\n";
        cout << "  Total nodes: " << totalNodes << "\n";
        cout << "  Leaf nodes:  " << leafNodes << "\n";
        cout << "  Internal nodes: " << (totalNodes - leafNodes) << "\n";
        
        double totalBits = (double)totalNodes + (double)(leafNodes * 8);
        int compressedBytes = ceil(totalBits / 8.0);
        
        result.originalSize = img->getTotalPixels();
        result.compressedSize = compressedBytes;
        result.compressionRatio = (1.0 - (double)result.compressedSize / result.originalSize) * 100.0;
        result.executionTime = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        
        return result;
    }

    // Create the visual reconstruction image
    Image* getReconstructedImage() {
        if (root == NULL || image == NULL) return NULL;
        
        Image* canvas = new Image(image->width, image->height);
        paintReconstruction(root, canvas, 0, 0, rootSize);
        return canvas;
    }
};

#endif