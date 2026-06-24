# 🗜️ Algorithmic Image Lab

An educational C++ sandbox designed to visualize and compare fundamental computer science algorithms through raw image data.
Rather than treating images merely as files to compress, this project demonstrates mathematical, statistical, and spatial concepts by applying classic algorithms directly to pixels. It serves as both a compression laboratory and an interactive visualization tool for core computer science principles.

---

## 🚀 Algorithms Explored

### Huffman Coding (Greedy / Statistical Compression)

Demonstrates **statistical redundancy** within images.

Frequently occurring pixel values are assigned shorter binary codes, while less common values receive longer codes. Images with limited color palettes or repeated pixel values achieve higher compression ratios.

**Concepts:** Greedy Algorithms, Frequency Analysis, Entropy Reduction.

---

### Arithmetic Coding (Probability & Information Theory)

Visualizes **Shannon's Entropy Limit**.

Instead of encoding individual symbols, Arithmetic Coding represents the entire image as a single fractional value within a probability interval. This allows compression beyond Huffman's integer-bit limitations and approaches the theoretical optimum.

**Concepts:** Probability Models, Information Theory, Entropy.

---

### Quadtree Compression (Divide and Conquer)

Demonstrates **spatial redundancy**.

The image is recursively divided into four regions until each block becomes sufficiently uniform. Large regions of identical colors can therefore be represented with minimal information.

This method is especially effective for:

* Logos
* Pixel art
* Icons
* Flat graphics

**Concepts:** Divide and Conquer, Recursive Data Structures, Spatial Compression.

---

### Seam Carving (Dynamic Programming)

A content-aware image resizing algorithm.

Instead of uniformly scaling an image, Seam Carving computes low-energy paths through the image and removes them while preserving important visual structures such as edges and objects.

This demonstrates that human visual perception is strongly influenced by contrast and edge information.

**Concepts:** Dynamic Programming, Energy Functions, Path Optimization.

---

## 🛠️ Technologies Used

* C++
* STL Data Structures
* Dynamic Programming
* Greedy Algorithms
* Divide and Conquer
* Probability Models
* `stb_image`
* `stb_image_write`

---

## ▶️ How to Run

1. Clone the repository.

```bash
git clone (https://github.com/haarshadd/Algorithmic-Image-Lab)
cd Algorithmic-Image-Lab
```

2. Compile the program:

```bash
g++ main.cpp -o ImageLab
```

3. Run the executable:

```bash
./ImageLab
```

4. Use the interactive command-line interface to:

   * Load `.png`, `.jpg`, or `.bmp` images.
   * Apply different algorithms.
   * Compare compression behavior and visual outputs.

---

## 🎯 Educational Goals

This project demonstrates how classical algorithms from computer science appear in real-world applications:

| Algorithm         | CS Topic            | Real-World Application      |
| ----------------- | ------------------- | --------------------------- |
| Huffman Coding    | Greedy Algorithms   | ZIP, PNG                    |
| Arithmetic Coding | Information Theory  | JPEG2000, Video Codecs      |
| Quadtree          | Divide & Conquer    | GIS, Graphics Engines       |
| Seam Carving      | Dynamic Programming | Content-Aware Image Editing |

---

## 📚 What This Project Proves

* Compression is fundamentally about removing redundancy.
* Information theory defines the limits of compression.
* Spatial patterns can be exploited differently than statistical patterns.
* Dynamic programming can solve visual optimization problems.
* Computer science concepts become easier to understand when visualized.

---

*Image loading and saving are handled using the lightweight `stb_image` and `stb_image_write` libraries.*
