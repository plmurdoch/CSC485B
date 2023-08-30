**Overview**
------------
My Implementation of the uvid lossy video compression returns to the basic methods which we have studied over the course of this semester. 
My compression scheme is rather simple as I did not choose to apply any advanced features. 
First the compression setting is computed and the quantization matrices are adjusted accordingly.
Upon reading a 16 x 16 block of input from the Y, U/Cb or V/Cr frame my code will compute 3 different encoding methods:
1. The standard I-frame encoding which is given block type 00 in the bitstream.
2. standard P-frame block which utilizes the difference between the current and previous value which uses block type 01 in bitstream.
3. Applying a motion vector with a local search of P-frame in a 32 x 32 matrix block surrounding the current block and remaining within the frame of the plane.
This utilizes block type 10 in the bitstream.
Each of these methods have DCT applied to them, quantization and are bound between -127 and 127 (and increased by 127 so they are 0 and 255) before they are compared to each other.
My code utilizes the Variable Unary RLE scheme which we learned in the beginning of the semester. In order to take advantage of this scheme,
I compare the 3 encoding methods, first I look to see if there is an offset which differentiates the the P-frame and the motion vector.
Any motion vector that occurs at vector (0,0) will be considered a normal P-frame. They are then based on the length of their longest runs. 
Once the encoding method is selected I will send the block type code (00, 01, 10) into the bitstream before beginning to encode the block.
If block type 10 is selected, then two 3-bit indicators are sent to the bitstream next, these codes are 000, 001, 010, 011, 100 and they represent vector values -16 ,-8, 0, 8, 16.
I decided to apply a rudimentary prefix code which takes advantage of the deltas -1, 0 or 1 (in this case the values are 126, 127, 128). They are assigned as codes 110, 0, 10 
respectively and finally the prefix code 111 means that the number is not compressed and thus it is an entire byte. This continues until the entire file is compressed.
The decompressor knows all this information, therefore only needs to parse the bytes it recieves from the file and sequentially build back up the video file frame by frame. 
**Features Implemented**
------------------------
I have only implemented the Basic requirements.
- Documentation within this README.md.
- I have constructed a code which utilizes temporally compressed P-frames. (Line 366- uvid_compress.cpp)
- I have implemented motion comprensation using the local search with each block (Line 479 - uvid_compress.cpp) 
- Although the quality of the medium and low videos are not without numerous artifacts, I have achieved a 12 compression ratio in all the videos I have run my program on. 
**Architecture**
----------------
The architecture of my code takes advantage of c++ vectors in order to store, offload and reconstruct data. Utilizing vectors of vectors allowed me to construct matrices which
stored the information needed in order to conduct the operations in order to successfully implement my assignment.

For the compressor, in the main function (Line 736-858) first the width, height and quality information is sent to the stream then the Y, Cb and Cr planes are built up by the code provided by Professor Bird and it is then sent to the blocks function with the height and width information of the frame alongside other helper data structures like the Coefficient matrix, the Quantization matrix and the previous frame matrix. 
The block function(Lines 561-734) parses the matrix of frame data into a 16 x 16 block of data. This data is then split into 3 different variations, normal I-frame, P-frame and motion vector block (Using motionvector function). 
Each of the variations have DCT applied onto them with DCT_high function (Lines 84-102) or DCT_low(Lines 64-82) depending on quality setting, then they are Quantized 
and clipped between -127 and 127, then iterated so that they are within 0 and 255 for byte output in Quantized function (Lines 105-124).
Each of these variations are put a 1-d vector which is designated to orient them into the Zig-Zag encoding order which JPEG utilizes in the E_O function (Lines 127-176).
The variations are then passed into the I_or_Pframe function (Lines 366 - Lines 435)which navigates each of the variations, determines if the P or motion vector variations are unusable and compares the longest runs of the viable options.
The resulting variation is then sent to the RLE function (Lines 196-361) which does most of the computational work. All the bits in this stream are sent into a buffer to packetize them into bytes before being streamed into output in the offload function (Lines 179-188). First the bitstream is occupied with the block type and the vector coordinates if it is of type 10. 
Then the function determines what prefix code should be assigned to the numbers. 126 becomes 110, 127 becomes 0, 128 becomes 10 and anything else gets prefix 111 followed by the actual byte value.
Following each number there is V-RLE length code. As we know from lectures VRLE utlizes unary 11...10 to indicate the number of bits for the length before the actual length code which omits the first bit. This compresses the number of bits needed to represent any lengths of 0 repetitions.
Once the block is finished if there are still bits in the buffer then it is padded and sent to the stream.
After all this is computed for all blocks in the frame the function returns to main with the matrix representation of the now previously encoded and decoded representation of the frame using inverse_dct_high or inverse_dct_low function (lines 437-475) to be used in the next computation. 

For the decompressor, in the main function(Lines 491-626) first the width, height and quality settings are grabbed in the first 72 bits of the stream. Then once entering a while loop which indicates that a frame is present the stream is sent to function read_input(Lines 127-Lines 488) which does all the work.
In this function it is first checked that it is at the start of a block which is indicated by the block type bits and if it is a motion vector there is an additional header.
Then the prefix codes are checked for the first bits and the numbers are built back up from the stream alongside the length codes. Then they are outputed into a vector which represented the encoding order of the stream.
Number : RLE length pairs are continuously computed until there are enough are gathered to build up a 16x 16 matrix, the values are unquantized, the inverse DCT (Lines 81-119) is computed and the results are sent into a matrix for width and height in order to reconstruct the image. 
If there is any buffer overflow of 8 bits or greater in between blocks or frames the overflow is returned so that it can contribute to the next frame or block.
Once the frame has height*width -1 values the frame is returned as a matrix and used to reconstruct the frame.
**Bitstream**
-------------
The Bitstrem has been discussed numerous times in this documentation therfore to sum it up:
- Quality Byte
- Height U32
- Width U32
The following is computed recursively until the end of frames
-- 1byte to indicate if there is a frame(1)
Note: All the following are packetized by an 8 bit buffer into a byte before being offloaded into the stream.
The following is computed recursively until the end of the frame
--- 2 bit block type code
--- (if type 10 then two 3-bit codes for vector direction -16: 000, -8: 001, 0: 010, 8: 011, 16: 100)
The following is computed recursively until it reaches the end of the block. 
---- Prefix code: 0 means 127, 10 means 128, 110 means 126, 111 means normal byte
---- (if 111 then normal byte follows.)
---- V-RLE Unary bit Code 11...10
---- Length code minus first bit.
---- ...
--- Once end of block then additional 0 added to packetize the last bits in the buffer.
--- ...
-- Once end of frames:
-  1 byte to declare end of input (0)
**Bibliography**
---------------
I utilized this resource to better understand how quantization works for assignment A3
I then expanded on this in A4 in order to construct 16 x 16 quantization matrices
//https://cs.stanford.edu/people/eroberts/courses/soco/projects/data-compression/lossy/jpeg/coeff.htm 
**Special Notes** 
It has been an honor to be in this class this summer, it has been a difficult course but I am immensely grateful to have gotten this experience.
Sincerely, Payton Murdoch.