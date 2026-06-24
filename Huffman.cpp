#ifndef HUFFMAN_H
#define HUFFMAN_H

#include "ImageHandler.h"
#include <ctime>
#include <fstream>
#include <iomanip>

struct HuffmanNode {
    int value;          
    int frequency;      
    HuffmanNode* left;
    HuffmanNode* right;
    
    HuffmanNode(int val, int freq) {
        value = val;
        frequency = freq;
        left = NULL;
        right = NULL;
    }
};

class HuffmanCompressor {
private:
    char codes[256][20];  
    int codeExists[256];  
    
    void countFrequency(Image* img, int frequency[256]) {
        for (int i = 0; i < 256; i++) {
            frequency[i] = 0;
        }
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                int pixel = img->getPixel(x, y);
                frequency[pixel]++;
            }
        }
    }
    
    HuffmanNode* findMin(HuffmanNode* nodes[], int size, int& minIndex) {
        int minFreq = 999999;
        minIndex = -1;
        for (int i = 0; i < size; i++) {
            if (nodes[i] != NULL && nodes[i]->frequency < minFreq) {
                minFreq = nodes[i]->frequency;
                minIndex = i;
            }
        }
        if (minIndex == -1) return NULL;
        HuffmanNode* result = nodes[minIndex];
        nodes[minIndex] = NULL;  
        return result;
    }
    
    HuffmanNode* buildHuffmanTree(int frequency[256], int& uniqueColors) {
        HuffmanNode* nodes[256];
        
        for (int i = 0; i < 256; i++) {
            nodes[i] = NULL;
        }
        
        int nodeCount = 0;
        for (int i = 0; i < 256; i++) {
            if (frequency[i] > 0) {
                nodes[nodeCount] = new HuffmanNode(i, frequency[i]);
                nodeCount++;
            }
        }
        
        uniqueColors = nodeCount;
        cout << "\n[Huffman] Found " << uniqueColors << " unique colors\n";
        cout << "[Huffman] Building tree using Greedy Algorithm...\n";
        
        while (nodeCount > 1) {
            int minIdx1, minIdx2;
            HuffmanNode* min1 = findMin(nodes, 256, minIdx1);
            HuffmanNode* min2 = findMin(nodes, 256, minIdx2);
            
            HuffmanNode* parent = new HuffmanNode(-1, min1->frequency + min2->frequency);
            parent->left = min1;
            parent->right = min2;
            
            for (int i = 0; i < 256; i++) {
                if (nodes[i] == NULL) {
                    nodes[i] = parent;
                    break;
                }
            }
            nodeCount--;  
        }
        
        for (int i = 0; i < 256; i++) {
            if (nodes[i] != NULL) {
                return nodes[i];
            }
        }
        return NULL;
    }
    
    void generateCodes(HuffmanNode* root, char* currentCode, int depth) {
        if (root == NULL) return;
        if (root->left == NULL && root->right == NULL) {
            currentCode[depth] = '\0';  
            for (int i = 0; i <= depth; i++) {
                codes[root->value][i] = currentCode[i];
            }
            codeExists[root->value] = 1;
            return;
        }
        currentCode[depth] = '0';
        generateCodes(root->left, currentCode, depth + 1);
        currentCode[depth] = '1';
        generateCodes(root->right, currentCode, depth + 1);
    }
    
public:
    HuffmanCompressor() {
        for (int i = 0; i < 256; i++) {
            codeExists[i] = 0;
        }
    }
    
    CompressionResult compress(Image* img) {
        CompressionResult result;
        clock_t start = clock();
        
        cout << "\n=== HUFFMAN CODING (GREEDY ALGORITHM) ===\n";
        
        int frequency[256];
        countFrequency(img, frequency);
        
        int uniqueColors;
        HuffmanNode* root = buildHuffmanTree(frequency, uniqueColors);
        
        char currentCode[20];
        generateCodes(root, currentCode, 0);
        
        int totalBits = 0;
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                int pixel = img->getPixel(x, y);
                int codeBits = 0;
                for (int i = 0; codes[pixel][i] != '\0'; i++) {
                    codeBits++;
                }
                totalBits += codeBits;
            }
        }
        
        clock_t end = clock();
        
        result.originalSize = img->getTotalPixels(); 
        result.compressedSize = (totalBits / 8) + uniqueColors * 9; 
        result.compressionRatio = (1.0 - (double)result.compressedSize / result.originalSize) * 100.0;
        result.executionTime = ((double)(end - start) / CLOCKS_PER_SEC) * 1000.0;
        
        return result;
    }

    // ADDED: Function to export the dictionary to a text file
    void saveDictionaryToFile(const char* filename) {
        ofstream outFile(filename);
        if (!outFile.is_open()) {
            cout << "[ERROR] Could not create file " << filename << "\n";
            return;
        }
        
        outFile << "========================================\n";
        outFile << "      HUFFMAN PREFIX CODE DICTIONARY    \n";
        outFile << "========================================\n";
        outFile << "Pixel Value | Generated Binary Code\n";
        outFile << "------------|---------------------------\n";
        
        for (int i = 0; i < 256; i++) {
            if (codeExists[i]) {
                outFile << "    " << setw(3) << i << "     | " << codes[i] << "\n";
            }
        }
        
        outFile.close();
        cout << "[SUCCESS] Huffman dictionary saved as: " << filename << "\n";
    }
};

#endif