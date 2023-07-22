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
#include <cmath>
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
std::vector<std::vector<double>> Coeff(){
    auto results = create_2d_vector<double>(8,8);
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            if(i == 0){
                results.at(i).at(j) = sqrt(0.125);
            }else{
                results.at(i).at(j) = (sqrt((0.25))*cos((((2.00*j)+1.00)*i*M_PI)/16.00));
            }
        }
    }
    return results;
}

std::vector<std::vector<double>> inverse_DCT_low(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(8,8);
    auto temp = create_2d_vector<double>(8,8);
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                temp.at(i).at(j) += (data.at(x).at(j)*c.at(x).at(i));//for c transpose
            }
        }
    }
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                results.at(i).at(j) += (temp.at(i).at(x)*c.at(x).at(j)); 
            }
        }
    }
    return results;
}

std::vector<std::vector<double>> inverse_DCT_high(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(8,8);
    auto temp = create_2d_vector<double>(8,8);
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                temp.at(i).at(j) += (data.at(i).at(x)*c.at(j).at(x));//for c transpose
            }
        }
    }
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                results.at(i).at(j) += (temp.at(i).at(x)*c.at(x).at(j)); 
            }
        }
    }
    return results;
}

std::vector<std::vector<unsigned char>> read_input(InputBitStream input, std::vector<std::vector<int>> Q, std::vector<std::vector<double>> C, int height, int width, int quality){
    auto result = create_2d_vector<unsigned char>(height, width);
    int counter = 0;
    std::vector<unsigned char> encoded;
    int recorded_y = 0;
    int recorded_x = 0;
    while(true){
        unsigned char num = input.read_byte();
        encoded.push_back(num);
        unsigned char length = input.read_byte();
        int value = length;
        if(value != 0){
            for(int j = 0; j<value; j++){
                encoded.push_back(num);
            }
        }
        int encoded_size = encoded.size();
        if(encoded_size == 64){
            auto DCT = create_2d_vector<int>(8,8);
            int E_O_size = E_O.size();
            for(int i = 0; i<E_O_size; i++){
                int row = E_O.at(i).first;
                int column = E_O.at(i).second;
                int for_DCT = encoded.at(i);
                DCT.at(row).at(column) = ((for_DCT-127)*Q.at(row).at(column));
            }
            encoded.clear();
            auto data = create_2d_vector<double>(8,8);
            if(quality == 0){
                data = inverse_DCT_low(DCT, C);
            }else{
                data = inverse_DCT_high(DCT, C);
            }
            for(int y = 0; y < 8; y++){
                for(int x = 0; x<8; x++){
                    if((y+recorded_y) < height){
                        if((x+recorded_x) < width){
                            int dat = round(data.at(y).at(x));
                            if(dat >255){
                                dat = 255;
                            }else if(dat <0){
                                dat = 0;
                            }
                            result.at(y+recorded_y).at(x+recorded_x) = dat;
                            if (counter == ((height*width)-1)){
                                return result;
                            }else{
                                counter++;
                            }
                        }
                    }
                }
            }
            if((recorded_x+8) < width){
                recorded_x += 8;
            }else{
                recorded_x = 0;
                recorded_y += 8;
            }
        }
    }
}

int main(int argc, char** argv){
    if (argc < 3){
        std::cerr << "Usage: " << argv[0] << " <input file> <output BMP>" << std::endl;
        return 1;
    }
    std::string input_filename {argv[1]};
    std::string output_filename {argv[2]};
    std::ifstream input_file{input_filename,std::ios::binary};
    InputBitStream input_stream {input_file};
    unsigned int quality = input_stream.read_byte();
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
        {8,16,19,22,26,27,29,34},
        {16,16,22,24,27,29,34,37},
        {19,22,26,27,29,34,34,38},
        {22,22,26,27,29,34,37,40},
        {22,26,27,29,32,35,40,48},
        {26,27,29,32,35,40,48,58},
        {26,27,29,34,38,46,56,69},
        {27,29,35,38,46,56,69,83}
    };
    if(quality == 0){
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q_l.at(i).at(j) = round((2*Q_l.at(i).at(j)));
                Q_C.at(i).at(j) = round((2*Q_C.at(i).at(j))); 
            }
        }
    }else if(quality == 2){
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q_l.at(i).at(j) = (round((0.25*Q_l.at(i).at(j))));
                Q_C.at(i).at(j) = (round((0.25*Q_C.at(i).at(j)))); 
            }
        }
    }
    std::vector<std::vector<double>> coeff = Coeff();
    unsigned int height = input_stream.read_u32();
    unsigned int width = input_stream.read_u32();
    auto Y = create_2d_vector<unsigned char>(height,width);
    auto Cb_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    auto Cr_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    
    Y = read_input(input_stream, Q_l, coeff,height, width, quality);
    Cb_scaled = read_input(input_stream, Q_C, coeff,((height+1)/2),((width+1)/2), quality);
    Cr_scaled = read_input(input_stream, Q_C, coeff,((height+1)/2),((width+1)/2), quality);    
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