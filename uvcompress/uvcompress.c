/* uvcompress.c
   CSC 485B/CSC 578B

   Placeholder starter code for UVCompress
   Provided by: B. Bird - 2023-05-01

   Student Name: Payton Murdoch
   V#:V00904677
   Due Date: 2023-05-23
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define INITIAL_ENTRY 256
#define FIRST_ENTRY 257
#define BASE 2
#define INITIAL_BIT 9
#define MAX_BIT 16
#define INPUT_SIZE 10
#define PACKETIZE 8

void Encode(char* str, char** dict, char* working, int length, int next, int bit, int size, int* carryover);
int Comparison(char** dict, char *str, int size);
void Output(int number, int bit, int* carryover);
void ReverseOrder(int* carryover);
/*
If combination of char and char +1 in the dictionary output index number
else add combination to the dictionary with the next available index number
*/
//what to do, create index structure so we know how to add elements to it
int main(){
    char **dict;
    dict = malloc(pow(BASE,INITIAL_BIT)*sizeof(char*));
    for(int i = 0; i <INITIAL_ENTRY; i++){
		dict[i] = malloc(2*sizeof(char));
        dict[i][0] = i;
        dict[i][1] ='\0';
    }
    fputc(0x1f,stdout);
    fputc(0x9d,stdout);
    fputc(0x90,stdout);
    int next_symbol = FIRST_ENTRY;
    int bits = INITIAL_BIT;
    char next_character;
    int count = 0;
    int size = INITIAL_ENTRY;
    char input_string[10];
    int length = 2;
    char *work = malloc(length*sizeof(char));
    int carry[PACKETIZE];
    while((next_character = fgetc(stdin))!=EOF){
		input_string[count] = next_character;
        count++;
        if(count == 10){
			Encode(input_string, dict, work,length, next_symbol, bits, size, carry);
            count = 0;
            strncpy(input_string,"",sizeof(input_string));
        }
    }
    if(strlen(work) > 0){
		int last_num = Comparison(dict,work, size);
        Output(last_num,bits,carry);
    }
}

void Encode(char* str, char** dict, char *working, int length, int next, int bit, int size, int* carryover){
    for(int i=0; i < INPUT_SIZE; i++){
		char augmented[length+1];
        strncpy(augmented, working, length);
        char str1[2];
        str1[0] = str[1];
        str1[1] ='\0';
        strncat(augmented,str1,2);
        int index_num = Comparison(dict, augmented, size);
        if(index_num != 65536){
			length++;
            free(working);
            working = malloc(length*sizeof(char));                       
			strncpy(working,augmented,length);
        }else if(next >=pow(BASE,MAX_BIT)){
			int work_index = Comparison(dict, working, size);
            Output(work_index,bit, carryover);
            free(working);
            working = malloc(length*sizeof(char));
                        strncpy(working,str1,length);
        }else{
			dict[next] = malloc((length+1)*sizeof(char));
            strncpy(dict[next], augmented, length+1);
            int comp_index = next;
            next++;
            Output(comp_index, bit, carryover);
            length++;
            free(working);
            working = malloc(length*sizeof(char));
            strncpy(working, augmented, length);
            if(next > pow(BASE,bit)){
				bit++;
                dict = realloc(dict,pow(BASE,bit)*sizeof(char*));
            }
        }
    }
}


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



void Output(int number, int bit,int *carryover){
    int reverse[bit];
    int to_bin = number;
    int count =0;
    while(to_bin != 0){
		reverse[count] = to_bin &1;
        to_bin >>= 1;
        count++;
    }
    int output = sizeof(carryover);
    for(int i = 0; i< count; i++){
		if(output != 8){
			carryover[output] = reverse[i];
            output++;
        }else{
			ReverseOrder(carryover);
            int blank[8];
            carryover = blank;
            carryover[0] = reverse[i];
            output = 1;
        }
    }
}

void ReverseOrder(int *carryover){
    int count = 0;
    for(int i = 0; i<8; i++){
		count += carryover[i]*pow(2,i);
    }
    fputc(count,stdout);
}