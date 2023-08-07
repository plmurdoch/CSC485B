/* uvid_compress.cpp
   CSC 485B/578B - Data Compression - Summer 2023

   Starter code for Assignment 4

   Reads video data from stdin in uncompresed YCbCr (YUV) format 
   (With 4:2:0 subsampling). To produce this format from 
   arbitrary video data in a popular format, use the ffmpeg
   tool and a command like 

     ffmpeg -i videofile.mp4 -f rawvideo -pixel_format yuv420p - 2>/dev/null | ./this_program <width> height>

   Note that since the width/height of each frame is not encoded into the raw
   video stream, those values must be provided to the program as arguments.

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
#include "output_stream.hpp"
#include "yuv_stream.hpp"

//Convenience function to wrap around the nasty notation for 2d vectors
template<typename T>
std::vector<std::vector<T> > create_2d_vector(unsigned int outer, unsigned int inner){
    std::vector<std::vector<T> > V {outer, std::vector<T>(inner,T() )};
    return V;
}

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

//RLE encoding using basic implementation outputing num:length pairs with each entry of the pair utilizing 8 bits.
//Will be improved in Assignment 4, however, my other implementation for Variable length RLE was not working properly.
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
            stream.push_byte((unsigned char)0);
        }else{
            stream.push_byte((unsigned char)count);
            i += count;
        }
    }
}

std::vector<int> P_or_IFrame(std::vector<int> P, std::vector<int> I, OutputBitStream stream){
    std::vector<int> result;
    int count_p = 0;
    int count_i = 0;
    int run_p = 0;
    int run_i = 0;
    for(int i = 0; i<64; i++){
        if(i == 0){
            run_p = P.at(i);
            run_i = I.at(i);
        }else{
            if(run_i == I.at(i)){
                count_i++;
            }else{
                run_i = I.at(i);
            }
            if(run_p == P.at(i)){
                count_p++;
            }else{
                run_p = P.at(i);
            }
        }
    }
    if(count_p >count_i){
        result = P;
        stream.push_byte(1);
    }else{
        result = I;
        stream.push_byte(0);
    }
    return result;
}
//Main body of code, blocks takes the input and parses them into 8x8 blocks for the dct and quantization operations to occur.
//In this function the block is placed into encoding order and then RLE is done before the block is streamed into the file.
void blocks(std::vector<std::vector<int>> data, std::vector<std::vector<int>>previous, int height, int width, std::vector<std::vector<double>> c, OutputBitStream stream, std::vector<std::vector<int>> Q, std::string quality){
    int recorded_x = 0;
    int recorded_y = 0;
    while(true){
        auto temporary = create_2d_vector<int>(8,8);
        auto previous_temp = create_2d_vector<int>(8,8);
        std::vector<int> prev_row;
        std::vector<int> previous_p_row;
        int prev = -1;
        int previous_p = -1;
        for(int i = 0; i<8; i++){
            for(int j = 0; j < 8; j++){
                if(recorded_y+i < height){
                    if(recorded_x+j < width){
                        temporary.at(i).at(j) = data.at(recorded_y+i).at(recorded_x+j);
                        previous_temp.at(i).at(j) = data.at(recorded_y+i).at(recorded_x+j)-previous.at(recorded_y+i).at(recorded_x+j);
                        prev = temporary.at(i).at(j);
                        previous_p = previous_temp.at(i).at(j);
                        if(prev_row.size() == 8){
                            prev_row.clear();
                            previous_p_row.clear();
                        }
                        prev_row.push_back(temporary.at(i).at(j));
                        previous_p_row.push_back(previous_temp.at(i).at(j));
                    }else{
                        if(prev != -1){
                            temporary.at(i).at(j) = prev;
                            previous_temp.at(i).at(j) = previous_p;
                            prev_row.push_back(prev);
                            previous_p_row.push_back(previous_p);
                        }
                    }
                }else{
                    for(int k = 0; k< 8; k++){
                        temporary.at(i).at(k) = prev_row.at(k);
                        previous_temp.at(i).at(k) = previous_p_row.at(k);
                    }
                }
            }
        }
        auto prev_dct_block = create_2d_vector<double>(8,8);
        auto dct_block = create_2d_vector<double>(8,8);
        if(quality=="high"){
            dct_block = DCT_high(temporary, c);
            prev_dct_block = DCT_high(previous_temp,c);
        }else{
            dct_block = DCT_low(temporary, c);
            prev_dct_block = DCT_low(previous_temp,c);
        }
        std::vector<std::vector<int>> quantum = Quantized(dct_block, Q);
        std::vector<std::vector<int>> quantum_p = Quantized(prev_dct_block, Q);
        std::vector<int> encoding = E_O(quantum);
        std::vector<int> encoding_p = E_O(quantum_p);
        std::vector<int> better = P_or_IFrame(encoding_p, encoding, stream);
        rle(better, stream);
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

int main(int argc, char** argv){

    if (argc < 4){
        std::cerr << "Usage: " << argv[0] << " <width> <height> <low/medium/high>" << std::endl;
        return 1;
    }
    u32 width = std::stoi(argv[1]);
    u32 height = std::stoi(argv[2]);
    std::string quality{argv[3]};

    YUVStreamReader reader {std::cin, width, height};
    OutputBitStream output_stream {std::cout};

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
    auto y_p = create_2d_vector<int>(height, width);
    auto cb_p = create_2d_vector<int>((height/2),(width/2));
    auto cr_p = create_2d_vector<int>((height/2),(width/2));
    while (reader.read_next_frame()){
        output_stream.push_byte(1); //Use a one byte flag to indicate whether there is a frame here
        YUVFrame420& frame = reader.frame();
        auto y_val = create_2d_vector<int>(height, width);
        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                y_val.at(y).at(x) = frame.Y(x,y);
        blocks(y_val, y_p, height, width, coeff, output_stream, Q, quality);
        y_p = y_val;
        auto Cb = create_2d_vector<int>((height/2),(width/2));
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                Cb.at(y).at(x) = frame.Cb(x,y);
        blocks(Cb, cb_p, ((height)/2), ((width)/2), coeff, output_stream, Q, quality);
        cb_p = Cb;
        auto Cr = create_2d_vector<int>((height/2),(width/2));
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                Cr.at(y).at(x) = frame.Cr(x,y);
        blocks(Cr, cr_p, ((height)/2), ((width)/2), coeff, output_stream, Q, quality);
        cr_p = Cr;
    }

    output_stream.push_byte(0); //Flag to indicate end of data
    output_stream.flush_to_byte();

    return 0;
}