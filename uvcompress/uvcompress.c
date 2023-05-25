/* uvcompress.c
   CSC 485B/CSC 578B

   Placeholder starter code for UVCompress
   Provided by: B. Bird - 2023-05-01

   Student Name: Payton M
   V#:V00904677
   Due Date: 2023-05-23
   Note: I did not perform well on this assignment.
   I understand that I will receive a 0 grade.
   I failed to optimize this code with a more efficient data structure.
   Therefore it will fail due to lookup computation times. 
   Secondly, this file is susceptible to overflow as its memory is not dynamically optimized 
   Additionally, in general I failed to make a proper compression algorithm.
   I know I can do better and I will try my hardest to excel and do better.
   Sincerely,
   Payton Larsen Murdoch.
   */
//added -lm to the Makefile so that I could use pow commands for changing dictionary length.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

//Initial Definitions
#define INITIAL_ENTRY 256
#define FIRST_ENTRY 257
#define BASE 2
#define INITIAL_BIT 9
#define MAX_BIT 16
#define INPUT_SIZE 10
#define PACKETIZE 8

//Info struct for globally utlized variables associated with the dictionary and bit sizes
typedef struct{
        int curr_bit;
        int next_symbol;
        int carry_size;
}Info;

//Function prototypes
void Encode(char* str, int size, char** dict, char* working, int length,Info *info, int* carryover);
int Comparison(char** dict, char *str, int size);
void Output(int number, int bit, int* carryover, Info *info);
void ReverseOrder(int* carryover);

//Main function for code
int main(){
	//Initializing dictionary
    char **dict;
    dict = malloc(pow(BASE,INITIAL_BIT)*sizeof(char*));
    for(int i = 0; i <INITIAL_ENTRY; i++){
		dict[i] = malloc(2*sizeof(char));
        dict[i][0] = i;
        dict[i][1] ='\0';
    }
	//Add magic numbers to output
    int arr = 0x1f;
    fwrite(&arr, 1,1,stdout);
    arr = 0x9d;
    fwrite(&arr, 1,1,stdout);
    arr = 0x90;
    fwrite(&arr, 1,1,stdout);
	//Initialize information for other functions to work in unison.
    Info dict_info;
    dict_info.curr_bit = INITIAL_BIT;
    dict_info.next_symbol = FIRST_ENTRY;
    dict_info.carry_size = 0;
	//local variables for while function.
    char next_character;
    int count = 0;
    char input_string[10];
    int length = pow(BASE,MAX_BIT);
    char work[length];
    int carry[PACKETIZE];
	//Get the sequences of chars.
    while((next_character = fgetc(stdin))!=EOF){
		input_string[count] = next_character;
        count++;
		//if there are 10 chars in the string then offload and start computing.
        if(count == 10){
			Encode(input_string, 10, dict, work,length, &dict_info, carry);
            count = 0;
            strncpy(input_string,"",sizeof(input_string));
        }
    }
	//after initial function if there are still leftover characters to start computing.
	if (0 <count && count <=10){
		Encode(input_string, count, dict, work, length, &dict_info, carry);
	}
	//if there is still strings or characters in the working string that has not been computed yet.
    if(strlen(work) > 0){
		int last_num = Comparison(dict,work, dict_info.next_symbol);
        if(last_num == 65536){
			last_num = dict_info.next_symbol;
        }
        Output(last_num, dict_info.curr_bit,carry, &dict_info);
    }
	//Lastly for padding the last bit array with zeros until it can be sent for computing.
    if(dict_info.carry_size != 0){
        for (int i = dict_info.carry_size; i<8; i++){
            carry[i] = 0;
        }
        ReverseOrder(carry);
    }
	//Free dictionary.
	for(int i = 0; i <pow(2,dict_info.curr_bit); i++){
		free(dict[i]);
    }
	free(dict);
}
//Main encoding function, where the brunt of the encode algorithm works. Self explanitory as it abides by the pseudocode.
void Encode(char* str, int size, char** dict, char *working, int length, Info *info, int* carryover){
    for(int i=0; i < size; i++){
        char augmented[length];
        strncpy(augmented, working, length);
        char str1[2];
        str1[0] = str[1];
        str1[1] ='\0';
        strcat(augmented,str1);
        int index_num = Comparison(dict, augmented, info->next_symbol);
        if(index_num != 65536){
			strcpy(working,augmented);
		}else if(info->next_symbol >=pow(BASE,MAX_BIT)){
			int work_index = Comparison(dict, working, info->next_symbol);
            Output(work_index,info->curr_bit, carryover, info);
            strcpy(working,str1);
        }else{
            dict[info->next_symbol] = malloc(pow(BASE,MAX_BIT)*sizeof(char));
            strcpy(dict[info->next_symbol], augmented);
            int comp_index = Comparison(dict, working, info->next_symbol);
            info->next_symbol += 1;
            Output(comp_index, info->curr_bit, carryover, info);
            strcpy(working, str1);
            if(info->next_symbol > pow(BASE,info->curr_bit)){
				info->curr_bit += 1;
                dict = realloc(dict,pow(BASE,info->curr_bit)*sizeof(char*));
            }
        }
    }
}

//Scan through the dictionary and look for the elements.
int Comparison(char** dict, char *str, int size){
    for(int i = 0; i<size; i++){
        if(i == 256){
			//pass
        }
        else if(strcmp(dict[i], str)==0){
			return i;
        }
    }
    return 65536;
}


//Output function performs rudimentary operations for converting ints to binary numbers of size in the current bits.
void Output(int number, int bit,int *carryover, Info *info){
    int reverse[bit];
    int to_bin = number;
    int count =0;
    while(to_bin != 0){
		if(to_bin%2 == 0){
			reverse[count] =0;
        }else{
			reverse[count] = 1;
        }
		to_bin = to_bin/2;
        count++;
    }
    for(int i = 0; i< bit; i++){
		if(info->carry_size != 8){
			carryover[info->carry_size] = reverse[i];
            info->carry_size += 1;
        }else{
			ReverseOrder(carryover);
            carryover[0] = reverse[i];
            info->carry_size = 1;
        }
    }
}

//Reverse Order computes packetized 8 bit values for printing.
void ReverseOrder(int *carryover){
    int count = 0;
	for(int i = 0; i< 8; i++){
		if(carryover[i] == 1){
			count += pow(BASE,i);
        }
    }
    fwrite(&count,1,1,stdout);
}

