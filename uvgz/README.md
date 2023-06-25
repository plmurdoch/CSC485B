CSC 485B Assignment 2
=====================
**Thank you for looking at my Assignment 2**

Features Implemented
--------------------
My implementation of DEFLATE includes the fundamental aspects of the algorithm with LZSS being utilized for type 1 and type 2 outputs. The back reference window for LZSS is composed of the maximum size of 32768 entries and slides with the 258 look ahead window.

When considering the time my algorithm takes to compress the entire collection of data, numerous tests have revealed that it takes a little under 4 minutes with the restrictions I have implemented to make it quicker. I decided to simply utilize a linear search method as I wanted to emphasize correctness, therefore speed has suffered. The restriction is the BACK_REF_LENGTH variable which allows the back reference checking loop to break once it has reached a length for the length/distance that is greater than BACK_REF_LENGTH. This restriction limits the compression ratio, however, without the restriction it passes in 5 minutes. 

Furthermore, my type 2 code includes dynamic LL and distance codes which generate canonical codes based on the actual frequency of LL and distance symbols.

I chose to forgo utilizing advanced data structures to optimize LZSS backreferences, I wanted to emphasis correctness and make sure my code met all the minimum requirements to pass the assignment. My implementation of LZSS utilizes simply vectors for its sliding window and utilizes the maximum possible size 32768.

However, my code generally utilizes a large number of unordered maps, ordered maps and priority queue data structures and Binary trees denoted as hufftree in order to traverse and search for keys quickly so that the only slow portion of my code is the linear search in the back reference section.

There is not optimization for CL prefix codes.

There is optimization logic in the main functions to use type 0 blocks when uncompressible and optimization to avoid emitting Type 2 blocks when type 1 would generate a more optimal output based on observations found while running the validation script.

Insights into code
------------------------------
From lines 0-31 we add our include statements for librarys and introduce some global variables:
MAX_BLOCK_SIZE Max bytes read for type 1 and type 2 outputs
MAX_BACK_REF Max Back Reference size to work with 
MIN_BLOCK_SIZE If a block is smaller than this size output type 1 as it would result in more compression.
BACK_REF_LENGTH Minimum size needed.
Line 32 - BlockType0() prints the simple uncompressed version of blocks, this function was written by Professor B. Bird.
Line 51 - LengthCode() LengthCode function implements an unordered map which is preoccupied with the actual integers mapped to the Length codes, number of bits and offsets stored as a tuple. 
Line 135 - DistanceCode() DistanceCode function implements an unordered map which is preoccupied with the actual integers mapped to the Distance codes, number of bits and offsets stored as a tuple. 
Line 291 - FixedHuffman() FixedHuffman function takes the integer code of the length/literal and returns the type 1 code it utilizes.
Line 316 - BitStream() BitStream takes an integer and size and returns a vector of size made up of 0s and 1s to represent the binary representation of the integer stored least significant bit first. 
Line 329 - Type1BlockOffload() Type1BlockOffload takes the stream, buffer, Length codes and Distance codes as input and outputs the stream as type 1 huffman codes.
Line 386 - FreqDist() FreqDist parses the buffer and identifies LL frequencies and distance frequencies and stores them as unordered maps. It returns the maps as a pair.
Following Node struct, comp struct, CodeType2 and Hufftree functions were inspired by https://www.geeksforgeeks.org/huffman-coding-greedy-algo-3/ this article was written by Aashish Barnwal and the GeeksforGeeks team.
The code provided in that site is referenced in lines 435-470 and 524-548 to assist in generating length values to be utilized in the Canonical coding algorithm.
Line 439 - Node struct Node structure with pointers to its children and contains the pair of LL/distance codes and frequency symbols.
Line 453 - Min struct Min structure to be used by the priority queue as a comparator to push the minimum values to the top.
Line 463 - CodesType2() CodesType2 navigates the tree and increments the code integer in a recursive loop, when it reaches a leaf node increment the code integer and loop.
Line 477 - Canonical struct Canonical structure is used by the canonical coding algorithm to sort a priority queue of lengths.
Line 491 - Compare struct Compare function utilized to compare canonical structs a and b by lengths, if they are of equal size then the smaller Ascii character goes first.
Line 505 - Canonical_Codes() Canonical Codes algorithm generates the corresponding codes for each length code.
Line 542 - HuffTree() HuffTree generates a huffman tree using the input table, after generation of the tree the lengths of corresponding symbols are passed to the Canonical codes algorithm to be computed and returned.
Line 571 - CLenstream() Canonical code lengths for LL and distances are then passed to the CLenstream so that it can inform the decoder.
Line 595 - Type2BlockOffload() Block Type 2 Offloading function recieves the stream, buffer, LL and dist frequency codes, and Length and distance offset codes. CL encodings in header purposefully exclude RLE encodings to prioritize generating correct outputs therefore large. Bitstream block almost exactly the same to Block Type 1 output.
Line 703 - Search() Search function utilizes a linear search to scan through the back reference vector and look ahead vector to find length distance pairs that meet the minimum requirement length (denoted greater than 5). Minimum requirement length restriction lowers the overall compression rate, however, improves speed to meet under 240 second compression for dataset.  
Line 762 - Main() 

High Level Description of Code
------------------------------
In the main function:
While reading input add to the look_ahead buffer.
If we detect any uncompressible symbols flag them as uncompressible and begin adding symbols to the blocktype0 buffer for output.
If the look ahead buffer full then see if any back references occur in the search function.
While observing back refs we have a output buffer that gets filled up with Length distance pairs and literals.
Move through the look ahead buffer, put symbols into back reference if at any point the back reference buffer exceeds the maximum amount then delete the oldest elements and continue.
Back in main, if the output buffer reaches its maximum capacity while we are still reading inputs then,
we calculate the frequency of LL symbols, and then compute the Huffman tree and Canonical codes based on these frequencies.
If there is a canonical code with a length > 14, there is a probability of an error in type 2 outputs, therefore offload as type 1
else compute distance frequencies as canonicals codes and offload as type2.
After output clear the buffer and start again.
At the end of file input, we must determine if the look ahead buffer is not empty, if not then we have more symbols to output.
We call search again to offload to buffer until the look ahead is empty.
Then we follow Same logic as we followed inside the while loop. 
We calculate the frequency of LL symbols, and then compute the Huffman tree and Canonical codes based on these frequencies.
If there is a canonical code with a length > 14, there is a probability of an error in type 2 outputs, therefore offload as type 1
else compute distance frequencies as canonicals codes and offload as type2.
Then we output the running CRC and the overall bytes read and we are done!

References
----------
Node struct, comp struct, CodeType2 and Hufftree functions were inspired by https://www.geeksforgeeks.org/huffman-coding-greedy-algo-3/ this article was written by Aashish Barnwal and the GeeksforGeeks team.
The code provided in that site is referenced in lines 435-470 and 524-548 to assist in generating length values to be utilized in the Canonical coding algorithm.

Advanced Features
-----------------
To my knowledge, asides from implementation of the maps and the utilization of the Canonical code algorithm no advanced structures or algorithms were implemented.

**Again, thank you for reviewing my code and I hope it is easy to follow and meets the standards of the course. Sincerely, Payton Larsen Murdoch**