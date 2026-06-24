#include <iostream>
#include <string>
#include <iomanip>
#include <cmath>
#include <climits>
#include <vector>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "ImageHandler.h"
#include "Huffman.cpp"
#include "Quadtree.cpp"
#include "SeamCarving.cpp"
#include "ArithmeticCoding.cpp"

using namespace std;

// ============================================================
//  ImageLoader
// ============================================================
class ImageLoader {
public:
    static Image* loadImage(const char* filename) {
        int width, height, channels;
        unsigned char* data = stbi_load(filename, &width, &height, &channels, 1);
        if (data == NULL) {
            cout << "\n[ERROR] Failed to load: " << filename << "\n";
            cout << "Reason: " << stbi_failure_reason() << "\n";
            return NULL;
        }
        Image* img = new Image(width, height);
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                img->setPixel(x, y, data[y * width + x]);
        stbi_image_free(data);
        cout << "\n[SUCCESS] Loaded: " << filename
             << " (" << width << "x" << height << ")\n";
        return img;
    }

    static void saveReconstruction(const char* filename, Image* img) {
        if (img == NULL) return;
        unsigned char* data = new unsigned char[img->width * img->height];
        for (int y = 0; y < img->height; y++)
            for (int x = 0; x < img->width; x++)
                data[y * img->width + x] = (unsigned char)img->getPixel(x, y);
        stbi_write_png(filename, img->width, img->height, 1, data, img->width);
        cout << "[SUCCESS] Saved: " << filename << "\n";
        delete[] data;
    }
};

long getSimulatedPngSize(Image* img) {
    if (!img) return 0;
    unsigned char* data = new unsigned char[img->width * img->height];
    for (int y = 0; y < img->height; y++)
        for (int x = 0; x < img->width; x++)
            data[y * img->width + x] = (unsigned char)img->getPixel(x, y);
    
    stbi_write_png("temp_measure.png", img->width, img->height, 1, data, img->width);
    delete[] data;
    
    long size = getFileSizeBytes("temp_measure.png");
    remove("temp_measure.png"); 
    return size;
}

// ============================================================
//  ImageStats + ImageAnalyzer
// ============================================================
struct ImageStats {
    double entropy;
    double variance;
    double edgeDensity;
    int    uniqueColors;
};

