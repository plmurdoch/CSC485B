/* uvg_decompress.cpp

   Starter code for Assignment 3 (in C++). This program
    - Reads a height/width value from the input file
    - Reads YCbCr data from the file, with the Y plane
      in full w x h resolution and the other two planes
      in half resolution.
    - Upscales the Cb and Cr planes to full resolution and
      transforms them to RGB.
    - Writes the result as a BMP image
     (Using bitmap_image.hpp, originally from 
      http://partow.net/programming/bitmap/index.html)

   B. Bird - 2023-07-03
*/

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include "input_stream.hpp"
#include "bitmap_image.hpp"
#include "uvg_common.hpp"



int main(int argc, char** argv){
    if (argc < 3){
        std::cerr << "Usage: " << argv[0] << " <input file> <output BMP>" << std::endl;
        return 1;
    }
    std::string input_filename {argv[1]};
    std::string output_filename {argv[2]};


    std::ifstream input_file{input_filename,std::ios::binary};
    InputBitStream input_stream {input_file};

    unsigned int height = input_stream.read_u32();
    unsigned int width = input_stream.read_u32();
    
    auto Y = create_2d_vector<unsigned char>(height,width);
    auto Cb_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    auto Cr_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);

    for (unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++)
            Y.at(y).at(x) = input_stream.read_byte();

    for (unsigned int y = 0; y < (height+1)/2; y++)
        for (unsigned int x = 0; x < (width+1)/2; x++)
            Cb_scaled.at(y).at(x) = input_stream.read_byte();

    for (unsigned int y = 0; y < (height+1)/2; y++)
        for (unsigned int x = 0; x < (width+1)/2; x++)
            Cr_scaled.at(y).at(x) = input_stream.read_byte();
    
    auto imageYCbCr = create_2d_vector<PixelYCbCr>(height,width);
    for (unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            imageYCbCr.at(y).at(x) = {
                Y.at(y).at(x),
                Cb_scaled.at(y/2).at(x/2),
                Cr_scaled.at(y/2).at(x/2)
            };
        }
    }
    

    input_stream.flush_to_byte();
    input_file.close();

    bitmap_image output_image {width,height};

    for (unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            auto pixel_rgb = imageYCbCr.at(y).at(x).to_rgb();
            auto [r,g,b] = pixel_rgb;
            output_image.set_pixel(x,y,r,g,b);
        }
    }
    
    output_image.save_image(output_filename);

    return 0;
}