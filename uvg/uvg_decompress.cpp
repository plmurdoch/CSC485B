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
/*
Assignment 3: Image Compression Code
Written By: Payton Murdoch
Student Number: V00904677
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

//Global vector which stores the encoding order for 8x8 Quantized DCT matrices.
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

//Function for building the Coefficient matrix to utilize in quantization
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

/*
Note: There were interesting interactions which resulted in me creating 2 DCT functions.
For the the High setting worked in a different way in comparison to the medium and low settings.
This was found after many hours of trial and error in the debugging process. Utilizing the method:
DCT(A) = CAC^T yielded a result in the high matrix which turned the entire image green.
However, utilizing a method DCT(A) = ACC^T kept the matrices in tact. I cannot explain how this occured.
Therefore to compensate for this we have inverse_DCT_low for medium and low values and inverse_DCT_high for the high setting values.  
*/
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

//Main body for the code read_input reads the input until the associated matrix of size height and width is filled completely.
//Number:length pair bytes are read and then stored in the encoded vector, once the encoded vector has enough entries for an 8x8 matrix they are offloaded into a DCT matrix.
//DCT matrix is then inverted using our coefficient in the order based on the quality. Data is then rounded and bounded to the range 0-255 for image reconstruction.
std::vector<std::vector<unsigned char>> read_input(InputBitStream input, std::vector<std::vector<int>> Q, std::vector<std::vector<double>> C, int height, int width, int quality){
    auto result = create_2d_vector<unsigned char>(height, width);
    int counter = 0;
    std::vector<unsigned char> encoded;
    int recorded_y = 0;
    int recorded_x = 0;
    int first_bit = -1;
    std::vector<int> bit_buffer;
    while(true){
        std::vector<int> num;
        if(first_bit == -1){
            int to_parse = input.read_byte();
            std::vector<int> first_bin;
            for(int i = 0; i<8; i++){
                first_bin.push_back(to_parse%2);
                to_parse = floor(to_parse/2);
            }
            first_bit = first_bin.at(7);
            int count = 6;
            while(count >=0){
                num.push_back(first_bin.at(count));
                count--;
            }
        }else{
            if(!bit_buffer.empty()){
                int size = bit_buffer.size();
                for(int i = 0; i<size; i++){
                    num.push_back(bit_buffer.at(i));
                }
                bit_buffer.clear();
            }
        }
        std::vector<int> next;
        int next_byte = input.read_byte();
        for(int i = 0; i<8; i++){
            next.push_back(next_byte%2);
            next_byte = floor(next_byte/2);
        }
        int count = 7;
        while(num.size() <8){
            num.push_back(next.at(count));
            count--;
        }
        int number = 0;
        int exp = 0;
        for(int i = 7; i>= 0; i--){
            number+= num.at(i)*pow(2,exp);
            exp++;
        }
        encoded.push_back((unsigned char)number);
        int length_code = 0;
        if(count < 0){
            next_byte = input.read_byte();
            next.clear();
            for(int i = 0; i<8; i++){
                next.push_back(next_byte%2);
                next_byte = floor(next_byte/2);
            }
            count = 7;
        }
        while(next.at(count)!= 0){
            length_code++;
            count--;
            if(count < 0){
                next_byte = input.read_byte();
                next.clear();
                for(int i = 0; i<8; i++){
                    next.push_back(next_byte%2);
                    next_byte = floor(next_byte/2);
                }
                count = 7;
            }
        }
        count--;
        if(length_code == 0){
            while(count >= 0){
                bit_buffer.push_back(next.at(count));
                count--;
            }
        }else{
            if (length_code == 1){
                encoded.push_back((unsigned char)number);
                while(count >= 0){
                    bit_buffer.push_back(next.at(count));
                    count--;
                }
            }else{
                if((length_code-1)<= (count+1)){
                    std::vector<int> length_bin;
                    length_bin.push_back(1);
                    for(int i = 0; i<length_code-1; i++){
                        length_bin.push_back(next.at(count));
                        count--;
                    }
                    int size = length_bin.size();
                    int len = 0;
                    int exponent = 0;
                    for(int i = size-1; i>= 0; i--){
                        len += length_bin.at(i)*pow(2,exponent);
                        exponent++;
                    }
                    for(int i = 0; i<len; i++){
                        encoded.push_back((unsigned char)number);
                    }
                    while(count >= 0){
                        bit_buffer.push_back(next.at(count));
                        count--;
                    }
                }else{
                    std::vector<int> length_bin;
                    length_bin.push_back(1);
                    while(count>= 0){
                        length_bin.push_back(next.at(count));
                        count--;
                        length_code--;
                    }
                    next_byte = input.read_byte();
                    next.clear();
                    for(int i = 0; i<8; i++){
                        next.push_back(next_byte%2);
                        next_byte = floor(next_byte/2);
                    }
                    count = 7;
                    for(int i = 0; i< length_code-1; i++){
                        length_bin.push_back(next.at(count));
                        count--;
                    }
                    int size = length_bin.size();
                    int len = 0;
                    int exponent = 0;
                    for(int i = size-1; i>= 0; i--){
                        len += length_bin.at(i)*pow(2,exponent);
                        exponent++;
                    }
                    for(int i = 0; i<len; i++){
                        encoded.push_back((unsigned char)number);
                    }
                    while(count >= 0){
                        bit_buffer.push_back(next.at(count));
                        count--;
                    }
                }
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
            bit_buffer.clear();
            first_bit = -1; 
            auto data = create_2d_vector<double>(8,8);
            if(quality != 2){
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
    //After trial and error I decided to utilize a single quantum value.
    //I utilized the resouce below to better understand standard quantum matrices.
    //I then create one that worked well with my DCT to limit the number of values which exceed the range of -127 and 127. 
    //https://cs.stanford.edu/people/eroberts/courses/soco/projects/data-compression/lossy/jpeg/coeff.htm 
    std::vector<std::vector<int>> Q = {
        {8,16,19,22,26,27,29,34},
        {16,16,22,24,27,29,34,37},
        {19,22,26,27,29,34,34,38},
        {22,22,26,27,29,34,37,40},
        {22,26,27,29,32,35,40,48},
        {26,27,29,32,35,40,48,58},
        {26,27,29,34,38,46,56,69},
        {27,29,35,38,46,56,69,83}
    };
    //the next for loops allow us to change the quality setting of our Quantum based on the user input.
    if(quality == 0){
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q.at(i).at(j) = (2*Q.at(i).at(j));
            }
        }
    }else if(quality == 2){
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q.at(i).at(j) = (round((0.2*Q.at(i).at(j)))); 
            }
        }
    }else{
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q.at(i).at(j) = ((Q.at(i).at(j)+8));    
            }
        }
    }
    std::vector<std::vector<double>> coeff = Coeff();
    unsigned int height = input_stream.read_u32();
    unsigned int width = input_stream.read_u32();
    auto Y = create_2d_vector<unsigned char>(height,width);
    auto Cb_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    auto Cr_scaled = create_2d_vector<unsigned char>((height+1)/2,(width+1)/2);
    //Build Y matrix.
    Y = read_input(input_stream, Q, coeff,height, width, quality);
    //Build Cb matrix.
    Cb_scaled = read_input(input_stream, Q, coeff,((height+1)/2),((width+1)/2), quality);
    //Build Cr matrix.
    Cr_scaled = read_input(input_stream, Q, coeff,((height+1)/2),((width+1)/2), quality);    
    //Rebuild image.
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