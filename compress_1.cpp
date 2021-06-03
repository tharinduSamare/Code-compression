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

unsigned int* Read_original(unsigned int *size);
void create_dictionary(unsigned int original_size, unsigned int original_data[]);
int string_to_int(string word);

bool bitmask_compression(unsigned int uncompressed_line, bitmask_compression_t *bcd);

bool direct_matching(unsigned int uncompressed_line, direct_matching_t *dm);

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
  

  bitmask_compression_t bcd;
  bcd.bitmask_size = 4;

  
  for (int i=0; i< original_size;i++){
    unsigned int uncompressed_line = original_data[i];
    bool compressed = bitmask_compression(uncompressed_line, &bcd);
    std::cout << "compared line: " << std::bitset<32>(uncompressed_line ^ dictionary[bcd.dictionary_index]) << endl;
    if (compressed == 1){      
      std::cout << "uncompressed line: " << std::bitset<32>(uncompressed_line) << endl;
      std::cout << "dictionary line: " << std::bitset<32>(dictionary[bcd.dictionary_index]) << endl;
      cout << "compressed " << compressed << endl;
      cout << "bitmask " << unsigned(bcd.bitmask) << endl;
      cout << "first_mismatch_index " << unsigned(bcd.first_mismatch_index) << endl;
      cout << "dictionary_index " << unsigned(bcd.dictionary_index) << endl;
    }
    else{
      cout << "can not compress" << endl;
    }
    cout <<  endl;
  }
  
  

  return 0;
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

bool bitmask_compression(unsigned int uncompressed_line, bitmask_compression_t *bcd){
  
  uint8_t bitmask_size = bcd->bitmask_size;
  uint8_t bitmask = 0;      // for a correct bitmask atleast 1 bit should be '1'
  bool compressed = 0;
  uint8_t dictionary_index = 0;
  uint8_t first_mismatch_index = 0;
  uint8_t last_mismatch_index = 0;

  for (uint8_t i=0;i<DICTIONARY_SIZE;i++){
    unsigned int compared_line = uncompressed_line ^ dictionary[i];  // find mismatches
    uint8_t mismatch_count = 0;
    bool mismatch_found = 0;
    first_mismatch_index = 0;

    for (int index=31;index>=0;index--){
      if(compared_line & (1<<index)){    //check bit by bit for a mismatch
        if(mismatch_found == 0){
          first_mismatch_index = index;   // identify first mismatch index
          mismatch_found = 1;
        }
        last_mismatch_index = index;
        mismatch_count += 1;
      }
    }
    if (mismatch_found && (mismatch_count <= bitmask_size) && (first_mismatch_index-last_mismatch_index < bitmask_size)){
      uint8_t bitmask_end_index = first_mismatch_index-bitmask_size+1;
      if (first_mismatch_index < bitmask_size-1){ // when first bits of mask are 0s 
        bitmask_end_index = 0;
      }
      bitmask = (compared_line >> bitmask_end_index) & ((1 << bitmask_size)-1);
      compressed = 1;
      dictionary_index = i; // get the dictionary index of correct dictionary word
      break;
    }
  }
  // std::cout << "compared line: " << std::bitset<32>(uncompressed_line ^ dictionary[dictionary_index]) << endl;
  // std::cout << "uncompressed line: " << std::bitset<32>(uncompressed_line) << endl;
  // std::cout << "dictionary line: " << std::bitset<32>(dictionary[dictionary_index]) << endl;
  // cout << "compressed " << compressed << endl;
  // cout << "bitmask " << unsigned(bitmask) << endl;
  // cout << "first_mismatch_index " << unsigned(first_mismatch_index) << endl;
  // cout << "dictionary_index " << unsigned(dictionary_index) << endl;

  ////////// return the compression details
  bcd->compressed = compressed;
  bcd->bitmask = bitmask;
  bcd->first_mismatch_index = first_mismatch_index;
  bcd->dictionary_index = dictionary_index;

  return compressed;
}

bool direct_matching(unsigned int uncompressed_line, direct_matching_t *dm){
  
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

bool consecutive_bit_mismatch(unsigned int uncompressed_line, consecutive_bit_mismatch_t *cbm){
  
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

bool nonconsec_2bit_mismatch (unsigned int uncompressed_line, nonconsec_2bit_mismatch_t *n2bm){
  
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
      compressed = 0;            // if the previous one is RLF compressed don't rle compress
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