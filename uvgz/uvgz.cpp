/* uvgz.cpp

   Starter code for Assignment 2 (in C++).
   Provided by:
   B. Bird - 2023-05-12.
   Assignment completed by: Payton Larsen Murdoch.
   Student number: V00904677
*/
#include <iostream>
#include <vector>
#include <array>
#include <unordered_map>
#include <string>
#include "output_stream.hpp"
#include <tuple>
#include <queue>
#include <map>
#include <cassert>
#include <cmath>
// To compute CRC32 values, we can use this library
// from https://github.com/d-bahr/CRCpp
#define CRCPP_USE_CPP11
#include "CRC.h"
#define MAX_BLOCK_SIZE 100000 //Max bytes read for type 1 and type 2 outputs
#define MAX_BACK_REF 32768    //Max Back Reference size to work with 
#define MIN_BLOCK_SIZE 50     //If a block is smaller than this size output type 1 as it would result in more compression.
#define BACK_REF_LENGTH 5     //Set restriction to end linear search once Length/Distance passes this size.

/*
BlockType0 was provided by Professor B. Bird and to allow for literals output, code only outputs type 1 when uncompressible literals are detected.
*/
void BlockType0(OutputBitStream stream, u32 &block_size, std::array<u8, (1<<16)-1> &block_contents){
    if (block_size == block_contents.size()){
        stream.push_bit(0);
    }else{
        stream.push_bit(1);
    }
    stream.push_bits(0, 2);
    stream.push_bits(0, 5);
    stream.push_u16(block_size);
    stream.push_u16(~block_size);
    for(unsigned int i = 0; i < block_size; i++)
        stream.push_byte(block_contents.at(i));
    block_size = 0;
    stream.flush_to_byte();
}
/*
LengthCode function implements an unordered map which is preoccupied with the actual integers
mapped to the Length codes, number of bits and offsets stored as a tuple. 
*/
std::unordered_map<int, std::tuple<u32, int, u32>> LengthCode(){
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
/*
DistanceCode function implements an unordered map which is preoccupied with the actual integers
mapped to the Distance codes, number of bits and offsets stored as a tuple. 
*/
std::unordered_map<int, std::tuple<u32,int, u32>> DistanceCode(){
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

/*
FixedHuffman function takes the integer code of the length/literal and returns the type 1 code it utilizes.
*/
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

/*
BitStream takes an integer and size and returns a vector of size
made up of 0s and 1s to represent the binary representation of the integer
stored least significant bit first. 
*/
std::vector<int> BitStream(u32 integer, int size){
    std::vector<int> bit_stream {};
    u32 j = integer;
    for(int i = 0; i <size; i++){
        bit_stream.push_back(j%2);
        j = j/2;
    }
    return bit_stream;
}

/*
Type1BlockOffload takes the stream, buffer, Length codes and Distance codes as input and outputs the stream as type 1 huffman codes.
*/
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

/*
FreqDist parses the buffer and identifies LL frequencies and distance frequencies and stores them as unordered maps.
It returns the maps as a pair.
*/
std::pair<std::unordered_map<std::string, int>,std::unordered_map<std::string, int>> FreqDist(std::vector<std::string> buffer, std::unordered_map<int, std::tuple<u32, int, u32>> lengths, std::unordered_map<int, std::tuple<u32, int, u32>> distances){
    std::unordered_map<std::string, int> dist {};
    std::unordered_map<std::string, int> freq {};
    for(auto i:buffer){
        int pos = i.find(":");
        if(pos== -1){
            if(freq.find(i) != freq.end()){
                freq.at(i)++;
            }
            else{
                freq.insert({i,1});
            }
        }else{
            std::string search1;
            for(int k= 0; k<pos; k++){
                search1 += i[k];
            }
            int x = stoi(search1);
            std::tuple<u32,int,u32> LL_C = lengths[x];
            if(freq.find(std::to_string(std::get<0>(LL_C))) != freq.end()){
                freq.at(std::to_string(std::get<0>(LL_C)))++;
            }
            else{
                freq.insert({std::to_string(std::get<0>(LL_C)),1});
            }
            int size_str = i.length();
            std::string search2;
            for(int k = pos+1; k<size_str; k++){
                search2 += i[k];
            }
            int y = stoi(search2);
            std::tuple<u32, int, u32> D_C = distances[y];
            if(dist.find(std::to_string(std::get<0>(D_C))) != dist.end()){
                dist.at(std::to_string(std::get<0>(D_C)))++;
            }
            else{
                dist.insert({std::to_string(std::get<0>(D_C)),1});
            }

        }
    }
    freq.insert({std::to_string(256),1});
    return std::make_pair(freq,dist);
}
/*
Node struct, comp struct, CodeType2 and Hufftree functions were inspired by https://www.geeksforgeeks.org/huffman-coding-greedy-algo-3/ 
this article was written by Aashish Barnwal and the GeeksforGeeks team.
The code provided in that site is referenced in lines 435-470 and 524-548 to assist in generating length values to be utilized in the Canonical coding algorithm.
*/

/*
Node structure with pointers to its children and contains the pair of LL/distance codes and frequency symbols.
*/
struct Node{
    std::pair<std::string, int> info;
    struct Node *left_child, *right_child;

    Node(std::pair<std::string, int> info){
        this->info =info;
        left_child = right_child = NULL; 
    }
};

/*
Min structure to be used by the priority queue as a comparator to 
push the minimum values to the top.
*/
struct min{
    bool operator()(Node* left, Node* right){
        return(left->info.second >right->info.second);
    }
};

/*
CodesType2 navigates the tree and increments the code integer in a recursive loop, 
when it reaches a leaf node increment the code integer and loop.
*/
void CodesType2(Node* root, int code, std::map<u32, u32> &T2codes){
    if(root == NULL){
        return;
    }
    if(!root->left_child &&!root->right_child){
        T2codes.insert({(u32)stoi(root->info.first),code});
    }
    CodesType2(root->left_child, code+1, T2codes);
    CodesType2(root->right_child,code+1, T2codes);
}

/*
Canonical structure is used by the canonical coding algorithm to sort a priority queue of lengths.
*/
struct Canonical{
    u32 id;
    int string_size;

    Canonical(const u32 id, u32 string_size){
        this->id =id;
        this->string_size = string_size; 
    }
};

/*
Compare function utilized to compare canonical structs a and b by lengths,
if they are of equal size then the smaller Ascii character goes first.
*/
struct compare{
    bool operator()(Canonical* a,Canonical* b){
        if(a->string_size == b->string_size){
            return (a->id > b->id);
        }
        else{
            return(a->string_size > b->string_size);
        }
    }
};

/*
Canonical Codes algorithm generates the corresponding codes for each length code.
*/
std::map<u32, std::string> Canonical_Codes(std::map<u32,u32> &code_lengths){
    std::priority_queue<Canonical*, std::vector<Canonical*>, compare> minimum_queue;
    for(auto i: code_lengths){
        minimum_queue.push(new Canonical(i.first, i.second));
    }
    std::map<u32, std::string> assign = {};
    u32 height = 0;
    for(auto i: code_lengths){
        if(height < i.second){
            height = i.second;
        }
    }
    u32 bit = 0; 
    while(minimum_queue.size() != 0){
        Canonical* min = minimum_queue.top();
        minimum_queue.pop();
        if(min->string_size != 0){
            std::vector <int> bits = BitStream(bit, height);
            int count_code =  height - (height-min->string_size +1);
            int counter = height- 1;
            std::string p = "";
            for(int i = 0; i< count_code; i++){
                p+=std::to_string(bits.at(counter));
                counter--;
            }
            p+=std::to_string(bits.at(counter));
            assign.insert({min->id, p});
            bit += pow(2,height-min->string_size);
        }
    }
    return assign;
}

/*
HuffTree generates a huffman tree using the input table, after generation of the tree
the lengths of corresponding symbols are passed to the Canonical codes algorithm to be computed and returned.
*/
std::map<u32, std::string> HuffTree(std::unordered_map<std::string, int> table){
    Node *left, *right, *root;
    std::priority_queue<Node*, std::vector<Node*>, min> minimum_queue;
    for(auto i:table){
        minimum_queue.push(new Node(std::make_pair(i.first, i.second)));
    }
    if(minimum_queue.size() == 1){
        Node *temp = minimum_queue.top();
        minimum_queue.push(new Node(std::make_pair(temp->info.first,0)));
    }
    while(minimum_queue.size() > 1){
        right = minimum_queue.top();
        minimum_queue.pop();
        left = minimum_queue.top();
        minimum_queue.pop();
        root = new Node(std::make_pair("EMPTY",left->info.second +right->info.second));
        root->left_child = left;
        root->right_child = right;
        minimum_queue.push(root);
    }
    std::map<u32, u32> codes {};
    CodesType2(root, 0, codes);
    std::map<u32, std::string> new_codes = Canonical_Codes(codes);
    return new_codes;
}

/*
Canonical code lengths for LL and distances are then passed to the CLenstream so that it can inform the decoder.
*/
std::vector<u32> CLenstream(std::map<u32, std::string> LL_code_lengths, std::map<u32, std::string> D_code_lengths){
    std::vector<u32> cl;
    for(int i = 0; i<286; i++){
        if(LL_code_lengths.find(i) != LL_code_lengths.end()){
            cl.push_back((u32)LL_code_lengths.at(i).length());
        }else{
            cl.push_back(0);
        }
    }
    for(int i = 0; i< 30; i++){
        if(D_code_lengths.find(i) != D_code_lengths.end()){
             cl.push_back((u32)D_code_lengths.at(i).length());
        }else{
            cl.push_back(0);
        }
    }
    return cl;
}

/*
Block Type 2 Offloading function recieves the stream, buffer, LL and dist frequency codes, and Length and distance offset codes.
CL encodings in header purposefully exclude RLE encodings to prioritize generating correct outputs therefore large .
Bitstream block almost exactly the same to Block Type 1 output.
*/
void Type2BlockOffload(OutputBitStream &stream, std::vector<std::string> &buff, std::map<u32, std::string> Freq, std::map<u32, std::string> Dist, std::unordered_map<int, std::tuple<u32,int, u32>>Length, std::unordered_map<int, std::tuple<u32,int, u32>> Distance){
    std::vector<u32> CL = CLenstream(Freq, Dist);
    std::unordered_map<std::string, int> CL_Prefix{};
    for(auto i: CL){
        if(CL_Prefix.find(std::to_string(i)) != CL_Prefix.end()){
            CL_Prefix.at(std::to_string(i))++;
        }
        else{
            CL_Prefix.insert({std::to_string(i),1});
        }
    }
    std::map<u32, std::string> CC = HuffTree(CL_Prefix);
    if(buff.size() != MAX_BLOCK_SIZE){
        stream.push_bit(1);
    }else{
        stream.push_bit(0);
    }
    stream.push_bits(2, 2);
    unsigned int HLIT = 286-257;
    unsigned int HDIST = 30-1;
    unsigned int HCLEN = 15; 
    stream.push_bits(HLIT, 5);
    stream.push_bits(HDIST,5);
    stream.push_bits(HCLEN,4);
    std::vector<u32> E_O = {16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15};
    for(auto i: E_O){
        if(CC.find(i) != CC.end()){
            stream.push_bits((int)CC[i].length(), 3);
        }else{
             stream.push_bits(0, 3);
        }
    }

    for(auto i: CL){
        std::string output= CC.at(i);
        int counter = output.length();
        for(int j = 0; j < counter; j++){
            if(output.at(j)== '1'){
                stream.push_bit(1);
            }else{
                stream.push_bit(0);
            }
        }
    }
    buff.push_back("256");
    for(auto i: buff){
        int pos = i.find(":");
        if(pos== -1){
            int x = std::stoi(i);
            std::string huffman = Freq.at(x);
            int Freq_count = huffman.length();
            for(int i = 0; i < Freq_count; i++){
                if(huffman.at(i)== '1'){
                    stream.push_bit(1);
                }else{
                    stream.push_bit(0);
                }
            }
        }
        else{
            int length_size;
            int distance_size;
            std::string temp1;
            for(int k= 0; k<pos; k++){
                temp1 += i.at(k);
            }
            length_size = std::stoi(temp1);
            int size_str = i.length();
            std::string temp2;
            for(int k = pos+1; k<size_str; k++){
                temp2 += i.at(k);
            }
            distance_size = std::stoi(temp2);
            std::tuple<u32, int, u32> LC = Length[length_size];
            std::tuple<u32, int, u32> DC = Distance[distance_size];
            std::string huffman_code = Freq.at(std::get<0>(LC));
            int huff_count =huffman_code.length();
            for(int i = 0; i < huff_count; i++){
                if(huffman_code.at(i)== '1'){
                    stream.push_bit(1);
                }else{
                    stream.push_bit(0);
                }
            }
            if(std::get<1>(LC) != 0){
                stream.push_bits(std::get<2>(LC), std::get<1>(LC));    
            }
            std::string Distance_code = Dist.at(std::get<0>(DC));
            int dist_count = Distance_code.length();
            for(int i = 0; i< dist_count; i++){
                if(Distance_code.at(i)== '1'){
                    stream.push_bit(1);
                }else{
                    stream.push_bit(0);
                }
            }
            if(std::get<1>(DC) != 0){
                stream.push_bits(std::get<2>(DC), std::get<1>(DC));
            }
        }
    }
}

/*
Search function utilizes a linear search to scan through the back reference vector and look ahead vector to find length distance pairs
that meet the minimum requirement length (denoted greater than 5).
Minimum requirement length restriction lowers the overall compression rate, however, improves speed to meet under 240 second compression for dataset.  
*/
void Search(std::vector<u8> &back_ref, std::vector<std::string>&buffer, std::vector<u8> &look_ahead){
    int counter = back_ref.size() -1;
    if(counter <0){
        buffer.push_back(std::to_string((u32)look_ahead.front()));
        back_ref.push_back(look_ahead.front());
        look_ahead.erase(look_ahead.begin());
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
                if(longest_length_hit > BACK_REF_LENGTH){
                    break;
                }
            }
            counter--;
        }
        if(longest_length_hit>2){
            buffer.push_back(std::to_string(longest_length_hit)+":"+std::to_string(back_ref.size()-longest_pos));
            for(int i = 0; i<longest_length_hit; i++){
                if(back_ref.size()>=MAX_BACK_REF){
                    back_ref.erase(back_ref.begin());
                }
                back_ref.push_back(look_ahead.front());
                look_ahead.erase(look_ahead.begin());
            }
        }else{
            buffer.push_back(std::to_string(look_ahead.front()));
            if(back_ref.size()>=MAX_BACK_REF){
                back_ref.erase(back_ref.begin());
            }
            back_ref.push_back(look_ahead.front());
            look_ahead.erase(look_ahead.begin());
        }
    }
}