class ImageAnalyzer {
public:
    static ImageStats analyze(Image* img) {
        ImageStats stats;
        int total = img->getTotalPixels();

        int freq[256];
        for (int i = 0; i < 256; i++) freq[i] = 0;
        for (int y = 0; y < img->height; y++)
            for (int x = 0; x < img->width; x++)
                freq[img->getPixel(x, y)]++;

        stats.uniqueColors = 0;
        for (int i = 0; i < 256; i++) if (freq[i] > 0) stats.uniqueColors++;

        stats.entropy = 0.0;
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                double p = (double)freq[i] / total;
                stats.entropy -= p * log2(p);
            }
        }

        double mean = 0.0;
        for (int i = 0; i < 256; i++) mean += (double)i * freq[i];
        mean /= total;
        stats.variance = 0.0;
        for (int i = 0; i < 256; i++) {
            double d = i - mean;
            stats.variance += d * d * freq[i];
        }
        stats.variance /= total;

        int edgeCount = 0;
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                int left  = (x == 0)            ? img->getPixel(x,y) : img->getPixel(x-1,y);
                int right = (x == img->width-1) ? img->getPixel(x,y) : img->getPixel(x+1,y);
                int up    = (y == 0)            ? img->getPixel(x,y) : img->getPixel(x,y-1);
                int down  = (y == img->height-1)? img->getPixel(x,y) : img->getPixel(x,y+1);
                if (abs(left-right) + abs(up-down) > 15) edgeCount++;
            }
        }
        stats.edgeDensity = (double)edgeCount / total;

        return stats;
    }

    static int autoThreshold(const ImageStats& stats) {
        if (stats.variance < 300)  return 5;   
        if (stats.variance < 1000) return 12;  
        if (stats.variance < 3000) return 22;  
        return 35;                              
    }

    static int autoSeams(Image* img, const ImageStats& stats) {
        double safePercent;
        if      (stats.edgeDensity < 0.15) safePercent = 0.20;
        else if (stats.edgeDensity < 0.30) safePercent = 0.12;
        else if (stats.edgeDensity < 0.45) safePercent = 0.07;
        else                               safePercent = 0.03;

        int seams = (int)(img->width * safePercent);
        if (seams < 1) seams = 1;
        if (seams >= img->width - 2) seams = img->width - 2;
        return seams;
    }

    static const char* edgeLabel(double edgeDensity) {
        if (edgeDensity < 0.15) return "Low  (open backgrounds, sky, water)";
        if (edgeDensity < 0.30) return "Medium (landscapes, architecture)";
        if (edgeDensity < 0.45) return "High (portraits, objects)";
        return "Very High (text, crowds, dense detail)";
    }

    static const char* imageTypeLabel(const ImageStats& stats) {
        if (stats.variance < 300  && stats.uniqueColors < 50)
            return "Simple graphic / Logo / Pixel art";
        if (stats.variance < 1500 && stats.entropy < 5.5)
            return "Diagram / Screenshot / Cartoon";
        if (stats.entropy > 6.5)
            return "Natural photograph";
        return "Mixed content image";
    }

    static void printStats(const ImageStats& stats, Image* img) {
        cout << "\n┌──────────────────────────────────────────────────┐\n";
        cout << "│               IMAGE ANALYSIS REPORT              │\n";
        cout << "├──────────────────────────────────────────────────┤\n";
        cout << fixed << setprecision(3);
        cout << "│ Detected type : " << left << setw(33)
             << imageTypeLabel(stats) << "│\n";
        cout << "│ Entropy       : " << setw(6) << stats.entropy
             << " bits/pixel  (max possible: 8.0)  │\n";
        cout << "│ Variance      : " << setw(8) << (int)stats.variance
             << "   (low=uniform, high=complex)  │\n";
        cout << "│ Edge content  : " << left << setw(33)
             << edgeLabel(stats.edgeDensity) << "│\n";
        cout << "│ Unique colors : " << setw(3) << stats.uniqueColors
             << " out of 256                         │\n";
        cout << "├──────────────────────────────────────────────────┤\n";
        cout << "│ Auto-computed settings:                          │\n";
        cout << "│   Quadtree threshold : " << setw(3)
             << autoThreshold(stats) << " (auto)                      │\n";
        cout << "│   Seams to remove    : " << setw(4)
             << autoSeams(img, stats) << " pixels wide ("
             << setw(4) << setprecision(1)
             << (100.0 * autoSeams(img, stats) / img->width) << "% of width)    │\n";
        cout << "└──────────────────────────────────────────────────┘\n";
        cout << right; 
    }
};

// ============================================================
// Arithmetic Coding Companion Output
// ============================================================
void saveArithmeticOutput(Image* img, int compressedSizeBytes) {
    string binName = "arithmetic_compressed.bin";
    ofstream outBin(binName, ios::binary);
    if (outBin.is_open()) {
        vector<char> dummy(compressedSizeBytes, 0);
        outBin.write(dummy.data(), compressedSizeBytes);
        outBin.close();
    }

    string txtName = "arithmetic_model.txt";
    ofstream outTxt(txtName);
    if (outTxt.is_open()) {
        outTxt << "========================================\n";
        outTxt << "  ARITHMETIC CODING PROBABILITY MODEL   \n";
        outTxt << "========================================\n";
        outTxt << "Pixel | Frequency | Probability | Theo. Bits \n";
        outTxt << "------|-----------|-------------|------------\n";
        int freq[256] = {0};
        int total = img->getTotalPixels();
        for (int y = 0; y < img->height; y++)
            for (int x = 0; x < img->width; x++)
                freq[img->getPixel(x, y)]++;

        outTxt << fixed << setprecision(6);
        for (int i = 0; i < 256; i++) {
            if (freq[i] > 0) {
                double p = (double)freq[i] / total;
                double bits = -log2(p);
                outTxt << "  " << setw(3) << i << " | "
                       << setw(9) << freq[i] << " | "
                       << setw(11) << p << " | "
                       << setw(10) << bits << "\n";
            }
        }
        outTxt.close();
        cout << "[SUCCESS] Saved probability dictionary : " << txtName << "\n";
        cout << "[SUCCESS] Saved simulated binary file  : " << binName 
             << " (" << compressedSizeBytes << " bytes)\n";
    }
}

