#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
using namespace std;

const int DICTIONARY_SIZE = 16;
const int MAX_SIZE = 1024;
static unsigned int original_data[MAX_SIZE];
static unsigned int dictionary[16];


int Read_original();
int create_dictionary(int original_size);
int string_to_int(string word);

int main(){
  int original_size;
  original_size = Read_original();
  cout << original_size << endl;
  create_dictionary(original_size);
  for (int i=0;i<16; i++){
    cout << dictionary[i] <<endl;
  }

  // cout << string_to_int("11011111101011111111111011101111");
  return 0;
}


int Read_original(){
  string uncompressed_line;
  ifstream original_file("original.txt");

  int i = 0;
  while (getline (original_file, uncompressed_line)) {
    original_data[i] = (unsigned int )string_to_int(uncompressed_line);
    i++;
  }
  original_file.close();
      
  return i;
}

int string_to_int(string word){
  int value = 0;
  for (int i=0;i<32;i++){
    value += stoi(word.substr(i,1))*(int)pow(2.0,i);
  }
  return value;
}

int create_dictionary(int original_size){
  int seen[original_size] = {0};
  int unique_count=0;
  int unique_values[original_size];
  int repeat_counts[original_size];

  /////////////identify the unique values and their counts/////////////
  for (int i=1;i<original_size;i++){
    if (seen[i] == 0){
      int count=0;
      for (int j=i; j<original_size; j++){
        if (original_data[i] == original_data[j]){
          count++;
          seen[j] = 1;
        }
      }
      repeat_counts[unique_count] = count;
      unique_values[unique_count] = original_data[i];
      unique_count ++;
    }    
  }
  //////////// create dictionary from unique values ////////////
  for (int i=0;i<DICTIONARY_SIZE;i++){

    int max_count = repeat_counts[0];
    int max_index = 0;
    
    for (int j=0; j<unique_count;j++){
      if (max_count < repeat_counts[j]){
        max_count = repeat_counts[j];
        max_index = j;
      }
    }

    repeat_counts[max_index] = -1;  // remove the current max value
    dictionary[i] = unique_values[max_index];
  }
  return 0;
}


// int bit_mask_compression(int line){
//   int compressed_line;
//   for 


//   return compressed_line;
// } 

void compress_line(int uncompressed_line){
  int compressed_line = 0;
  unsigned int compress_method = 0;
  unsigned int dictionary_index = 0;
  bool compressed = 0;
  ///////////////// checking direct matching ///////////////
  for (int i=0;i<DICTIONARY_SIZE;i++){
    if (uncompressed_line == dictionary[i]){
      compressed_line = dictionary[i];
      compress_method = 7;     // '111' for direct match
      compressed = 1;
      break;
    }
  }
  if (compressed){
    return;
  }

  ////////// checking 1 bit mismatch ////////////////
  unsigned int compared_val;
  for (int i=0;i<DICTIONARY_SIZE;i++){
    compared_val = uncompressed_line ^ dictionary[i];

    unsigned int mismatch_index;

    for (int index = 31;index >-1;index--){
      if ((compared_val & 1) == 1){
        compared_val = compared_val >>1;
        if (compared_val == 0){
          mismatch_index = index;
          dictionary_index = i;
          compressed = 1;
          compress_method = 3; // '011' for 1 bit mismatch
          break;
        }
        else{
          break;   // more than 1 bit mismatch
        }
      }
      else{
        compared_val = compared_val >> 1;  // to check for next bit
      }
    }
    if (compressed){
      break;
    }
  }

  if (compressed){
    return;
  }

  //////// check 2 bit consecutive mismatches ///////////
  for (int i=0;i<DICTIONARY_SIZE;i++){
    compared_val = uncompressed_line ^ dictionary[i];

    unsigned int mismatch_index;

    for (int index = 30;index >-1;index--){
      if ((compared_val & 3) == 1){
        compared_val = compared_val >>2;
        if (compared_val == 0){
          mismatch_index = index;
          dictionary_index = i;
          compressed = 1;
          compress_method = 4; // '100' for 2 bit consecutive mismatch
          break;
        }
        else{
          break;   // more than 1, consecutive 2 bit mismatch
        }
      }
      else{
        compared_val = compared_val >> 1;  // to check for next 2 bit
      }
    }
    if (compressed){
      break;
    }
  }

  if (compressed){
    return;
  }

  //////// check 4 consecutive bit mismatches ///////////
  for (int i=0;i<DICTIONARY_SIZE;i++){
    compared_val = uncompressed_line ^ dictionary[i];

    unsigned int mismatch_index;

    for (int index = 28;index >-1;index--){
      if ((compared_val & 15) == 1){
        compared_val = compared_val >>4;
        if (compared_val == 0){
          mismatch_index = index;
          dictionary_index = i;
          compressed = 1;
          compress_method = 5; // '101' for 4 bit consecutive mismatch
          break;
        }
        else{
          break;   // more than 1, consecutive 2 bit mismatch
        }
      }
      else{
        compared_val = compared_val >> 1;  // to check for next 4 bit
      }
    }
    if (compressed){
      break;
    }
  }

  if (compressed){
    return;
  }

  //////// 4 bit bitmask compression //////////
  unsigned int BITMASK_SIZE = 4;
  unsigned int bitmask = 0;      // for a correct bitmask atleast 1 bit should be '1'
  for (int i=0;i<DICTIONARY_SIZE;i++){
    unsigned int compared_line = uncompressed_line ^ dictionary[i];  // take the LSB 4 bits 
    unsigned int mismatch_count = 0;
    bool mismatch_found = 0;
    unsigned int first_mismatch_index = 0;
    for (int index=31;index>=0;index--){
      if(compared_line & (1<<index)){    //check bit by bit for a mismatch
        if(mismatch_found == 0){
          first_mismatch_index = index;   // identify first mismatch index
          mismatch_found = 1;
        }
        mismatch_count += 1;
      }
      if ((mismatch_count >4) || (mismatch_found && ((first_mismatch_index-index)>=BITMASK_SIZE))){
        break;    // can not use 4 bit 
      }

    }
    if(mismatch_found){
      unsigned int bitmask_end_index = first_mismatch_index-BITMASK_SIZE+1;
      bitmask = (compared_line >> bitmask_end_index) & ((1 << BITMASK_SIZE)-1);
      compressed = 1;
      break;
    }
  }


   

  return;
}
