/* uvgz.cpp

   Starter code for Assignment 2 (in C++).
   This basic implementation generates a fully compliant .gz output stream,
   using block mode 0 (store only) for all DEFLATE data.

   B. Bird - 2023-05-12
*/
#include <iostream>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include "output_stream.hpp"
#include <tuple>

// To compute CRC32 values, we can use this library
// from https://github.com/d-bahr/CRCpp
#define CRCPP_USE_CPP11
#include "CRC.h"
#define MAX_BLOCK_SIZE 100000
#define MAX_BACK_REF 32768

void BlockType0(OutputBitStream stream, u32 &block_size, std::array<u8, (1<<16)-1> &block_contents){
    if (block_size == block_contents.size()){
        //The block is full, so write it out.
        //We know that there are more bytes left, so this is not the last block
        stream.push_bit(0); //0 = not last block
    }else{
        //Write out any leftover data
        stream.push_bit(1); //0 = not last block
    }
    stream.push_bits(0, 2); //Two bit block type (in this case, block type 0)
    stream.push_bits(0, 5); //Pad the stream to the next byte boundary.
    //Now write the block size (as a pair (X, ~X) where X is the 16 bit size)
    stream.push_u16(block_size);
    stream.push_u16(~block_size);
    //Now write the actual block data
    for(unsigned int i = 0; i < block_size; i++)
        stream.push_byte(block_contents.at(i)); //Interesting optimization question: Will the compiler optimize away the bounds checking for .at here?
    block_size = 0;
}
std::unordered_map<int, std::tuple<u32, int, u32>> LengthCode1(){
    std::unordered_map<int, std::tuple<u32,int,u32>> return_map;
    u32 code = 257;
    for(int i = 3; i<259; i++){
        if (i<11){
            return_map.insert({i, std::make_tuple(code, 0, 0)});
            code++;
        }
        else if(i <19){
            u32 bit = 0;
            while(bit<2){
                return_map.insert({i, std::make_tuple(code, 1, bit)});
                bit++;
                if(bit != 2){
                    i++;
                }
            }
            code++;
        }
        else if(i <35){
            u32 bit = 0;
            while(bit<4){
                return_map.insert({i, std::make_tuple(code, 2, bit)});
                bit++;
                if(bit!= 4){
                    i++;
                }
            }
            code++;
        }
        else if(i <67){
            u32 bit = 0;
            while(bit<8){
                return_map.insert({i, std::make_tuple(code,3, bit)});
                bit++;
                if(bit != 8){
                    i++;
                }
            }
            code++;
        }
        else if(i <131){
            u32 bit = 0;
            while(bit<16){
                return_map.insert({i, std::make_tuple(code,4, bit)});
                bit++;
                if(bit != 16){
                    i++;
                }
            }
            code++;
        }
        else if(i <227){
            u32 bit = 0;
            while(bit<32){
                return_map.insert({i, std::make_tuple(code, 5,bit)});
                bit++;
                if(bit != 32){
                    i++;
                }
            }
            code++;
        }
        else if(i <258){
            u32 bit = 0;
            while(bit<31){
                return_map.insert({i, std::make_tuple(code, 5, bit)});
                bit++;
                if(bit != 31){
                    i++;
                }
            }
            code++;
        }
        else{
            return_map.insert({i, std::make_tuple(code, 0, 0)});
        }
    }
    return return_map;
}
std::unordered_map<int, std::tuple<u32,int, u32>> DistanceCode1(){
    std::unordered_map<int, std::tuple<u32,int,u32>> return_map;
    u32 code = 0;
    for(int i = 1; i<32789; i++){
        if (i<5){
            return_map.insert({i, std::make_tuple(code, 0,0)});
            code++;
        }
        else if(i <9){
            u32 bit = 0;
            while(bit<2){
                return_map.insert({i, std::make_tuple(code, 1, bit)});
                bit++;
                if(bit != 2){
                    i++;
                }
            }
            code++;
        }
        else if(i <17){
            u32 bit = 0;
            while(bit<4){
                return_map.insert({i, std::make_tuple(code, 2, bit)});
                bit++;
                if(bit != 4){
                    i++;
                }
            }
            code++;
        }
        else if(i <33){
            u32 bit = 0;
            while(bit<8){
                return_map.insert({i, std::make_tuple(code, 3, bit)});
                bit++;
                if(bit != 8){
                    i++;
                }
            }
            code++;
        }
        else if(i <65){
            u32 bit = 0;
            while(bit<16){
                return_map.insert({i, std::make_tuple(code, 4, bit)});
                bit++;
                if(bit != 16){
                    i++;
                }
            }
            code++;
        }
        else if(i <129){
            u32 bit = 0;
            while(bit<32){
                return_map.insert({i, std::make_tuple(code, 5, bit)});
                bit++;
                if(bit != 32){
                    i++;
                }
            }
            code++;
        }
        else if(i <257){
            u32 bit = 0;
            while(bit<64){
                return_map.insert({i, std::make_tuple(code, 6, bit)});
                bit++;
                if(bit != 64){
                    i++;
                }
            }
            code++;
        }
        else if(i <513){
            u32 bit = 0;
            while(bit<128){
                return_map.insert({i, std::make_tuple(code, 7, bit)});
                bit++;
                if(bit != 128){
                    i++;
                }
            }
            code++;
        }
        else if(i <1025){
            u32 bit = 0;
            while(bit<256){
                return_map.insert({i, std::make_tuple(code, 8, bit)});
                bit++;
                if(bit != 256){
                    i++;
                }
            }
            code++;
        }
        else if(i <2049){
            u32 bit = 0;
            while(bit<512){
                return_map.insert({i, std::make_tuple(code, 9, bit)});
                bit++;
                if(bit != 512){
                    i++;
                }
            }
            code++;
        }
        else if(i <4097){
            u32 bit = 0;
            while(bit<1024){
                return_map.insert({i, std::make_tuple(code, 10, bit)});
                bit++;
                if(bit != 1024){
                    i++;
                }
            }
            code++;
        }
        else if(i <8193){
            u32 bit = 0;
            while(bit<2048){
                return_map.insert({i, std::make_tuple(code, 11, bit)});
                bit++;
                if(bit != 2048){
                    i++;
                }
            }
            code++;
        }
        else if(i <16385){
            u32 bit = 0;
            while(bit<4096){
                return_map.insert({i, std::make_tuple(code, 12, bit)});
                bit++;
                if(bit != 4096){
                    i++;
                }
            }
            code++;
        }
        else if(i <=32768){
            u32 bit = 0;
            while(bit<8192){
                return_map.insert({i, std::make_tuple(code, 13, bit)});
                bit++;
                if(bit != 8192){
                    i++;
                }
            }
            code++;
        }
    }
    return return_map;
}
std::pair<u32, int> FixedHuffman(int Code){
    u32 Lit_to_143 = 48;
    u32 Lit_to_255 = 400;
    u32 Lit_to_287 = 192;
    if(Code<144){
        return std::make_pair(Lit_to_143 + Code, 8);
    }
    else if(Code<256){
        return std::make_pair(Lit_to_255+(Code-144), 9);
    }
    else if(Code<280){
        return std::make_pair(Code-256, 7);
    }
    else{
        return std::make_pair(Lit_to_287+(Code-280), 8);
    }
}