// ============================================================
// Reality Check Helper
// ============================================================
void displayDiskRealityCheck(long originalDiskSize, long finalDiskSize, bool isLossy = false) {
    cout << "\n[REALITY CHECK]\n";
    cout << "  Original File on Disk   : " << originalDiskSize << " bytes\n";
    cout << "  Final Output File Size  : " << finalDiskSize << " bytes\n";
    
    if (isLossy) {
        cout << "  -> Educational Note: Seam Carving deletes pixels, but it destroys predictable\n"
             << "     patterns in the image. Because the new image is 'noisier', a PNG compressor\n"
             << "     struggles to zip it. This demonstrates Information Entropy vs Raw Pixel Count!\n";
    } else if (finalDiskSize < originalDiskSize) {
        cout << "  -> Success! File size reduced by " << (originalDiskSize - finalDiskSize) << " bytes.\n";
    } else {
        cout << "  -> Educational Note: Highly optimized formats like PNG/JPG are currently\n"
             << "     smaller than our custom algorithm by " 
             << (finalDiskSize - originalDiskSize) << " bytes.\n"
             << "     This proves that multi-stage industry algorithms are highly efficient.\n";
    }
}

// ============================================================
//  Compare All 
// ============================================================
void compareAlgorithms(Image* img, string currentFilename) {
    if (img == NULL) { cout << "\n[ERROR] No image loaded!\n"; return; }

    cout << "\n=== FULL COMPARISON (ALL ALGORITHMS) ===\n";
    ImageStats stats = ImageAnalyzer::analyze(img);
    ImageAnalyzer::printStats(stats, img);

    int qtThreshold = ImageAnalyzer::autoThreshold(stats);
    int autoSeams   = ImageAnalyzer::autoSeams(img, stats);

    cout << "\nUsing auto-computed settings:\n";
    cout << "  Quadtree threshold = " << qtThreshold << "\n";
    cout << "  Seam Carving seams = " << autoSeams
         << " (" << setprecision(1) << (100.0*autoSeams/img->width) << "% of width)\n";

    cout << "\nRunning Huffman...\n";
    HuffmanCompressor* huffman = new HuffmanCompressor();
    CompressionResult hResult = huffman->compress(img);
    delete huffman;

    cout << "\nRunning Arithmetic Coding...\n";
    ArithmeticCoder* ac = new ArithmeticCoder();
    CompressionResult acResult = ac->compress(img);
    delete ac;

    cout << "\nRunning Quadtree (threshold=" << qtThreshold << ")...\n";
    QuadtreeCompressor* quadtree = new QuadtreeCompressor();
    quadtree->setThreshold(qtThreshold);
    CompressionResult qResult = quadtree->compress(img);
    delete quadtree;

    cout << "\nRunning Seam Carving (" << autoSeams << " seams)...\n";
    SeamCarvingCompressor* seam = new SeamCarvingCompressor();
    CompressionResult dpResult = seam->compress(img, autoSeams);
    
    long scRealDiskSize = getSimulatedPngSize(seam->finalReconstructedImage);
    delete seam;

    long diskSize = getFileSizeBytes(currentFilename.c_str());

    cout << "\n╔═══════════════════════════╦══════════════╦══════════════╦══════════════╦══════════════╗\n";
    cout << "║ Metric                    ║   Huffman    ║  Arith.Code  ║   Quadtree   ║  SeamCarve   ║\n";
    cout << "╠═══════════════════════════╬══════════════╬══════════════╬══════════════╬══════════════╣\n";
    cout << fixed << setprecision(2);
    cout << "║ Orig Disk File (bytes)    ║ " 
         << setw(12) << diskSize << " ║ " << setw(12) << diskSize << " ║ " 
         << setw(12) << diskSize << " ║ " << setw(12) << diskSize << " ║\n";
    cout << "║ Raw Memory Output (bytes) ║ " 
         << setw(12) << hResult.compressedSize << " ║ " << setw(12) << acResult.compressedSize << " ║ " 
         << setw(12) << qResult.compressedSize << " ║ " << setw(12) << dpResult.compressedSize << " ║\n";
    cout << "║ Final Disk Size (Actual)  ║ " 
         << setw(12) << hResult.compressedSize << " ║ " << setw(12) << acResult.compressedSize << " ║ " 
         << setw(12) << qResult.compressedSize << " ║ " << setw(12) << scRealDiskSize << " ║\n";
    cout << "║ Execution Time (ms)       ║ " 
         << setw(12) << hResult.executionTime << " ║ " << setw(12) << acResult.executionTime << " ║ " 
         << setw(12) << qResult.executionTime << " ║ " << setw(12) << dpResult.executionTime << " ║\n";
    cout << "║ Lossless?                 ║     Yes      ║      Yes     ║   Approx.    ║      No      ║\n";
    cout << "╚═══════════════════════════╩══════════════╩══════════════╩══════════════╩══════════════╝\n";

    cout << "\n[NOTE] 'Raw Memory' is the unzipped RAM footprint (Width × Height).\n";
    cout << "[NOTE] 'Final Disk Size' for Seam Carving shows its real size if saved back as a PNG.\n";
}

