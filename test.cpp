#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <bitset>
using namespace std;

const int DICTIONARY_SIZE = 16;
static unsigned int dictionary[DICTIONARY_SIZE];

typedef struct bitmask_compression_t{
  uint8_t bitmask_size;
  bool compressed;
  uint8_t first_mismatch_index;
  uint8_t bitmask;
  uint8_t dictionary_index;
}bitmask_compression_t;

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

unsigned int* Read_original(unsigned int *size);
void create_dictionary(unsigned int original_size, unsigned int original_data[]);
int string_to_int(string word);

bool bitmask_compression(unsigned int uncompressed_line, bitmask_compression_t *bcd);

bool direct_matching(unsigned int uncompressed_line, uint8_t *dictionary_index);

bool consecutive_bit_mismatch(unsigned int uncompressed_line, consecutive_bit_mismatch_t *cbm);

bool nonconsec_2bit_mismatch (unsigned int uncompressed_line, nonconsec_2bit_mismatch_t *n2bm);

bool RLE_compression(unsigned int index, unsigned int uncompressed_data[], uint8_t *repeat_count);


int main(){
  unsigned int original_size = 0; // size of the uncompressed dataset
  ///////// get original data /////
  unsigned int* dyn_arr = Read_original(&original_size);
  unsigned int original_data[original_size] = {0};

 
  for (int i=0;i<original_size;i++){
    original_data[i] = dyn_arr[i];
  }
  delete[] dyn_arr;
  
  /////// create the dictionary ///////
  ofstream dictionary_file("dictionary.txt");
  create_dictionary(original_size, original_data);
  if (dictionary_file.is_open()){
    for (int i=0;i<16; i++){
      dictionary_file << dictionary[i] << endl;
    }
  }
  dictionary_file.close();

  consecutive_bit_mismatch_t cbm ;
  cbm.mismatch_size = 1;

//   unsigned int  uncompressed_line = original_data[5]; 
//   bool compressed =  consecutive_bit_mismatch(uncompressed_line,&cbm);
//   std::cout << "compared line: " << std::bitset<32>(uncompressed_line ^ dictionary[cbm.dictionary_index]) << endl;
//   std::cout << "uncompressed line: " << std::bitset<32>(uncompressed_line) << endl;
//   std::cout << "dictionary line: " << std::bitset<32>(dictionary[cbm.dictionary_index]) << endl;
//   cout << "first_mismatch_index " << unsigned(cbm.first_mismatch_index) << endl;
//   cout << "dictionary_index " << unsigned(cbm.dictionary_index) << endl;

  for (int i=0;i<1;i++){
    unsigned int uncompressed_line = original_data[i];
    bool compressed =  consecutive_bit_mismatch(uncompressed_line,&cbm);
    if (compressed){
      std::cout << "compared line: " << std::bitset<32>(uncompressed_line ^ dictionary[cbm.dictionary_index]) << endl;
      std::cout << "uncompressed line: " << std::bitset<32>(uncompressed_line) << endl;
      std::cout << "dictionary line: " << std::bitset<32>(dictionary[cbm.dictionary_index]) << endl;
      cout << "first_mismatch_index " << unsigned(cbm.first_mismatch_index) << endl;
      cout << "dictionary_index " << unsigned(cbm.dictionary_index) << endl;
    }
    else{
      std::cout << "uncompressed line: " << std::bitset<32>(uncompressed_line) << endl;
      cout << "can not do" << endl;
    }
    cout << endl;
  }

  return 0;
}

bool consecutive_bit_mismatch(unsigned int uncompressed_line, consecutive_bit_mismatch_t *cbm){
  
  uint8_t mask_size = cbm->mismatch_size;
  bool compressed = 0;
  uint8_t dictionary_index = 0;
  uint8_t first_mismatch_index = 0;
  unsigned int mask = 0;
  for(uint8_t i=0;i<DICTIONARY_SIZE;i++){
    
    unsigned int compared_line = uncompressed_line ^ dictionary[i];  // find mismatches
    for (uint8_t index=mask_size-1;index<32;index++){
        cout << unsigned(index) << endl;
      mask = ((1<<mask_size)-1)<<(index-mask_size+1);        // make lsb bits 1s and shift left
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
  // std::cout << "compared line: " << std::bitset<32>(uncompressed_line ^ dictionary[dictionary_index]) << endl;
  // std::cout << "uncompressed line: " << std::bitset<32>(uncompressed_line) << endl;
  // std::cout << "dictionary line: " << std::bitset<32>(dictionary[dictionary_index]) << endl;
  // cout << "compressed " << compressed << endl;
  // cout << "first_mismatch_index " << unsigned(first_mismatch_index) << endl;
  // cout << "dictionary_index " << unsigned(dictionary_index) << endl;

  cbm->compressed = compressed;
  cbm->dictionary_index = dictionary_index;
  cbm->first_mismatch_index = first_mismatch_index;

  return compressed;
}


unsigned int* Read_original(unsigned int *size){
  string uncompressed_line;
  ifstream original_file("original.txt");
  //////// find the size of original dataset ///
  while(getline(original_file,uncompressed_line)){
    *size = *size + 1;
  }
  original_file.close();

  unsigned int * original_data = new unsigned int [*size]();
  unsigned int index = 0;

  original_file.open("original.txt");
  while (getline (original_file, uncompressed_line)) {
    original_data[index] = (unsigned int )string_to_int(uncompressed_line);
    index++;   
  }
  original_file.close();
  return original_data;  
}

int string_to_int(string word){
  int value = 0;
  for (int i=0;i<32;i++){
    value += stoi(word.substr(i,1))*(int)pow(2.0,(31-i));
  }
  return value;
}

void create_dictionary(unsigned int original_size, unsigned int original_data[]){
  int seen[original_size] = {0};
  int unique_count=0;
  int unique_values[original_size];
  int repeat_counts[original_size];

  /////////////identify the unique values and their counts/////////////
  for (unsigned int i=1;i<original_size;i++){
    if (seen[i] == 0){
      unsigned int count=0;
      for (unsigned int j=i; j<original_size; j++){
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
  return;
}
