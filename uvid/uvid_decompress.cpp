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
    {0,0},{0,1},{1,0},{2,0},{1,1},{0,2},{0,3},{1,2},{2,1},{3,0},{4,0},{3,1},{2,2},{1,3},{0,4},{0,5},
    {1,4},{2,3},{3,2},{4,1},{5,0},{6,0},{5,1},{4,2},{3,3},{2,4},{1,5},{0,6},{0,7},{1,6},{2,5},{3,4},
    {4,3},{5,2},{6,1},{7,0},{8,0},{7,1},{6,2},{5,3},{4,4},{3,5},{2,6},{1,7},{0,8},{0,9},{1,8},{2,7},
    {3,6},{4,5},{5,4},{6,3},{7,2},{8,1},{9,0},{10,0},{9,1},{8,2},{7,3},{6,4},{5,5},{4,6},{3,7},{2,8},
    {1,9},{0,10},{0,11},{1,10},{2,9},{3,8},{4,7},{5,6},{6,5},{7,4},{8,3},{9,2},{10,1},{11,0},{12,0},{11,1},
    {10,2},{9,3},{8,4},{7,5},{6,6},{5,7},{4,8},{3,9},{2,10},{1,11},{0,12},{0,13},{1,12},{2,11},{3,10},{4,9},
    {5,8},{6,7},{7,6},{8,5},{9,4},{10,3},{11,2},{12,1},{13,0},{14,0},{13,1},{12,2},{11,3},{10,4},{9,5},{8,6},
    {7,7},{6,8},{5,9},{4,10},{3,11},{2,12},{1,13},{0,14},{0,15},{1,14},{2,13},{3,12},{4,11},{5,10},{6,9},{7,8},
    {8,7},{9,6},{10,5},{11,4},{12,3},{13,2},{14,1},{15,0},{15,1},{14,2},{13,3},{12,4},{11,5},{10,6},{9,7},{8,8},
    {7,9},{6,10},{5,11},{4,12},{3,13},{2,14},{1,15},{2,15},{3,14},{4,13},{5,12},{6,11},{7,10},{8,9},{9,8},{10,7},
    {11,6},{12,5},{13,4},{14,3},{15,2},{15,3},{14,4},{13,5},{12,6},{11,7},{10,8},{9,9},{8,10},{7,11},{6,12},{5,13},
    {4,14},{3,15},{4,15},{5,14},{6,13},{7,12},{8,11},{9,10},{10,9},{11,8},{12,7},{13,6},{14,5},{15,4},{15,5},{14,6},
    {13,7},{12,8},{11,9},{10,10},{9,11},{8,12},{7,13},{6,14},{5,15},{6,15},{7,14},{8,13},{9,12},{10,11},{11,10},{12,9},
    {13,8},{14,7},{15,6},{15,7},{14,8},{13,9},{12,10},{11,11},{10,12},{9,13},{8,14},{7,15},{8,15},{9,14},{10,13},{11,12},
    {12,11},{13,10},{14,9},{15,8},{15,9},{14,10},{13,11},{12,12},{11,13},{10,14},{9,15},{10,15},{11,14},{12,13},{13,12},{14,11},
    {15,10},{15,11},{14,12},{13,13},{12,14},{11,15},{12,15},{13,14},{14,13},{15,12},{15,13},{14,14},{13,15},{14,15},{15,14},{15,15}
};

//Function for building the Coefficient matrix to utilize in quantization
std::vector<std::vector<double>> Coeff(){
    auto results = create_2d_vector<double>(16,16);
    for(int i = 0; i < 16; i++){
        for(int j = 0; j < 16; j++){
            if(i == 0){
                results.at(i).at(j) = sqrt(0.0625);
            }else{
                results.at(i).at(j) = (sqrt((0.125))*cos((((2.00*j)+1.00)*i*M_PI)/32.00));
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
    auto results = create_2d_vector<double>(16,16);
    auto temp = create_2d_vector<double>(16,16);
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                temp.at(i).at(j) += (data.at(x).at(j)*c.at(x).at(i));//for c transpose
            }
        }
    }
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                results.at(i).at(j) += (temp.at(i).at(x)*c.at(x).at(j)); 
            }
        }
    }
    return results;
}

