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

//Make into matrix so that it only needs to be computed once instead of multiple computations and function calls.
std::vector<std::vector<double>> Coeff(){
    auto results = create_2d_vector<double>(8,8);
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            if(i = 0){
                results.at(i).at(j) = sqrt((1/8));
            }else{
                results.at(i).at(j) = (sqrt((1/4))*cos((((2*j)+1)*i*M_PI)/16));
            }
        }
    }
    return results;
}

//Compute DCT(A) = matrix(C)*matrix(A)*matrix(C transpose) and store in results; 
void DCT(std::vector<int> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(8,8);
    auto temp = create_2d_vector<double>(8,8);
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                temp.at(i).at(j) += data.at(i).at(x)*c.at(x).at(j);
            }
        }
    }
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                results.at(i).at(j) += temp.at(i).at(x)*c.at(j).at(x);
            }
        }
    }
}

void blocks(std::vector<std::vector<PixelYCbCr>> data, int height, int width){
    int recorded_x = 0;
    int recorded_y = 0;
    while(true){
        auto temporary = create_2d_vector<int>(8,8);
        std::vector<int> prev_row;
        int prev = -1;
        for(int i = 0; i<8; i++){
            for(int j = 0; j < 8; j++){
                if(recorded_y+i <=height){
                    if(recorded_x+j <= width){
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
                        else{
                            exit(1); //error no data
                        }
                    }
                }else{
                    for(int k = 0; k< 8; k++){
                        temporary.at(i).at(k) = prev_row.at(k);
                    }
                }
            }
        }
        //block matrix now available as temporary
        if(recorded_x+8 > width){
            if(recorded_y+8 > height){
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
std::vector<std::vector<unsigned char> > scale_down(std::vector<std::vector<unsigned char> > source_image, unsigned int source_width, unsigned int source_height, int factor){

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

    auto result = create_2d_vector<unsigned char>(scaled_height,scaled_width);
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

    std::ofstream output_file{output_filename,std::ios::binary};
    OutputBitStream output_stream {output_file};

    //Placeholder: Use a simple bitstream containing the height/width (in 32 bits each)
    //followed by the entire set of values in each colour plane (in row major order).

    output_stream.push_u32(height);
    output_stream.push_u32(width);

    //Write the Y values 
     auto y_val = create_2d_vector<unsigned char>(height, width);
    for(unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++)
            y_val.at(y).at(x) - imageYCbCr.at(y).at(x).Y;
            output_stream.push_byte(imageYCbCr.at(y).at(x).Y);

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

    //Write the Cb values 
    for(unsigned int y = 0; y < (height+1)/2; y++)
        for (unsigned int x = 0; x < (width+1)/2; x++)
            output_stream.push_byte(Cb_scaled.at(y).at(x));

    //Write the Cr values 
    for(unsigned int y = 0; y < (height+1)/2; y++)
        for (unsigned int x = 0; x < (width+1)/2; x++)
            output_stream.push_byte(Cr_scaled.at(y).at(x));


    output_stream.flush_to_byte();
    output_file.close();

    return 0;
}