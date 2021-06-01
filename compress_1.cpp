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



typedef struct bitmask_compression_t{
  uint8_t bitmask_size;
  bool compressed;
  uint8_t first_mismatch_index;
  uint8_t bitmask;
  uint8_t dictionary_index;
}bitmask_compression_t;

typedef struct direct_matching_t{
  bool compressed;
  uint8_t dictionary_index;
}direct_matching_t;


typedef struct consecutive_bit_mismatch_t{
  bool compressed;
  uint8_t mismatch_size;
  uint8_t dictionary_index;
  uint8_t first_mismatch_index;
}consecutive_bit_mismatch_t;

typedef struct nonconsec_2bit_mismatch_t{
  bool compressed;
  uint8_t dictionary_index;
  uint8_t first_mismatch_index;
  uint8_t second_mismatch_index;
} nonconsec_2bit_mismatch_t;

bool bitmask_compression(unsigned int uncompressed_line, unsigned int dictionary[DICTIONARY_SIZE], 
                        uint8_t DICTIONARY_SIZE, bitmask_compression_t *bcd){
  
  uint8_t bitmask_size = bcd->bitmask_size;
  uint8_t bitmask = 0;      // for a correct bitmask atleast 1 bit should be '1'
  bool compressed = 0;
  uint8_t dictionary_index = 0;
  uint8_t first_mismatch_index = 0;
  for (uint8_t i=0;i<DICTIONARY_SIZE;i++){
    unsigned int compared_line = uncompressed_line ^ dictionary[i];  // find mismatches
    uint8_t mismatch_count = 0;
    bool mismatch_found = 0;
    first_mismatch_index = 0;

    for (int index=31;index>=bitmask_size-1;index--){
      if(compared_line & (1<<(index-1))){    //check bit by bit for a mismatch
        if(mismatch_found == 0){
          first_mismatch_index = index;   // identify first mismatch index
          mismatch_found = 1;
        }
        mismatch_count += 1;
      }
      if ((mismatch_count >4) || (mismatch_found && ((first_mismatch_index-index)>=bitmask_size))){
        break;    // can not use n bit bitmask
      }
    }
    ////// when bitmask compression applicable //////////
    if(mismatch_found){
      uint8_t bitmask_end_index = first_mismatch_index-bitmask_size+1;
      bitmask = (compared_line >> bitmask_end_index) & ((1 << bitmask_size)-1);
      compressed = 1;
      dictionary_index = i; // get the dictionary index of correct dictionary word
      break;
    }
  }
  ////////// return the compression details
  bcd->compressed = compressed;
  bcd->bitmask = bitmask;
  bcd->first_mismatch_index = first_mismatch_index;
  bcd->dictionary_index = dictionary_index;

  return compressed;
}

bool direct_matching(unsigned int uncompressed_line, unsigned int dictionary[DICTIONARY_SIZE], 
                     uint8_t DICTIONARY_SIZE, direct_matching_t *dm){
  
  uint8_t dictionary_index = 0;
  bool compressed = 0;
  for (uint8_t i=0;i<DICTIONARY_SIZE;i++){
    if (uncompressed_line == dictionary[i]){
      compressed = 1;
      dictionary_index = i;
      break;
    }
  }
  ////// set values in structure ///
  dm->compressed = compressed;
  dm->dictionary_index = dictionary_index;
  return compressed;
}

bool consecutive_bit_mismatch(unsigned int uncompressed_line, unsigned int dictionary[DICTIONARY_SIZE], 
                        uint8_t DICTIONARY_SIZE, consecutive_bit_mismatch_t *cbm){
  
  uint8_t mask_size = cbm->mismatch_size;
  bool compressed = 0;
  uint8_t dictionary_index = 0;
  uint8_t first_mismatch_index = 0;
  unsigned int mask = 0;

  for(uint8_t i=0;i<DICTIONARY_SIZE;i++){
    unsigned int compared_line = uncompressed_line ^ dictionary[i];  // find mismatches
    for (uint8_t index=31;index>=mask_size-1;index--){
      uint8_t mask_end_index = index-mask_size+1;
      mask = ((1<<mask_size)-1)<<(index-mask_size);        // make lsb bits 1s and shift left
      if (compared_line == mask){
        compressed = 1;
        dictionary_index = i;
        first_mismatch_index = index;
        break;
      }
    }
    if (compressed){
      break;
    }
  }
  ////// update structure values //////
  cbm->compressed = compressed;
  cbm->dictionary_index = dictionary_index;
  cbm->first_mismatch_index = first_mismatch_index;

  return compressed;
}

bool nonconsec_2bit_mismatch (unsigned int uncompressed_line, unsigned int dictionary[DICTIONARY_SIZE], 
                              uint8_t DICTIONARY_SIZE, nonconsec_2bit_mismatch_t *n2bm){
  
  bool compressed = 0;
  uint8_t dictionary_index = 0;
  uint8_t first_mismatch_index = 0;
  uint8_t second_mismatch_index = 0;
  unsigned int mask;

  for (uint8_t i=0; i<DICTIONARY_SIZE;i++){
    unsigned int compared_line = uncompressed_line ^ dictionary[i];  // find mismatches
    for (uint8_t j=0;j<=30;j++){
      for (uint8_t k=j+1;k<=31;k++){
        mask = (1<<(j)) | (1<<(k));
        if (compared_line == mask){
          compressed = 1;
          dictionary_index = i;
          first_mismatch_index = k;   // left side mismatch
          second_mismatch_index = j;  // right side mismatch
          break;
        }
      }
      if (compressed){
        break;
      }
    }
    if (compressed){
      break;
    }
  }

  ////// update struct values ////
  n2bm->compressed = compressed;
  n2bm->dictionary_index = dictionary_index;
  n2bm->first_mismatch_index = first_mismatch_index;
  n2bm-> second_mismatch_index = second_mismatch_index;

  return compressed;
}

bool RLE_compression(unsigned int index, unsigned int uncompressed_data[], uint8_t *repeat_count){
  bool compressed = 0;
  if (index == 0){
    compressed = 0;
    return compressed;
  }
  if (index>1){
    if (uncompressed_data[index-1] == uncompressed_data[index-2]){
      compressed = 0;
      return compressed;
    }
  }
  if (uncompressed_data[index] != uncompressed_data[index-1]){
    compressed = 0;
    return compressed;
  }
  
  *repeat_count = 0;
  unsigned int value = uncompressed_data[index];
  for (uint8_t i = 1;i<=8;i++){
    if (dictionary[index+i] == value){
      *repeat_count ++;
    }
    else{           
      break;
    }
  }
  compressed = 1;
  return compressed;
}