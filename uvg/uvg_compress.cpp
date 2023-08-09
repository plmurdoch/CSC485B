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
/*
Assignment 3: Image Compression Code
Written By: Payton Murdoch
Student Number: V00904677
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
Therefore to compensate for this we have DCT_low for medium and low values and DCT_high for the high setting values.  
*/
//Compute DCT(A) = matrix(C)*matrix(A)*matrix(C transpose) and store in results; 
std::vector<std::vector<double>> DCT_low(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(8,8);
    auto temp = create_2d_vector<double>(8,8);
    for(int i = 0; i< 8; i++){
        for(int j = 0; j<8; j++){
            for(int x = 0; x<8; x++){
                temp.at(i).at(j) += (data.at(x).at(j)*c.at(i).at(x));
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
std::vector<std::vector<double>> DCT_high(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
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

//Quantized values are rounded and clamped within the (-127,127) range so that they can be output as bytes in 0-255 range.
std::vector<std::vector<int>> Quantized(std::vector<std::vector<double>> data, std::vector<std::vector<int>> Q){
    auto results = create_2d_vector<int>(8,8);
    for(int i = 0; i<8; i++){
        for(int j = 0; j <8; j++){
            int dat = round((data.at(i).at(j)/Q.at(i).at(j)));
            if(dat < -127){
                dat = 0;
            }else if(dat > 127){
                dat = 255;
            }else{
                dat = dat +127;
            }
            results.at(i).at(j) = dat;
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

void Offload(std::vector<int> to_output, OutputBitStream stream){
    int final_num = 0; 
    int count = 0;
    for(int i = 7; i>= 0; i--){
        final_num += to_output.at(i)*pow(2,count);
        count++;
    }
    unsigned char output = final_num;
    stream.push_byte(output);
}
//RLE encoding using basic implementation outputing num:length pairs with each entry of the pair utilizing 8 bits.
//Will be improved in Assignment 4, however, my other implementation for Variable length RLE was not working properly.
void rle(std::vector<int> data, OutputBitStream stream){
    int size = data.size();
    std::vector<int> buffer;
    buffer.push_back(1);
    for(int i = 0; i<size; i++){
        int output = data.at(i);
        std::vector<int> temp;
        for(int j = 0; j<8; j++){
            temp.push_back((output%2));
            output = floor(output/2);
        }
        for(int j =7; j>=0; j--){
            buffer.push_back(temp.at(j));
            if(buffer.size() == 8){
                Offload(buffer, stream);
                buffer.clear();
            }
        }
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
            buffer.push_back(0);
            if(buffer.size() == 8){
                Offload(buffer, stream);
                buffer.clear();
            }
        }else{
            int bits = ceil(log2(count+1));
            for(int j = 0; j<bits; j++){
                buffer.push_back(1);
                if(buffer.size() == 8){
                    Offload(buffer, stream);
                    buffer.clear();
                }
            }
            buffer.push_back(0);
            if(buffer.size() == 8){
                Offload(buffer, stream);
                buffer.clear();
            }
            std::vector<int> temp;
            int temp_count = count;
            for(int j =0; j<bits-1; j++){
                temp.push_back(temp_count%2);
                temp_count = floor(temp_count/2);
            }
            for(int j = bits-2; j>= 0; j--){
                buffer.push_back(temp.at(j));
                if(buffer.size() == 8){
                    Offload(buffer, stream);
                    buffer.clear();
                }
            }
            i += count;
        }
    }
    if(buffer.size() != 0){
        while(buffer.size()<8){
            buffer.push_back(0);
        }
        Offload(buffer,stream);
    }
}

//Main body of code, blocks takes the input and parses them into 8x8 blocks for the dct and quantization operations to occur.
//In this function the block is placed into encoding order and then RLE is done before the block is streamed into the file.
void blocks(std::vector<std::vector<int>> data, int height, int width, std::vector<std::vector<double>> c, OutputBitStream stream, std::vector<std::vector<int>> Q, std::string quality){
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
        auto dct_block = create_2d_vector<double>(8,8);
        if(quality=="high"){
            dct_block = DCT_high(temporary, c);
        }else{
            dct_block = DCT_low(temporary, c);
        }
        std::vector<std::vector<int>> quantum = Quantized(dct_block, Q);
        std::vector<int> encoding = E_O(quantum);
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
    std::ofstream output_file{output_filename,std::ios::binary};
    OutputBitStream output_stream {output_file};
    //the next for loops allow us to change the quality setting of our Quantum based on the user input.
    if(quality == "low"){
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q.at(i).at(j) = round(2*(Q.at(i).at(j))); 
            }
        }
        output_stream.push_byte((unsigned char)0);
        
    }else if(quality == "high"){
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q.at(i).at(j) = (round(0.2*Q.at(i).at(j)));
            }
        }
        output_stream.push_byte((unsigned char)2);
    }else{
        for(int i = 0; i< 8; i++){
            for(int j = 0; j<8; j++){
                Q.at(i).at(j) = ((Q.at(i).at(j)+8));     
            }
        }
        output_stream.push_byte((unsigned char)1);
    }
    output_stream.push_u32(height);
    output_stream.push_u32(width);

    //Write the Y values 
    auto y_val = create_2d_vector<int>(height, width);
    for(unsigned int y = 0; y < height; y++)
        for (unsigned int x = 0; x < width; x++){
            y_val.at(y).at(x) = imageYCbCr.at(y).at(x).Y;
        }
    blocks(y_val, height, width, coeff, output_stream, Q,quality);
    
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
    blocks(Cb_scaled, ((height+1)/2), ((width+1)/2), coeff, output_stream, Q, quality);

    //Write the Cr values 
    blocks(Cr_scaled, ((height+1)/2), ((width+1)/2), coeff, output_stream, Q, quality);

    output_stream.flush_to_byte();
    output_file.close();

    return 0;
}