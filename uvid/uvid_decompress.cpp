/* uvid_decompress.cpp
   CSC 485B/578B - Data Compression - Summer 2023

   Starter code for Assignment 4
   
   This placeholder code reads the (basically uncompressed) data produced by
   the uvid_compress starter code and outputs it in the uncompressed 
   YCbCr (YUV) format used for the sample video input files. To play the 
   the decompressed data stream directly, you can pipe the output of this
   program to the ffplay program, with a command like 

     ffplay -f rawvideo -pixel_format yuv420p -framerate 30 -video_size 352x288 - 2>/dev/null
   (where the resolution is explicitly given as an argument to ffplay).

   B. Bird - 2023-07-08
*/

#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <cassert>
#include <cstdint>
#include <tuple>
#include <cmath>
#include "input_stream.hpp"
#include "yuv_stream.hpp"

//Convenience function to wrap around the nasty notation for 2d vectors
template<typename T>
std::vector<std::vector<T> > create_2d_vector(unsigned int outer, unsigned int inner){
    std::vector<std::vector<T> > V {outer, std::vector<T>(inner,T() )};
    return V;
}

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
std::vector<std::vector<unsigned char>> read_input(InputBitStream input, std::vector<std::vector<unsigned char>> previous, std::vector<std::vector<int>> Q, std::vector<std::vector<double>> C, int height, int width, int quality){
    auto result = create_2d_vector<unsigned char>(height, width);
    int counter = 0;
    std::vector<unsigned char> encoded;
    int recorded_y = 0;
    int recorded_x = 0;
    int prev = input.read_byte();
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
            if(quality != 2){
                data = inverse_DCT_low(DCT, C);
            }else{
                data = inverse_DCT_high(DCT, C);
            }
            for(int y = 0; y < 8; y++){
                for(int x = 0; x<8; x++){
                    if((y+recorded_y) < height){
                        if((x+recorded_x) < width){
                            int dat = 0;
                            if(prev == 1){
                                dat = round(data.at(y).at(x)+previous.at(y+recorded_y).at(x+recorded_x));
                            }
                            else{
                                dat = round(data.at(y).at(x));
                            }
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
            prev = input.read_byte();
        }
    }
}


int main(int argc, char** argv){

    //Note: This program must not take any command line arguments. (Anything
    //      it needs to know about the data must be encoded into the bitstream)
    
    InputBitStream input_stream {std::cin};
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
    u32 height {input_stream.read_u32()};
    u32 width {input_stream.read_u32()};
    YUVStreamWriter writer {std::cout, width, height};
    auto y_p = create_2d_vector<unsigned char>(height,width);
    auto cb_p = create_2d_vector<unsigned char>((height)/2,(width)/2);
    auto cr_p = create_2d_vector<unsigned char>((height)/2,(width)/2);
    while (input_stream.read_byte()){
        auto Y = create_2d_vector<unsigned char>(height,width);
        auto Cb = create_2d_vector<unsigned char>((height)/2,(width)/2);
        auto Cr = create_2d_vector<unsigned char>((height)/2,(width)/2);
        //Build Y matrix.
        Y = read_input(input_stream,y_p, Q, coeff,height, width, quality);
        y_p = Y;
        //Build Cb matrix.
        Cb = read_input(input_stream,cb_p, Q, coeff,((height)/2),((width)/2), quality);
        cb_p = Cb;
        //Build Cr matrix.
        Cr = read_input(input_stream, cr_p, Q, coeff,((height)/2),((width)/2), quality);
        cr_p = Cr;
        YUVFrame420& frame = writer.frame();
        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                frame.Y(x,y) = Y.at(y).at(x);
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                frame.Cb(x,y) = Cb.at(y).at(x);
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                frame.Cr(x,y) = Cr.at(y).at(x);
        writer.write_frame();
    }


    return 0;
}