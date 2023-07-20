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

std::vector<std::pair<int, int>> E_O = {
    {0,0},{0,1},{1,0},{2,0},{1,1},{0,2},{0,3},{1,2},
    {2,1},{3,0},{4,0},{3,1},{2,2},{1,3},{0,4},{0,5},
    {1,4},{2,3},{3,2},{4,1},{5,0},{6,0},{5,1},{4,2},
    {3,3},{2,4},{1,5},{0,6},{0,7},{1,6},{2,5},{3,4},
    {4,3},{5,2},{6,1},{7,0},{7,1},{6,2},{5,3},{4,4},
    {3,5},{2,6},{1,7},{2,7},{3,6},{4,5},{5,4},{6,3},
    {7,2},{7,3},{6,4},{5,5},{4,6},{3,7},{4,7},{5,6},
    {6,5},{7,4},{7,5},{6,6},{5,7},{6,7},{7,6},{7,7}
};

std::vector<std::vector<int>> Q_l = {
    {16,11,10,16,24,40,51,61},
    {12,12,14,19,26,58,60,55},
    {14,13,16,24,40,57,69,56},
    {14,17,22,29,51,87,80,62},
    {18,22,37,56,68,109,103,77},
    {24,35,55,64,81,104,113,92},
    {49,64,78,87,103,121,120,101},
    {72,92,95,98,112,100,103,99}
};

std::vector<std::vector<int>> Q_C = {
    {17,18,24,47,99,99,99,99},
    {18,21,26,66,99,99,99,99},
    {24,26,56,99,99,99,99,99},
    {47,66,99,99,99,99,99,99},
    {99,99,99,99,99,99,99,99},
    {99,99,99,99,99,99,99,99},
    {99,99,99,99,99,99,99,99},
    {99,99,99,99,99,99,99,99}
};

int main(int argc, char** argv){
    if (argc < 3){
        std::cerr << "Usage: " << argv[0] << " <input file> <output BMP>" << std::endl;
        return 1;
    }
    std::string input_filename {argv[1]};
    std::string output_filename {argv[2]};


    std::ifstream input_file{input_filename,std::ios::binary};
    InputBitStream input_stream {input_file};
    unsigned int quality = input_stream.read_bits(2);
    if(quality == 0){
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q_l.at(i).at(j) = (2*Q_l.at(i).at(j));
                Q_C.at(i).at(j) = (2*Q_C.at(i).at(j)); 
            }
        }
    }else if(quality == 2){
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q_l.at(i).at(j) = (round(0.25*Q_l.at(i).at(j)));
                Q_C.at(i).at(j) = (round(0.25*Q_C.at(i).at(j))); 
            }
        }
    }
    unsigned int height = input_stream.read_u32();
    unsigned int width = input_stream.read_u32();
    int  Y_count = ((ceil(height*0.125)*ceil(width*0.125))*64);
    int Cr_Cb = ceil(((width+1)/2)*0.125)*ceil(((height+1)/2)*0.125)*64;
    auto Y = create_2d_vector<unsigned char>(height,width);
    auto Cb_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    auto Cr_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    std::vector<unsigned char> encoded;
    int count = 0;
    while(count<Y_count){
        unsigned char num = input_stream.read_byte();
        encoded.push_back(encoded);
        std::vector<unsigned char> length;
        length.push_back(input_stream.read_bit());
        if(length.at(0) != 0){
            unsigned char unary = input_stream.read_bit();
            while(unary != 0){
                length.push_back(unary);
                unary = input_stream.read_bit();
            }
            int bits = length.size();
            std::vector<unsigned char> length_bits;
            for(int i = 0; i<bits-1; i++){
                length_bits.push_back(input_stream.read_bit());
            }
            //compute length of repetitions.
        }else{
            if(encoded.size() == 64){
                //fill next matrix
            }
            count++;
        }
    }
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