std::vector<int> BitStream(u32 integer, int size){
    std::vector<int> bit_stream {};
    u32 j = integer;
    for(int i = 0; i <size; i++){
        bit_stream.push_back(j%2);
        j = j/2;
    }
    return bit_stream;
}

void Type1BlockOffload(OutputBitStream &stream, std::vector<std::string> &buff, std::unordered_map<int, std::tuple<u32,int, u32>>Length, std::unordered_map<int, std::tuple<u32,int, u32>> Distance){
    if (buff.size() == MAX_BLOCK_SIZE){
        stream.push_bit(0);
    }else{
        stream.push_bit(1); 
    }
    stream.push_bits(1,2);
    for(auto i:buff){
        int pos = i.find(":");
        if(pos== -1){
            int x = std::stoi(i);
            std::pair<u32, int> huffman = FixedHuffman(x);
            std::vector<int> code = BitStream(std::get<0>(huffman), std::get<1>(huffman));
            for(int i = std::get<1>(huffman)-1; i >= 0; i--){
                stream.push_bit(code.at(i));
            }
        }
        else{
            int length_size;
            int distance_size;
            std::string temp1;
            for(int k= 0; k<pos; k++){
                temp1 += i[k];
            }
            length_size = std::stoi(temp1);
            int size_str = i.length();
            std::string temp2;
            for(int k = pos+1; k<size_str; k++){
                temp2 += i[k];
            }
            distance_size = std::stoi(temp2);
            std::tuple<u32, int, u32> LC = Length[length_size];
            std::tuple<u32, int, u32> DC = Distance[distance_size];
            std::pair<u32, int> huffman_code = FixedHuffman(std::get<0>(LC));
            std::vector<int> Length_code = BitStream(std::get<0>(huffman_code), std::get<1>(huffman_code));
            for(int i = std::get<1>(huffman_code)-1; i>= 0; i--){
                stream.push_bit(Length_code.at(i));
            }
            if(std::get<1>(LC) != 0){
                stream.push_bits(std::get<2>(LC), std::get<1>(LC));    
            }
            std::vector<int> Distance_code = BitStream(std::get<0>(DC), 5);
            for(int i = 4; i>= 0; i--){
                stream.push_bit(Distance_code.at(i));
            }
            if(std::get<1>(DC) != 0){
                stream.push_bits(std::get<2>(DC), std::get<1>(DC));
            }
        }
    }
    stream.push_bits(0,7);
}