std::vector<std::vector<double>> inverse_DCT_high(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(16,16);
    auto temp = create_2d_vector<double>(16,16);
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                temp.at(i).at(j) += (data.at(i).at(x)*c.at(j).at(x));//for c transpose
            }
        }
    }
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
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
    int first_bit = -1;
    int second_bit = -1;
    int x_val = -1;
    int y_val = -1;
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
            second_bit = first_bin.at(6);
            if(first_bit == 1){
                int count = 5;
                std::vector<int> x_store;
                while(count >=0){
                    x_store.push_back(first_bin.at(count));
                    count--;
                }
                int next_few = input.read_byte();
                std::vector<int> second_bin;
                for(int i = 0; i<8; i++){
                    second_bin.push_back(next_few%2);
                    next_few = floor(next_few/2);
                }
                count = 7;
                std::vector<int> y_store;
                while(count>= 2){
                    y_store.push_back(second_bin.at(count));
                    count--;
                }
                int x_number = 0;
                int y_number = 0;
                int exp = 0;
                for(int i = 5; i>= 0; i--){
                    x_number+= x_store.at(i)*pow(2,exp);
                    y_number+= y_store.at(i)*pow(2,exp);
                    exp++;
                }
                x_val = x_number-16;
                y_val = y_number-16;
                while(count >= 0){
                    num.push_back(second_bin.at(count));
                    count--;
                }
            }else{
                int count = 5;
                while(count >=0){
                    num.push_back(first_bin.at(count));
                    count--;
                }
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
        if(encoded_size == 256){
            auto DCT = create_2d_vector<int>(16,16);
            int E_O_size = E_O.size();
            for(int i = 0; i<E_O_size; i++){
                int row = E_O.at(i).first;
                int column = E_O.at(i).second;
                int for_DCT = encoded.at(i);
                DCT.at(row).at(column) = ((for_DCT-127)*Q.at(row).at(column));
            }
            encoded.clear();
            bit_buffer.clear();
            auto data = create_2d_vector<double>(16,16);
            if(quality != 2){
                data = inverse_DCT_low(DCT, C);
            }else{
                data = inverse_DCT_high(DCT, C);
            }
            for(int y = 0; y < 16; y++){
                for(int x = 0; x<16; x++){
                    if((y+recorded_y) < height){
                        if((x+recorded_x) < width){
                            int dat = 0;
                            if(first_bit == 0 && second_bit == 1){
                                dat = round(data.at(y).at(x)+previous.at(y+recorded_y).at(x+recorded_x));
                            }
                            else if(first_bit == 1 && second_bit == 0){
                                dat = round(data.at(y).at(x)+previous.at(y+recorded_y+y_val).at(x+recorded_x+x_val));
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
            if((recorded_x+16) < width){
                recorded_x += 16;
            }else{
                recorded_x = 0;
                recorded_y += 16;
            }
            first_bit = -1; 
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
        { 40,  51,  61,  66,  70,  61,  64,  73,  79,  82,  87,  89,  95,  98, 103, 104 },
        { 50,  64,  77,  83,  82,  64,  68,  81,  84,  87,  89,  93,  97, 100, 103, 104 },
        { 61,  76,  75,  82,  88,  71,  78,  82,  87,  90,  94,  97, 101, 104, 106, 107 },
        { 70,  82,  81,  87,  90,  77,  85,  89,  94,  98, 102, 104, 108, 110, 112, 113 },
        { 79,  87,  90,  94,  97,  82,  91,  95, 100, 103, 107, 109, 112, 114, 116, 117 },
        { 87,  95,  98, 102, 105,  87,  94, 100, 104, 109, 110, 114, 117, 119, 121, 122 },
        { 95, 103, 106, 109, 112,  95, 102, 108, 112, 117, 118, 122, 125, 127, 128, 129 },
        {102, 110, 113, 116, 119, 101, 108, 115, 119, 124, 125, 129, 132, 133, 135, 136 },
        {108, 116, 120, 123, 126, 108, 115, 122, 126, 131, 132, 136, 139, 141, 142, 143 },
        {113, 121, 125, 128, 131, 113, 120, 127, 131, 136, 137, 141, 144, 146, 147, 148 },
        {118, 126, 130, 133, 136, 118, 125, 132, 136, 141, 142, 146, 149, 151, 152, 153 },
        {122, 130, 134, 137, 140, 122, 129, 136, 140, 145, 146, 150, 153, 155, 156, 157 },
        {126, 134, 138, 141, 144, 126, 133, 140, 144, 149, 150, 154, 157, 159, 160, 161 },
        {129, 137, 141, 144, 147, 129, 136, 143, 147, 152, 153, 157, 160, 162, 163, 164 },
        {132, 140, 144, 147, 150, 132, 139, 146, 150, 155, 156, 160, 163, 165, 166, 167 },
        {135, 143, 147, 150, 153, 135, 142, 149, 153, 158, 159, 163, 166, 168, 169, 170 }
    };
    //the next for loops allow us to change the quality setting of our Quantum based on the user input.
    if(quality == 0){
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = (2*Q.at(i).at(j));
            }
        }
    }else if(quality == 2){
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = (round((0.2*Q.at(i).at(j)))); 
            }
        }
    }else{
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = ((Q.at(i).at(j)));    
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