// ============================================================
//  Main
// ============================================================
void displayGuide() {
    cout << "\n===============================================================================\n";
    cout << "                      ALGORITHMIC IMAGE LAB                                  \n";
    cout << "          Exploring Entropy, Spatial Data, and Human Perception                \n";
    cout << "===============================================================================\n";
    cout << " This sandbox visualizes how Computer Science solves data problems:\n\n";
    cout << " [3] HUFFMAN CODING (Greedy)\n";
    cout << "     Proves that statistical redundancy (skewed colors) can be compressed\n";
    cout << "     by assigning short binary codes to frequent pixel values.\n\n";
    cout << " [4] ARITHMETIC CODING (Probabilistic Limits)\n";
    cout << "     Proves Shannon's Entropy limit. It breaks Huffman's 1-bit barrier by\n";
    cout << "     compressing the whole image into a single floating-point fraction.\n\n";
    cout << " [5] QUADTREE (Divide & Conquer)\n";
    cout << "     Proves Spatial Redundancy. Instead of math, it groups identical\n";
    cout << "     physical 2D blocks together. Great for logos, terrible for photos.\n\n";
    cout << " [6] SEAM CARVING (Dynamic Programming)\n";
    cout << "     Proves that human vision relies on edges and contrast, not pixel count.\n";
    cout << "     It finds the 'lowest energy' path to safely shrink image dimensions.\n";
    cout << "===============================================================================\n";
}

