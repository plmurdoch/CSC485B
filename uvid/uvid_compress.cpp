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

   Code written by: Payton Larsen Murdoch, V00904677
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
//This template function was taken from Assignment 3 starter code for CSC 485B, composed by Bill Bird.
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

//Quantized values are rounded and attemped to be clamped within the (-127,127) range so that they can be output as bytes in 0-255 range.
std::pair<std::vector<std::vector<int>>,int> Quantized(std::vector<std::vector<double>> data, std::vector<std::vector<int>> Q){
    auto results = create_2d_vector<int>(16,16);
    int oob = 0;
    for(int i = 0; i<16; i++){
        for(int j = 0; j <16; j++){
            int dat = round((data.at(i).at(j)/Q.at(i).at(j)));
            if(dat < -127){
                dat = 0;
                oob = 1;
            }else if(dat > 127){
                dat = 255;
                oob = 1;
            }else{
                dat = dat +127;
            }
            results.at(i).at(j) = dat;
        }
    }
    return std::make_pair(results, oob);
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

//Bits are loaded into an output buffer in sequential order then offloaded as a byte value in this function.
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

//This function named RLE first sends the block type as overhead, either it is computed as an I-frame, code 00, P-frame, code 01 or P-frame motion vector, code 10.
//I and P frames have no more heading, however, the next 6 bits of the motion vector block contains the motion vector codes for x and y.
//000 is -16, 001 is -8, 010 is 0, 011 is 8, 100 is 16. 
//Then the funciton packetizes the encoding ordered data into either 3 shortened codes if -1, 0 or 1 (or 126,127,128) as 0, 10, 110,
//or they are stored as the standard bytes with a 111 prefix and thus expanded. This reduces the file size drastically on medium and low settings.
//The function then uses Unary Variable RLE encodings following the codes, as encoding blocks contain 256 values this can show significant improvement.
void rle(std::vector<int> data, OutputBitStream stream, int frame, std::pair<int,int> x_y){
    int size = data.size();
    std::vector<int> shortened = {127,128, 126};
    std::vector<int> buffer;
    if(frame == 2){
        buffer.push_back(1);
        buffer.push_back(0);
        int x = 0;
        int y = 0;
        std::vector<int> order {-16,-8,0,8,16};
        for(int i = 0; i <5; i++){
            if(x_y.first == order.at(i)){
                x = i;
            }
            if(x_y.second == order.at(i)){
                y = i;
            }
        }
        std::vector<int> temp_x;
        std::vector<int> temp_y;
        for(int j = 0; j<3; j++){
            temp_x.push_back((x%2));
            temp_y.push_back((y%2));
            x = floor(x/2);
            y=floor(y/2);
        }
        for(int j =2; j>=0; j--){
            buffer.push_back(temp_x.at(j));
            if(buffer.size() == 8){
                Offload(buffer, stream);
                buffer.clear();
            }
        }
        for(int j =2; j>=0; j--){
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
        int shorten = -1;
        for(int j = 0; j<3; j++){
            if(shortened.at(j) == output){
                shorten = j;
                break;
            }
        }
        if(shorten != -1){
            if(shorten == 0){
                buffer.push_back(0);
                if(buffer.size() == 8){
                    Offload(buffer, stream);
                    buffer.clear();
                }
            }else if(shorten == 1){
                int temp = 2; 
                std::vector<int> temp_v;
                for(int i = 0; i<2; i++){
                    temp_v.push_back(temp%2);
                    temp = floor(temp/2);
                }
                for(int i = 1; i>=0; i--){
                    buffer.push_back(temp_v.at(i));
                    if(buffer.size() == 8){
                        Offload(buffer, stream);
                        buffer.clear();
                    }
                }
            }else{
                int temp = 6; 
                std::vector<int> temp_v;
                for(int i = 0; i<3; i++){
                    temp_v.push_back(temp%2);
                    temp = floor(temp/2);
                }
                for(int i = 2; i>=0; i--){
                    buffer.push_back(temp_v.at(i));
                    if(buffer.size() == 8){
                        Offload(buffer, stream);
                        buffer.clear();
                    }
                }
            }
        }else{
            for(int k = 0; k <3; k++){
                buffer.push_back(1);
                if(buffer.size() == 8){
                    Offload(buffer, stream);
                    buffer.clear();
                }
            }
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

//P_or_IFrame function examines blocks of P frames, I frames or a P frame using a motion vector and determines what would have the longest runs in the given setting.
//The block with the longest runs will take advantage of RLE reduction the most. If there are multiple blocks with the same run lengths, prioritize I-frames. 
//There is also an out of bounds restriction on P frames and motion vectors as they would yield data that skews the frame too much.
std::pair<std::vector<int>,int> P_or_IFrame(std::vector<int> V,std::vector<int> P, std::vector<int> I, std::pair<int,int> x_y, std::pair<int,int>oob_p_v){
    std::vector<int> result;
    int count_p = 0;
    int count_i = 0;
    int count_v = 0;
    int run_p = 0;
    int run_i = 0;
    int run_v = 0;
    if(x_y.first != 0 && x_y.second != 0){
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
        if(count_p >count_i && count_p >count_v && oob_p_v.first == 0){
            result = P;
            return std::make_pair(result,1);
        }else if(count_v >count_i && count_v >count_p && oob_p_v.second == 0){
            result = V;
            return std::make_pair(result, 2);
        }else{
            result = I;
            return std::make_pair(result, 0);
        }
    }else{
        for(int i = 0; i<256; i++){
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
        if(count_p >count_i && oob_p_v.first == 0){
            result = P;
            return std::make_pair(result,1);
        }else{
            result = I;
            return std::make_pair(result, 0);
        }
    }
    
}
//To minimize culminating errors we utilized decompressed frame values for next P-frame calculations, therefore we have inverse DCT functions here 
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

//Function for determining Motion Vectors. A local search occurs surrounding the current block in the P-frame and this remains within the bounds of the frame.
//Motion Vectors can range from x and y values of -16, -8, 0, +8, +16. We utilize MSD so find the vectors which have the best result.
std::pair<std::vector<std::vector<int>>,std::pair<int,int>> MotionVector(int x_val, int y_val, int height, int width, std::vector<std::vector<int>> previous, std::vector<std::vector<int>> data){
    int count = 0;
    int start_x = x_val; 
    int end_x = 0;
    if(start_x < 8){
        while(start_x > 0){
            count++;
            start_x--;
        }
        end_x = start_x+(32-count);
    }else if(start_x > width-24){
        end_x = start_x;
        while(end_x < width){
            count++;
            end_x++;
        }
        start_x = width - count -(32-count);
    }else{
        start_x = x_val -8;
        end_x = x_val +24;
    }
    int count2 = 0;
    int start_y = y_val; 
    int end_y = 0;
    if(start_y < 8){
        while(start_y > 0){
            count2++;
            start_y--;
        }
        end_y = start_y+ (32-count2);
    }else if(start_y > height-24){
        end_y = start_y;
        while(end_y < height){
            count2++;
            end_y++;
        }
        start_y = height - count2 -(32-count2);
    }else{
        start_y = y_val -8;
        end_y = y_val +24;
    }
    auto result = create_2d_vector<int>(16,16);
    int min_x = 0;
    int min_y = 0;
    int record_x = start_x;
    double MSD = -1;
    while(true){
        int local_sum = 0;
        auto temp = create_2d_vector<int>(16,16);
        for(int i = 0; i<16; i++){
            for(int j = 0; j < 16; j++){
                local_sum+= pow(data.at(i).at(j) - previous.at(start_y+i).at(start_x+j),2);
                temp.at(i).at(j) = data.at(i).at(j) - previous.at(start_y+i).at(start_x+j);
            }
        }
        if(MSD == -1){
            MSD = (1/(pow(16,2)))*local_sum;
        }else{
            if(MSD > ((1/(pow(16,2)))*local_sum)){
                MSD = (1/(pow(16,2)))*local_sum;
                min_x = start_x- x_val;
                min_y = start_y - y_val; 
                result = temp;
            }
        }
        if(start_x+16 >=end_x){
            if(start_y+16 >=end_y){
                break;
            }else{
                start_x = record_x;
                start_y += 16;
            }
        }else{
            start_x += 16;
        }
    }
    return std::make_pair(result,std::make_pair((min_x), (min_y)));
}

//Main body of code, blocks takes the input and parses them into 3- 16x16 blocks, one I-frame, one P-frame, one P-frame motion vector.
//Dct and quantization is performed then we send all 3 blocks to be examined in order to determine the proper block to use to maximize encoding.
//In this function the block is then placed into encoding order and then encoding is done before the block is streamed into the file.
std::vector<std::vector<int>> blocks(std::vector<std::vector<int>> data, std::vector<std::vector<int>>previous, int height, int width, std::vector<std::vector<double>> c, OutputBitStream stream, std::vector<std::vector<int>> Q, std::string quality, int first){
    int recorded_x = 0;
    int recorded_y = 0;
    auto encoded_data = create_2d_vector<int>(height, width); 
    while(true){
        if(first == 0){
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
                motion_dct_block = DCT_low(motion_frames, c);
            }
            std::pair<std::vector<std::vector<int>>,int> quantum = Quantized(dct_block, Q);
            std::pair<std::vector<std::vector<int>>,int> quantum_p = Quantized(prev_dct_block, Q);
            std::pair<std::vector<std::vector<int>>,int> quantum_v = Quantized(motion_dct_block, Q);
            auto for_prev = create_2d_vector<int>(16,16);
            for(int i = 0; i<16; i++){
                for(int j = 0; j< 16; j++){
                    for_prev.at(i).at(j) = (quantum.first.at(i).at(j)-127)*Q.at(i).at(j);
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
                            int data = round(inverse_dct.at(i).at(j));
                            if(data > 255){
                                data = 255;
                            }else if(data < 0){
                                data = 0;
                            }
                            encoded_data.at(recorded_y+i).at(recorded_x+j) = data;
                        }
                    }
                }
            }
            std::vector<int> encoding = E_O(quantum.first);
            std::vector<int> encoding_p = E_O(quantum_p.first);
            std::vector<int> encoding_v = E_O(quantum_v.first);
            if(recorded_x == 0 &&recorded_y == 0){
                rle(encoding, stream, 0, std::make_pair(0,0));
            }else{
                std::pair<std::vector<int>, int> pair = P_or_IFrame(encoding_v, encoding_p, encoding, x_y, std::make_pair(quantum_p.second,quantum_v.second));
                rle(pair.first, stream, pair.second, x_y);
            }
        }else{
            auto temporary = create_2d_vector<int>(16,16);
            std::vector<int> prev_row;
            int prev = -1;
            for(int i = 0; i<16; i++){
                for(int j = 0; j < 16; j++){
                    if(recorded_y+i < height){
                        if(recorded_x+j < width){
                            temporary.at(i).at(j) = data.at(recorded_y+i).at(recorded_x+j);
                            prev = temporary.at(i).at(j);
                            if(prev_row.size() == 16){
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
                        for(int k = 0; k< 16; k++){
                            temporary.at(i).at(k) = prev_row.at(k);
                        }
                    }
                }
            }
            auto dct_block = create_2d_vector<double>(16,16);
            if(quality=="high"){
                dct_block = DCT_high(temporary, c);
            }else{
                dct_block = DCT_low(temporary, c);
            }
            std::pair<std::vector<std::vector<int>>,int> quantum = Quantized(dct_block, Q);
            auto for_prev = create_2d_vector<int>(16,16);
            for(int i = 0; i<16; i++){
                for(int j = 0; j< 16; j++){
                    for_prev.at(i).at(j) = (quantum.first.at(i).at(j)-127)*Q.at(i).at(j);
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
                            int data = round(inverse_dct.at(i).at(j));
                            if(data > 255){
                                data = 255;
                            }else if(data < 0){
                                data = 0;
                            }
                            encoded_data.at(recorded_y+i).at(recorded_x+j) = data;
                        }
                    }
                }
            }
            std::vector<int> encoding = E_O(quantum.first);
            rle(encoding, stream, 0, std::make_pair(0,0));
        }
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
    //I then create one that worked well with my DCT in hopes to limit the number of values which exceed the range of -127 and 127. 
    //https://cs.stanford.edu/people/eroberts/courses/soco/projects/data-compression/lossy/jpeg/coeff.htm 
    //This has been expanded to a 16 x 16 Quantization matrix building on the 8 x 8 matrix using trial and error.
    auto Q = create_2d_vector<int>(16,16);
    //the next if statements allow us to change the quality setting of our Quantum based on the user input.
    if(quality == "low"){
        Q ={
            {  30,  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105 },
            {  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110 },
            {  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115 },
            {  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120 },
            {  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125 },
            {  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130 },
            {  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135 },
            {  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140 },
            {  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145 },
            {  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150 },
            {  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155 },
            {  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160 },
            {  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165 },
            {  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170 },
            { 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170, 175 },
            { 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180 }
        };
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = round(2*(Q.at(i).at(j))); 
            }
        }
        output_stream.push_byte((unsigned char)0);
        
    }else if(quality == "high"){
        Q ={
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
            {11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11},
        };
        output_stream.push_byte((unsigned char)2);
    }else{
        Q ={
            {  30,  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105 },
            {  35,  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110 },
            {  40,  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115 },
            {  45,  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120 },
            {  50,  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125 },
            {  55,  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130 },
            {  60,  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135 },
            {  65,  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140 },
            {  70,  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145 },
            {  75,  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150 },
            {  80,  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155 },
            {  85,  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160 },
            {  90,  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165 },
            {  95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170 },
            { 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170, 175 },
            { 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 155, 160, 165, 170, 175, 180 }
        };
        for(int i = 0; i< 16; i++){
            for(int j = 0; j<16; j++){
                Q.at(i).at(j) = round(0.8*(Q.at(i).at(j))); 
            }
        }
        output_stream.push_byte((unsigned char)1);
    }

    output_stream.push_u32(height);
    output_stream.push_u32(width);
    //x_p stands for the previous version of the following YUV planes in order to apply P frames or motion vectors. 
    auto y_p = create_2d_vector<int>(height,width);
    auto cb_p = create_2d_vector<int>((height/2),(width/2));
    auto cr_p = create_2d_vector<int>((height/2),(width/2));
    int first = 1;
    while (reader.read_next_frame()){
        output_stream.push_byte(1); //Use a one byte flag to indicate whether there is a frame here
        YUVFrame420& frame = reader.frame();
        auto y_val = create_2d_vector<int>(height, width);
        for (u32 y = 0; y < height; y++)
            for (u32 x = 0; x < width; x++)
                y_val.at(y).at(x) = frame.Y(x,y);
        y_p = blocks(y_val, y_p, height, width, coeff, output_stream, Q, quality, first);
        auto Cb = create_2d_vector<int>((height/2),(width/2));
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                Cb.at(y).at(x) = frame.Cb(x,y);
        cb_p =blocks(Cb, cb_p, ((height)/2), ((width)/2), coeff, output_stream, Q, quality,first);
        auto Cr = create_2d_vector<int>((height/2),(width/2));
        for (u32 y = 0; y < height/2; y++)
            for (u32 x = 0; x < width/2; x++)
                Cr.at(y).at(x) = frame.Cr(x,y);
        cr_p = blocks(Cr, cr_p, ((height)/2), ((width)/2), coeff, output_stream, Q, quality,first);
        first = 0;
    }

    output_stream.push_byte(0); //Flag to indicate end of data
    output_stream.flush_to_byte();

    return 0;
}