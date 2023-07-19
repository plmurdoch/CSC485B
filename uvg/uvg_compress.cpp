/* uvg_compress.cpp

   Starter code for Assignment 3 (in C++). This program
    - Reads an input image in BMP format
     (Using bitmap_image.hpp, originally from 
      http://partow.net/programming/bitmap/index.html)
    - Transforms the image from RGB to YCbCr (i.e. "YUV").
    - Downscales the Cb and Cr planes by a factor of two
      (producing the same resolution that would result
       from 4:2:0 subsampling, but using interpolation
       instead of ignoring some samples)
    - Writes each colour plane (Y, then Cb, then Cr)
      in 8 bits per sample to the output file.

   B. Bird - 2023-07-03
*/
//What to do, compression techniques
//Check to see if we need to round and clamp DCT aswell.
//Change Q_1 so that it records different quantums based on the quality setting.
//Encoding is all we need with rudimentary techniques.
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cassert>
#include <cstdint>
#include <cmath>
#include "output_stream.hpp"
#include "bitmap_image.hpp"
#include "uvg_common.hpp"

//Based on quality the quantum for Q_1 will become a changing value
std::vector<std::vector<int>> Q_1 = {
    {3,5,7,9,11,13,15,17},
    {5,7,9,11,13,15,17,19},
    {7,9,11,13,15,17,19,21},
    {9,11,13,15,17,19,21,23},
    {11,13,15,17,19,21,23,25},
    {13,15,17,19,21,23,25,27},
    {15,17,19,21,23,25,27,29},
    {17,19,21,23,25,27,29,31}
};

//Make into matrix so that it only needs to be computed once instead of multiple computations and function calls.
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

//Compute DCT(A) = matrix(C)*matrix(A)*matrix(C transpose) and store in results; 
std::vector<std::vector<double>> DCT(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(8,8);
    auto temp = create_2d_vector<double>(8,8);
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                temp.at(i).at(j) += (data.at(i).at(x)*c.at(x).at(j));
            }
        }
    }
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                results.at(i).at(j) += (temp.at(i).at(x)*c.at(j).at(x)); //for c transpose
            }
        }
    }
    return results;
}

//Quantized values are rounded and clamped to the closest char within the (0,255) range
std::vector<std::vector<int>> Quantized(std::vector<std::vector<double>> data){
    auto results = create_2d_vector<int>(8,8);
    for(int i = 0; i<8; i++){
        for(int j = 0; j <8; j++){
            results.at(i).at(j) = round((data.at(i).at(j)/Q_1.at(i).at(j)));
        }
    }
    return results;
}

//Function for navigating through the matrices and outputs the results as the encoding order.
std::vector<int> E_O(std::vector<std::vector<int>> data){
    std::vector<int> results;
    int count = 0;
    int reverse = 1;
    while(count <= 14){
        int i = 0;
        int j = 0;
        if(count < 8){
            if((count % 2) == 0){
                i = count;
                j = 0;
                for(int k = 0; k <= count; k++){
                    results.push_back(data.at(i).at(j));
                    i-=1;
                    j+=1;
                }
            }else{
                i = 0;
                j = count;
                for(int k = 0; k <= count; k++){
                    results.push_back(data.at(i).at(j));
                    i+=1;
                    j-=1;
                }
            }
        }
        else{
            if((count % 2) == 0){
                i = 7;
                j = count-7;
                for(int k = 0; k < (count-reverse); k++){
                    results.push_back(data.at(i).at(j));
                    i-=1;
                    j+=1;
                }
            }else{
                i = count - 7;
                j = 7;
                for(int k = 0; k <(count-reverse); k++){
                    results.push_back(data.at(i).at(j));
                    i+=1;
                    j-=1;
                }
            }
            reverse += 2;
        }
        count++;
    }
    return results;
}

void rle(std::vector<int> data, OutputBitStream stream){
    int size = data.size();
    for(int i = 0; i<size; i++){
        unsigned char output = data.at(i);
        stream.push_byte(output);
        int j = i+1;
        int count = 0;
        while(j < size){
            if(data.at(j) == data.at(i)){
                count++;
                j++;
            }else{
                break;
            }
        }
        if(count == 0){
            stream.push_bit(0);
        }else{
            std::vector<int> length_bits;
            int temp = count;
            int num_bits = floor(log2((double)count)) + 1;
            for(int k = 0; k< num_bits; k++){
                stream.push_bit(1);
            }
            stream.push_bit(0);
            for(int k = 1; k<num_bits; k++){
                length_bits.push_back((temp%2));
                if(temp%2 == 1){
                    temp-= 1;
                }
                temp = temp/2;
            }
            int length_size = length_bits.size();
            for(int x = length_size-1; x>= 0; x--){
                stream.push_bit(length_bits.at(x));
            }
            i += count;
        }
    }
}


