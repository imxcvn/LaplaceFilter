#include "pch.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <Windows.h>
#include <cmath>


// Funkcja oblicza wartoœæ piksela na podstawie maski.
unsigned char CalculateValue(unsigned char* fragment, int* mask)
{
    int value = 0;

    for (int j = 0; j < 3; j++)
    {
        for (int i = 0; i < 3; i++)
        {
            int element = fragment[i + j * 3] * mask[i + j * 3];
            value += element;
        }
    }
    value = std::clamp<int>(value, 0, 255);
    return static_cast<unsigned char>(value);
}

extern "C" __declspec(dllexport) void ProcessImageFragment(unsigned char* pixels, unsigned char* outputPixels, int width, int height) {
    // Maska Laplace'a
    int* mask = new int[9];

    // Inicjalizacja maski Laplace'a LAPL1
    //  1 -2  1
    // -2  4 -2
    //  1 -2  1
    mask[0] = 1;  mask[1] = -2; mask[2] = 1;
    mask[3] = -2; mask[4] = 4;  mask[5] = -2;
    mask[6] = 1;  mask[7] = -2; mask[8] = 1;

    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int index = (y * width + x) * 3;

            unsigned char r[9], g[9], b[9];

            for (int j = 0; j < 3; j++)
            {
                for (int i = 0; i < 3; i++)
                {
                    int neighborIndex = ((y + j - 1) * width + (x + i - 1)) * 3;
                    int maskIndex = i + j * 3;

                    b[maskIndex] = pixels[neighborIndex];
                    g[maskIndex] = pixels[neighborIndex + 1];
                    r[maskIndex] = pixels[neighborIndex + 2];
                    
                }
            }
            outputPixels[index] = CalculateValue(b, mask);
            outputPixels[index + 1] = CalculateValue(g, mask);
            outputPixels[index + 2] = CalculateValue(r, mask);
        }
    }
    delete[] mask;
}