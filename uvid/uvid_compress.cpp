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
Therefore to compensate for this we have DCT_low for medium and low values and DCT_high for the high setting values.  
*/
//Compute DCT(A) = matrix(C)*matrix(A)*matrix(C transpose) and store in results; 
std::vector<std::vector<double>> DCT_low(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(16,16);
    auto temp = create_2d_vector<double>(16,16);
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                temp.at(i).at(j) += (data.at(x).at(j)*c.at(i).at(x));
            }
        }
    }
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                results.at(i).at(j) += (temp.at(i).at(x)*c.at(j).at(x)); //for c transpose
            }
        }
    }
    return results;
}

std::vector<std::vector<double>> DCT_high(std::vector<std::vector<int>> data, std::vector<std::vector<double>> c){
    auto results = create_2d_vector<double>(16,16);
    auto temp = create_2d_vector<double>(16,16);
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                temp.at(i).at(j) += (data.at(i).at(x)*c.at(x).at(j));
            }
        }
    }
    for(int i = 0; i< 16; i++){
        for(int j = 0; j<16; j++){
            for(int x = 0; x<16; x++){
                results.at(i).at(j) += (temp.at(i).at(x)*c.at(j).at(x)); //for c transpose
            }
        }
    }
    return results;
}

//Quantized values are rounded and clamped within the (-127,127) range so that they can be output as bytes in 0-255 range.
std::vector<std::vector<int>> Quantized(std::vector<std::vector<double>> data, std::vector<std::vector<int>> Q){
    auto results = create_2d_vector<int>(16,16);
    for(int i = 0; i<16; i++){
        for(int j = 0; j <16; j++){
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
    while(count <= 30){
        int i = 0;
        int j = 0;
        if(count < 16){
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
                i = 15;
                j = count-15;
                for(int k = 0; k < (count-reverse); k++){
                    results.push_back(data.at(i).at(j));
                    i-=1;
                    j+=1;
                }
            }else{
                i = count - 15;
                j = 15;
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
void rle(std::vector<int> data, OutputBitStream stream, int frame, std::pair<int,int> x_y){
    int size = data.size();
    std::vector<int> buffer;
    if(frame == 2){
        buffer.push_back(1);
        buffer.push_back(0);
        int x = x_y.first;
        std::cerr<<x<<":";
        int y = x_y.second;
        std::cerr<<y<<std::endl;
        std::vector<int> temp_x;
        std::vector<int> temp_y;
        for(int j = 0; j<7; j++){
            temp_x.push_back((x%2));
            temp_y.push_back((y%2));
            x = floor(x/2);
            y=floor(y/2);
        }
        for(int j =6; j>=0; j--){
            buffer.push_back(temp_x.at(j));
            if(buffer.size() == 8){
                Offload(buffer, stream);
                buffer.clear();
            }
        }
        for(int j =6; j>=0; j--){
            buffer.push_back(temp_y.at(j));
            if(buffer.size() == 8){
                Offload(buffer, stream);
                buffer.clear();
            }
        }
    }else if(frame == 1){
        buffer.push_back(0);
        buffer.push_back(1);
    }else{
        buffer.push_back(0);
        buffer.push_back(0);
    }
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

std::pair<std::vector<int>,int> P_or_IFrame(std::vector<int> V,std::vector<int> P, std::vector<int> I){
    std::vector<int> result;
    int count_p = 0;
    int count_i = 0;
    int count_v = 0;
    int run_p = 0;
    int run_i = 0;
    int run_v = 0;
    for(int i = 0; i<256; i++){
        if(i == 0){
            run_p = P.at(i);
            run_i = I.at(i);
            run_v = V.at(i);
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
            if(run_v == V.at(i)){
                count_v++;
            }else{
                run_v = V.at(i);
            }
        }
    }
    if(count_p >count_i){
        result = P;
        return std::make_pair(result,1);
    /*}else if(count_v >count_i && count_v >count_p){
        result = V;
        return std::make_pair(result, 2);*/
    }else{
        result = I;
        return std::make_pair(result, 0);
    }
}
/*
For P-frame calculations 
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

std::pair<std::vector<std::vector<int>>,std::pair<int,int>> MotionVector(int x_val, int y_val, int height, int width, std::vector<std::vector<int>> previous, std::vector<std::vector<int>> data){
    int starting_x = 0;
    if(x_val < 8){
        starting_x = 0;
    }else if(x_val >(width-72)){
        starting_x = (width-80);
    }else{
        starting_x = (x_val-8);
    }
    int starting_y = 0;
    if(y_val < 8){
        starting_y = 0;
    }else if(y_val >(height-72)){
        starting_y = (height-80);
    }else{
        starting_y = y_val-8;
    }
    int min_x = -1;
    int min_y = -1;
    double MSD = -1;
    auto result = create_2d_vector<int>(16,16);
    while(true){
        int local_sum = 0;
        auto temp = create_2d_vector<int>(16,16);
        for(int i = 0; i<16; i++){
            for(int j = 0; j < 16; j++){
                local_sum+= pow(data.at(i).at(j) - previous.at(starting_y+i).at(starting_x+j),2);
                temp.at(i).at(j) = data.at(i).at(j) - previous.at(starting_y+i).at(starting_x+j);
            }
        }
        if(MSD == -1){
            MSD = (1/(pow(16,2)))*local_sum;
        }else{
            if(MSD > ((1/(pow(16,2)))*local_sum)){
                MSD = (1/(pow(16,2)))*local_sum;
                min_x = starting_x- x_val;
                min_y = starting_y - y_val; 
                result = temp;
            }
        }
        if(starting_x+16 >=80){
            if(starting_y+16 >=80){
                break;
            }else{
                starting_x = 0;
                starting_y += 16;
            }
        }else{
            starting_x += 16;
        }
    }
    return std::make_pair(result,std::make_pair((min_x), (min_y)));
}
//Main body of code, blocks takes the input and parses them into 8x8 blocks for the dct and quantization operations to occur.
//In this function the block is placed into encoding order and then RLE is done before the block is streamed into the file.
std::vector<std::vector<int>> blocks(std::vector<std::vector<int>> data, std::vector<std::vector<int>>previous, int height, int width, std::vector<std::vector<double>> c, OutputBitStream stream, std::vector<std::vector<int>> Q, std::string quality){
    int recorded_x = 0;
    int recorded_y = 0;
    auto encoded_data = create_2d_vector<int>(height, width); 
    while(true){
        auto temporary = create_2d_vector<int>(16,16);
        auto previous_temp = create_2d_vector<int>(16,16);
        std::vector<int> prev_row;
        std::vector<int> previous_p_row;
        int prev = -1;
        int previous_p = -1;
        for(int i = 0; i<16; i++){
            for(int j = 0; j < 16; j++){
                if(recorded_y+i < height){
                    if(recorded_x+j < width){
                        temporary.at(i).at(j) = data.at(recorded_y+i).at(recorded_x+j);
                        previous_temp.at(i).at(j) = data.at(recorded_y+i).at(recorded_x+j)-previous.at(recorded_y+i).at(recorded_x+j);
                        prev = temporary.at(i).at(j);
                        previous_p = previous_temp.at(i).at(j);
                        if(prev_row.size() == 16){
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
                    for(int k = 0; k< 16; k++){
                        temporary.at(i).at(k) = prev_row.at(k);
                        previous_temp.at(i).at(k) = previous_p_row.at(k);
                    }
                }
            }
        }
        auto motion_frames = create_2d_vector<int>(16,16);
        std::pair<std::vector<std::vector<int>>,std::pair<int,int>>motion = MotionVector(recorded_x, recorded_y, height, width, previous, temporary);
        motion_frames =motion.first;
        std::pair<int,int> x_y = motion.second;
        auto motion_dct_block = create_2d_vector<double>(16,16);
        auto prev_dct_block = create_2d_vector<double>(16,16);
        auto dct_block = create_2d_vector<double>(16,16);
        if(quality=="high"){
            dct_block = DCT_high(temporary, c);
            prev_dct_block = DCT_high(previous_temp,c);
            motion_dct_block = DCT_high(motion_frames, c);
        }else{
            dct_block = DCT_low(temporary, c);
            prev_dct_block = DCT_low(previous_temp,c);
            motion_dct_block = DCT_high(motion_frames, c);
        }
        std::vector<std::vector<int>> quantum = Quantized(dct_block, Q);
        std::vector<std::vector<int>> quantum_p = Quantized(prev_dct_block, Q);
        std::vector<std::vector<int>> quantum_v = Quantized(motion_dct_block, Q);
        auto for_prev = create_2d_vector<int>(16,16);
        for(int i = 0; i<16; i++){
            for(int j = 0; j< 16; j++){
                for_prev.at(i).at(j) = (quantum.at(i).at(j)-127)*Q.at(i).at(j);
            }
        }
        auto inverse_dct = create_2d_vector<double>(16,16);
        if(quality=="high"){
            inverse_dct = inverse_DCT_high(for_prev, c);
        }else{
            inverse_dct = inverse_DCT_low(for_prev, c);
        }
        for(int i = 0; i<16; i++){
            for(int j = 0; j< 16; j++){
                if(recorded_y+i < height){
                    if(recorded_x+j < width){
                        encoded_data.at(recorded_y+i).at(recorded_x+j) = round(inverse_dct.at(i).at(j));
                    }
                }
            }
        }
        std::vector<int> encoding = E_O(quantum);
        std::vector<int> encoding_p = E_O(quantum_p);
        std::vector<int> encoding_v = E_O(quantum_v);
        std::pair<std::vector<int>, int> pair = P_or_IFrame(encoding_v, encoding_p, encoding);
        rle(pair.first, stream, pair.second, x_y);
        if(recorded_x+16 >=width){
            if(recorded_y+16 >=height){
                break;
            }else{
                recorded_x = 0;
                recorded_y += 16;
            }
        }else{
            recorded_x += 16;
        }
    }
    return encoded_data;
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
    if(quality == "low"){
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = round(2*(Q.at(i).at(j))); 
            }
        }
        output_stream.push_byte((unsigned char)0);
        
    }else if(quality == "high"){
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = (round(0.2*Q.at(i).at(j)));
            }
        }
        output_stream.push_byte((unsigned char)2);
    }else{
        for(int i = 0; i<16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = ((Q.at(i).at(j)));     
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
        y_p = blocks(y_val, y_p, height, width, coeff, output_stream, Q, quality);
        auto Cb = create_2d_vector<int>((height/2),(width/2));
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                Cb.at(y).at(x) = frame.Cb(x,y);
        cb_p =blocks(Cb, cb_p, ((height)/2), ((width)/2), coeff, output_stream, Q, quality);
        auto Cr = create_2d_vector<int>((height/2),(width/2));
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                Cr.at(y).at(x) = frame.Cr(x,y);
        cr_p = blocks(Cr, cr_p, ((height)/2), ((width)/2), coeff, output_stream, Q, quality);
    }

    output_stream.push_byte(0); //Flag to indicate end of data
    output_stream.flush_to_byte();

    return 0;
}