void Type2BlocckOffload(OutputBitStream &stream, std::vector<std::string> &buff){
    /*
    For Type 2 Blocks we have to
    1. Create a LL Frequency table (std::unordered_map).
    2. Create a DC frequency table (std::unordered_map).
    3. Huffman encoding queue of nodes with LL and frequency.
    4. Huffman encoding queue of distance Codes.
    5. Write tree encodings into file.
    6. Encoding similar to type 1.
    */
}
void Search(std::string &back_ref, std::vector<std::string>&buffer, std::string &look_ahead){
    int counter = back_ref.size() -1;
    if(counter <0){
        buffer.push_back(std::to_string((u32)look_ahead.front()));
        back_ref.push_back(look_ahead.front());
        look_ahead.erase(0,1);
    }else{
        int look_size = look_ahead.size();
        int length_back = back_ref.size();
        int longest_length_hit = 0;
        int longest_pos = -1;
        while(counter >= 0){
            int longest_hit = 0;
            int i = counter;
            int pos = -1;
            for(int j= 0; j < look_size; j++){
                if(look_ahead[j] == back_ref[i]){
                    if (pos == -1){
                        pos = i;
                    }
                    i++;
                    longest_hit++;
                    if(i == length_back){
                        i = counter;
                    }
                }else{
                    break;
                }
            }
            if(longest_length_hit < longest_hit){
                longest_length_hit = longest_hit;
                longest_pos = pos;
            }
            counter--;
        }
        if(longest_length_hit>2){
            buffer.push_back(std::to_string(longest_length_hit)+":"+std::to_string(back_ref.size()-longest_pos));
            for(int i = 0; i<longest_length_hit; i++){
                if(back_ref.size()>=MAX_BACK_REF){
                    back_ref.erase(0,1);
                }
                back_ref.push_back(look_ahead.front());
                look_ahead.erase(0,1);
            }
        }else{
            buffer.push_back(std::to_string(look_ahead.front()));
            if(back_ref.size()>=MAX_BACK_REF){
                back_ref.erase(0,1);
            }
            back_ref.push_back(look_ahead.front());
            look_ahead.erase(0,1);
        }
    }
}