void blocks(std::vector<std::vector<int>> data, int height, int width, std::vector<std::vector<double>> c, OutputBitStream stream){
    int recorded_x = 0;
    int recorded_y = 0;
    while(true){
        auto temporary = create_2d_vector<int>(8,8);
        std::vector<int> prev_row;
        int prev = -1;
        for(int i = 0; i<8; i++){
            for(int j = 0; j < 8; j++){
                if(recorded_y+i < height){
                    if(recorded_x+j < width){
                        temporary.at(i).at(j) = data.at(recorded_y+i).at(recorded_x+j);
                        prev = temporary.at(i).at(j);
                        if(prev_row.size() == 8){
                            prev_row.clear();
                        }
                        prev_row.push_back(temporary.at(i).at(j));
                    }else{
                        if(prev != -1){
                            temporary.at(i).at(j) = prev;
                            prev_row.push_back(prev);
                        }
                    }
                }else{
                    for(int k = 0; k< 8; k++){
                        temporary.at(i).at(k) = prev_row.at(k);
                    }
                }
            }
        }
        std::vector<std::vector<double>> dct_block = DCT(temporary, c);
        std::vector<std::vector<int>> quantum = Quantized(dct_block);
        std::vector<int> encoding = E_O(quantum); //Should be organized in encoding order now for RLE and DC
        rle(encoding, stream);
        if(recorded_x+8 >=width){
            if(recorded_y+8 >=height){
                break;
            }else{
                recorded_x = 0;
                recorded_y += 8;
            }
        }else{
            recorded_x += 8;
        }
    }
}

//A simple downscaling algorithm using averaging.
std::vector<std::vector<int> > scale_down(std::vector<std::vector<unsigned char> > source_image, unsigned int source_width, unsigned int source_height, int factor){

    unsigned int scaled_height = (source_height+factor-1)/factor;
    unsigned int scaled_width = (source_width+factor-1)/factor;

    //Note that create_2d_vector automatically initializes the array to all-zero
    auto sums = create_2d_vector<unsigned int>(scaled_height,scaled_width);
    auto counts = create_2d_vector<unsigned int>(scaled_height,scaled_width);

    for(unsigned int y = 0; y < source_height; y++)
        for (unsigned int x = 0; x < source_width; x++){
            sums.at(y/factor).at(x/factor) += source_image.at(y).at(x);
            counts.at(y/factor).at(x/factor)++;
        }

    auto result = create_2d_vector<int>(scaled_height,scaled_width);
    for(unsigned int y = 0; y < scaled_height; y++)
        for (unsigned int x = 0; x < scaled_width; x++)
            result.at(y).at(x) = (unsigned char)((sums.at(y).at(x)+0.5)/counts.at(y).at(x));
    return result;
}


int main(int argc, char** argv){

    if (argc < 4){
        std::cerr << "Usage: " << argv[0] << " <low/medium/high> <input BMP> <output file>" << std::endl;
        return 1;
    }
    std::string quality{argv[1]};
    std::string input_filename {argv[2]};
    std::string output_filename {argv[3]};

    bitmap_image input_image {input_filename};

    unsigned int height = input_image.height();
    unsigned int width = input_image.width();

    //Read the entire image into a 2d array of PixelRGB objects 
    //(Notice that height is the outer dimension, so the pixel at coordinates (x,y) 
    // must be accessed as imageRGB.at(y).at(x)).
    std::vector<std::vector<PixelYCbCr>> imageYCbCr = create_2d_vector<PixelYCbCr>(height,width);
    for(unsigned int y = 0; y < height; y++){
        for (unsigned int x = 0; x < width; x++){
            auto [r,g,b] = input_image.get_pixel(x,y);
            PixelRGB rgb_pixel {r,g,b};
            imageYCbCr.at(y).at(x) = rgb_pixel.to_ycbcr();            
        }
    }
    std::vector<std::vector<double>> coeff = Coeff();
    std::ofstream output_file{output_filename,std::ios::binary};
    OutputBitStream output_stream {output_file};

    //Placeholder: Use a simple bitstream containing the height/width (in 32 bits each)
    //followed by the entire set of values in each colour plane (in row major order).

    output_stream.push_u32(height);
    output_stream.push_u32(width);

    //Write the Y values 
    auto y_val = create_2d_vector<int>(height, width);
    for(unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++){
            y_val.at(y).at(x) = imageYCbCr.at(y).at(x).Y;
        }
    blocks(y_val, height, width, coeff, output_stream);
    //Extract the Cb plane into its own array 
    auto Cb = create_2d_vector<unsigned char>(height,width);
    for(unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++)
            Cb.at(y).at(x) = imageYCbCr.at(y).at(x).Cb;
    auto Cb_scaled = scale_down(Cb,width,height,2);

    //Extract the Cr plane into its own array 
    auto Cr = create_2d_vector<unsigned char>(height,width);
    for(unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++)
            Cr.at(y).at(x) = imageYCbCr.at(y).at(x).Cr;
    auto Cr_scaled = scale_down(Cr,width,height,2);
    
    //In decompressor keep a running total of the bits read for Y, cb, cr
    //In decompressor with first 64 bits are size:
    //Count for Y: ceil(width/8)*ceil(height/8)*64
    //Count for Cb, Cr: ceil(((width+1)/2)/8)*ceil(((height+1)/2)/8)*64
    //In the decoder for we will keep count so that we know when we have iterated through an entire block and moved to the next/finished.
    //Write the Cb values 
    blocks(Cb_scaled, (height+1)/2, (width+1)/2, coeff, output_stream);

    //Write the Cr values 
    blocks(Cr_scaled, (height+1)/2, (width+1)/2, coeff, output_stream);

    output_stream.flush_to_byte();
    output_file.close();

    return 0;
}