int main(){

    //See output_stream.hpp for a description of the OutputBitStream class
    std::unordered_map<int, std::tuple<u32, int, u32>> Length_Codes = LengthCode();
    std::unordered_map<int, std::tuple<u32, int, u32>> Distance_Codes = DistanceCode();
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
    //Note that the types u8, u16 and u32 are defined in the output_stream.hpp header
    std::array< u8, (1<<16)-1 > block_contents {};
    std::vector<u8> back_ref;
    std::vector<u8> look_ahead;
    u32 block_size {0};
    u32 bytes_read {0};
    std::vector<std::string> buffer {};
    char next_byte {}; 
    int incompressible = 0;
    int last_block = 0;
    u32 crc {};


    if (!std::cin.get(next_byte)){
        std::cerr<<"Invalid Input Please Pipe in File to Compress"<<std::endl;
        exit(1); 
    }else{
        bytes_read++;
        //Update the CRC as we read each byte (there are faster ways to do this)
        crc = CRC::Calculate(&next_byte, 1, crc_table); //This call creates the initial CRC value from the first byte read.
        //Read through the input
        while(1){
            if((u8)next_byte < 0 && incompressible == 0){
                incompressible = 1; //Incompressible means that it a byte is registering a non-ASCII value and will go into type 0 output. 
            }
            look_ahead.push_back(next_byte);
            block_contents.at(block_size++) = next_byte;
            if(look_ahead.size() == 258){
                if( incompressible != 1){
                    Search(back_ref, buffer, look_ahead); //If the lookahead is full, start looking and offloading into the buffer and back_ref.
                }
                else{
                    while(!look_ahead.empty()){
                        if(back_ref.size()>=MAX_BACK_REF){ //Move up the back reference window if it is at the max size.
                           back_ref.erase(back_ref.begin());
                        }
                        back_ref.push_back(look_ahead.front());
                        look_ahead.erase(look_ahead.begin());
                    }
                }
            }
            if (!std::cin.get(next_byte)){
                break;
            }
            bytes_read= bytes_read + 1;
            crc = CRC::Calculate(&next_byte,1, crc_table, crc);
            if(block_size == block_contents.size()){
                if(incompressible == 1){
                    BlockType0(stream, block_size, block_contents); //If incompressible, clear out look_ahead and buffer and output literals.
                    buffer.clear();
                    while(!look_ahead.empty()){
                        if(back_ref.size()>=MAX_BACK_REF){ //Move up the back reference window if it is at the max size.
                            back_ref.erase(back_ref.begin());
                        }
                        back_ref.push_back(look_ahead.front());
                        look_ahead.erase(look_ahead.begin());
                    }
                    incompressible = 0;
                }else{
                    block_size = 0; //reset block_size and incompressible key.
                    incompressible = 0;
                }
            }
            if(buffer.size() == MAX_BLOCK_SIZE){ //If buffer is big enough, start offloading to type 1 or 2.
                std::pair<std::unordered_map<std::string, int>,std::unordered_map<std::string, int>> results = FreqDist(buffer, Length_Codes, Distance_Codes);
                std::unordered_map<std::string, int> Freq = std::get<0>(results);
                std::unordered_map<std::string, int> Dist = std::get<1>(results);
                std::map<u32, std::string> Freq_mapping = HuffTree(Freq);
                int red_flag = 0;
                for(auto i: Freq_mapping){
                    if(std::get<1>(i).length() > 14){ //check to see if the height of the trees are small enough so that type 2 can be used.
                        red_flag = 1;
                        break;
                    }
                }
                if(red_flag == 1){ //if height of the tree too big then go block type 1.
                    Type1BlockOffload(stream, buffer,Length_Codes, Distance_Codes); 
                    last_block = 1;
                }
                else{ //or go block type 2.
                    std::map<u32, std::string> Dist_mapping = HuffTree(Dist);
                    Type2BlockOffload(stream,buffer, Freq_mapping, Dist_mapping, Length_Codes, Distance_Codes);
                    last_block = 0;
                }
                buffer.clear();
            }
        }
    }
    //Treating leftover look_ahead and buffer.
    if(block_size > 0 && incompressible == 1 ){
        BlockType0(stream, block_size, block_contents);
        buffer.clear();
    }
    else if(!look_ahead.empty()){
        while(!look_ahead.empty()){
            Search(back_ref,buffer, look_ahead);
            if(buffer.size() == MAX_BLOCK_SIZE){
                std::pair<std::unordered_map<std::string, int>,std::unordered_map<std::string, int>> results = FreqDist(buffer, Length_Codes, Distance_Codes);
                std::unordered_map<std::string, int> Freq = std::get<0>(results);
                std::unordered_map<std::string, int> Dist = std::get<1>(results);
                std::map<u32, std::string> Freq_mapping = HuffTree(Freq);
                int red_flag = 0;
                for(auto i: Freq_mapping){
                    if(std::get<1>(i).length() > 14){
                        red_flag = 1;
                        break;
                    }
                }
                if(red_flag == 1){
                    Type1BlockOffload(stream, buffer,Length_Codes, Distance_Codes);
                    last_block = 1;
                }
                else{
                    std::map<u32, std::string> Dist_mapping = HuffTree(Dist);
                    Type2BlockOffload(stream,buffer, Freq_mapping, Dist_mapping, Length_Codes, Distance_Codes);
                    last_block = 0;
                }
                buffer.clear();
            }
        }
    }
    //At this point, we've finished reading the input (no new characters remain), and we may have an incomplete block to write.
    if(!buffer.empty()){
        std::pair<std::unordered_map<std::string, int>,std::unordered_map<std::string, int>> results = FreqDist(buffer, Length_Codes, Distance_Codes);
        std::unordered_map<std::string, int> Freq = std::get<0>(results);
        std::unordered_map<std::string, int> Dist = std::get<1>(results);
        std::map<u32, std::string> mapping_freq = HuffTree(Freq);
        int red_flag = 0;
        for(auto i: mapping_freq){
            if(std::get<1>(i).length() > 14){
                red_flag = 1;
                break;
            }
        }
        if(red_flag == 1 || buffer.size() < MIN_BLOCK_SIZE || last_block == 1){
            Type1BlockOffload(stream, buffer,Length_Codes, Distance_Codes);
        }
        else{
            std::map<u32, std::string> Dist_mapping = HuffTree(Dist);
            Type2BlockOffload(stream,buffer, mapping_freq, Dist_mapping, Length_Codes, Distance_Codes);
        }
        stream.flush_to_byte();
    }
    //Now close out the bitstream by writing the CRC and the total number of bytes stored.
    stream.push_u32(crc);
    stream.push_u32(bytes_read);
    return 0;
}