int main(){

    //See output_stream.hpp for a description of the OutputBitStream class
    std::unordered_map<int, std::tuple<u32, int, u32>> Length_Codes = LengthCode1();
    std::unordered_map<int, std::tuple<u32, int, u32>> Distance_Codes = DistanceCode1();
    OutputBitStream stream {std::cout};

    //Pre-cache the CRC table
    auto crc_table = CRC::CRC_32().MakeTable();

    //Push a basic gzip header
    stream.push_bytes( 0x1f, 0x8b, //Magic Number
        0x08, //Compression (0x08 = DEFLATE)
        0x00, //Flags
        0x00, 0x00, 0x00, 0x00, //MTIME (little endian)
        0x00, //Extra flags
        0x03 //OS (Linux)
    );


    //This starter implementation writes a series of blocks with type 0 (store only)
    //Each store-only block can contain up to 2**16 - 1 bytes of data.
    //(This limit does NOT apply to block types 1 and 2)
    //Since we have to keep track of how big each block is (and whether any more blocks 
    //follow it), we have to save up the data for each block in an array before writing it.
    

    //Note that the types u8, u16 and u32 are defined in the output_stream.hpp header
    std::array< u8, (1<<16)-1 > block_contents {};
    std::string back_ref;
    std::string look_ahead;
    u32 block_size {0};
    u32 bytes_read {0};
    std::vector<std::string> buffer {};
    char next_byte {}; //Note that we have to use a (signed) char here for compatibility with istream::get()
    int incompressible = 0;
    //We need to see ahead of the stream by one character (e.g. to know, once we fill up a block,
    //whether there are more blocks coming), so at each step, next_byte will be the next byte from the stream
    //that is NOT in a block.

    //Keep a running CRC of the data we read.
    u32 crc {};


    if (!std::cin.get(next_byte)){
        //Empty input?
        
    }else{
        bytes_read++;
        //Update the CRC as we read each byte (there are faster ways to do this)
        crc = CRC::Calculate(&next_byte, 1, crc_table); //This call creates the initial CRC value from the first byte read.
        //Read through the input
        while(1){
            if(next_byte < 0 && incompressible == 0){
                incompressible = 1;
            }
            look_ahead+=(next_byte);
            if(look_ahead.size() == 258){
                Search(back_ref, buffer, look_ahead);
            }
            block_contents.at(block_size++) = next_byte;
            if (!std::cin.get(next_byte)){
                break;
            }
            bytes_read++;
            crc = CRC::Calculate(&next_byte,1, crc_table, crc);
            if(block_size == block_contents.size()){
                if(incompressible == 1){
                    BlockType0(stream, block_size, block_contents);
                    buffer.clear();
                    while(!look_ahead.empty()){
                        if(back_ref.size()>=MAX_BACK_REF){
                            back_ref.erase(0,1);
                        }
                        back_ref.push_back(look_ahead.front());
                        look_ahead.erase(0,1);
                    }
                    incompressible = 0;
                }else{
                    block_size = 0;
                }
            }
            if(buffer.size() == MAX_BLOCK_SIZE){
                Type1BlockOffload(stream, buffer,Length_Codes, Distance_Codes);
                buffer.clear();
            }
        }
    }
    if(block_size > 0 && incompressible == 1){
        BlockType0(stream, block_size, block_contents);
        buffer.clear();
    }
    else if(!look_ahead.empty()){
        while(!look_ahead.empty()){
            Search(back_ref,buffer, look_ahead);
            if(buffer.size() == MAX_BLOCK_SIZE){
                Type1BlockOffload(stream, buffer, Length_Codes, Distance_Codes);
                buffer.clear();
            }
        }
    }
    //At this point, we've finished reading the input (no new characters remain), and we may have an incomplete block to write.
    if(!buffer.empty()){
        Type1BlockOffload(stream, buffer, Length_Codes, Distance_Codes);
        stream.flush_to_byte();
    }
    //Now close out the bitstream by writing the CRC and the total number of bytes stored.
    
    stream.push_u32(crc);
    stream.push_u32(bytes_read);
    return 0;
}