int main() {
    Image* currentImage = NULL;
    string currentFilename = "";
    displayGuide();

    while (true) {
        cout << "\n┌──────────────────────────────────────────────┐\n";
        cout << "│             ALGORITHMIC IMAGE LAB            │\n";
        cout << "├──────────────────────────────────────────────┤\n";
        cout << "│ 1. Upload Image (PNG/JPG/BMP)                │\n";
        cout << "│ 2. Display Image Info, Preview & Analysis    │\n";
        cout << "│ 3. Run — Huffman (Greedy/Statistical)        │\n";
        cout << "│ 4. Run — Arithmetic Coding (Optimal Limits)  │\n";
        cout << "│ 5. Run — Quadtree (Spatial D&C)              │\n";
        cout << "│ 6. Run — Seam Carving (Dynamic Programming)  │\n";
        cout << "│ 7. Compare All Algorithms                    │\n";
        cout << "│ 0. Exit                                      │\n";
        cout << "└──────────────────────────────────────────────┘\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            cin.clear(); 
            cin.ignore(10000, '\n');
            cout << "[ERROR] Invalid input.\n";
            continue;
        }
        cin.ignore(10000, '\n'); 
        if (choice == 0) break;

        switch (choice) {
            case 1: {
                cout << "Enter image path (e.g., photo.png): ";
                string filename; 
                getline(cin, filename); 
                Image* newImg = ImageLoader::loadImage(filename.c_str());
                if (newImg) { 
                    delete currentImage; 
                    currentImage = newImg; 
                    currentFilename = filename;
                }
                break;
            }
            case 2: {
                if (!currentImage) { cout << "\n[ERROR] No image loaded.\n"; break; }
                long diskSize = getFileSizeBytes(currentFilename.c_str());
                
                cout << "\n┌──────────────────────────────────────────────────┐\n";
                cout << "│               BASIC INFORMATION                  │\n";
                cout << "├──────────────────────────────────────────────────┤\n";
                cout << "│  Resolution   : " << left << setw(31) << (to_string(currentImage->width) + " x " + to_string(currentImage->height)) << "│\n";
                cout << "│  Raw Memory   : " << left << setw(31) << (to_string(currentImage->getTotalPixels()) + " bytes") << "│\n";
                cout << "│  File on Disk : " << left << setw(31) << (to_string(diskSize) + " bytes") << "│\n";
                cout << "└──────────────────────────────────────────────────┘\n";

                if (currentImage->width <= 64 && currentImage->height <= 64)
                    currentImage->display();
                
                ImageStats s = ImageAnalyzer::analyze(currentImage);
                ImageAnalyzer::printStats(s, currentImage);
                break;
            }
            case 3: {
                if (!currentImage) { cout << "\n[ERROR] No image loaded.\n"; break; }
                HuffmanCompressor* h = new HuffmanCompressor();
                CompressionResult r = h->compress(currentImage);
                r.display("Huffman Coding (Greedy)");
                displayDiskRealityCheck(getFileSizeBytes(currentFilename.c_str()), r.compressedSize);
                
                cout << "\nSave Huffman dictionary? (Y/N): ";
                char opt; cin >> opt;
                cin.ignore(10000, '\n'); 
                if (opt=='Y'||opt=='y') h->saveDictionaryToFile("huffman_dictionary.txt");
                delete h;
                break;
            }
            case 4: {
                if (!currentImage) { cout << "\n[ERROR] No image loaded.\n"; break; }
                ArithmeticCoder* ac = new ArithmeticCoder();
                CompressionResult r = ac->compress(currentImage);
                r.display("Arithmetic Coding");
                displayDiskRealityCheck(getFileSizeBytes(currentFilename.c_str()), r.compressedSize);

                cout << "\nSave Arithmetic probability model & output binary? (Y/N): ";
                char opt; cin >> opt;
                cin.ignore(10000, '\n'); 
                if (opt=='Y'||opt=='y') {
                    saveArithmeticOutput(currentImage, r.compressedSize);
                }
                delete ac;
                break;
            }
            case 5: {
                if (!currentImage) { cout << "\n[ERROR] No image loaded.\n"; break; }
                ImageStats s = ImageAnalyzer::analyze(currentImage);
                int autoT = ImageAnalyzer::autoThreshold(s);
                
                cout << "Auto-computed threshold: " << autoT
                     << "  (press Enter to accept, or type a number): ";
                
                string line; 
                getline(cin, line); 
                int t = autoT;
                if (!line.empty()) {
                    try { t = stoi(line); } catch (...) {
                        cout << "[WARNING] Invalid number entered. Using auto value: " << autoT << "\n";
                    }
                }
                
                QuadtreeCompressor* qt = new QuadtreeCompressor();
                qt->setThreshold(t);
                CompressionResult r = qt->compress(currentImage);
                r.display("Quadtree (Divide & Conquer)");
                displayDiskRealityCheck(getFileSizeBytes(currentFilename.c_str()), r.compressedSize);

                cout << "\nSave reconstruction image? (Y/N): ";
                char opt; cin >> opt;
                cin.ignore(10000, '\n'); 
                if (opt=='Y'||opt=='y') {
                    Image* recon = qt->getReconstructedImage();
                    ImageLoader::saveReconstruction("quadtree_compressed.png", recon);
                    delete recon;
                }
                delete qt;
                break;
            }
            case 6: {
                if (!currentImage) { cout << "\n[ERROR] No image loaded.\n"; break; }
                ImageStats s = ImageAnalyzer::analyze(currentImage);
                int autoS = ImageAnalyzer::autoSeams(currentImage, s);
                
                cout << "Auto-computed seams to remove: " << autoS
                     << " (" << setprecision(1)
                     << (100.0*autoS/currentImage->width) << "% width reduction)\n";
                cout << "(press Enter to accept, or type a number): ";
                
                string line; 
                getline(cin, line);
                int seams = autoS;
                if (!line.empty()) {
                    try { seams = stoi(line); } catch (...) {
                        cout << "[WARNING] Invalid number entered. Using auto value: " << autoS << "\n";
                    }
                }
                
                SeamCarvingCompressor* sc = new SeamCarvingCompressor();
                CompressionResult r = sc->compress(currentImage, seams);
                r.display("Seam Carving (DP + Backtracking)");
                
                long realPngSize = getSimulatedPngSize(sc->finalReconstructedImage);
                displayDiskRealityCheck(getFileSizeBytes(currentFilename.c_str()), realPngSize, true);

                cout << "\nSave resized image? (Y/N): ";
                char opt; cin >> opt;
                cin.ignore(10000, '\n'); 
                if (opt=='Y'||opt=='y')
                    ImageLoader::saveReconstruction("seam_carved.png", sc->finalReconstructedImage);
                delete sc;
                break;
            }
            case 7:
                if (!currentImage) cout << "\n[ERROR] No image loaded.\n";
                else compareAlgorithms(currentImage, currentFilename);
                break;
            default: cout << "\n[ERROR] Enter 0-7.\n";
        }
    }

    delete currentImage;
    cout << "\nGoodbye!\n";
    return 0;
}