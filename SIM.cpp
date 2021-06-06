#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <bitset>
#include <vector>
#include <bits/stdc++.h>
using namespace std;

const int DICTIONARY_SIZE = 16;
static unsigned int dictionary[DICTIONARY_SIZE];
static uint8_t RLE_MAX_SIZE = 8;
static uint8_t BITMASK_LENGTH = 4;
static uint8_t BIT_INDEX_LENGTH = 5;
static uint8_t DICTIONARY_INDEX_LENGTH = 4;

static unsigned int testing_index = 1;

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

typedef struct compressed_data_t{
    unsigned int compressed_word;
    uint8_t compress_format;
}compressed_data_t;

int string_to_int(string word, uint8_t length);

string uint8_to_string(uint8_t value, uint8_t bit_count);
string uint_to_string(unsigned int value);

void compression_top();
unsigned int* Read_original(unsigned int *size);
void create_dictionary(unsigned int original_size, unsigned int original_data[]);
bool bitmask_compression(unsigned int uncompressed_line, bitmask_compression_t *bc);
bool direct_matching(unsigned int uncompressed_line, uint8_t *dictionary_index);
bool consecutive_bit_mismatch(unsigned int uncompressed_line, consecutive_bit_mismatch_t *cbm);
bool nonconsec_2bit_mismatch (unsigned int uncompressed_line, nonconsec_2bit_mismatch_t *n2bm);
bool RLE_compression(unsigned int index, unsigned int uncompressed_data[], unsigned int array_size, uint8_t *repeat_count);
vector<string> compression (unsigned int uncompressed_data[], unsigned int uncompressed_array_size);
void create_compressed_file(vector<string> &compressed_data);

void decompression_top();
void read_compressed_file(vector<compressed_data_t> &compressed_data_vect);
void decode_string_to_compressed_lines(string compressed_text, vector<compressed_data_t> &compressed_data_vect);
unsigned int bitmask_decompression(unsigned int compressed_word);
unsigned int consecutive_bit_mismatch_decompression(unsigned int compressed_word, uint8_t format);
unsigned int nonconsec_2bit_mismatch_decompression(unsigned int compressed_word);
void decompression (unsigned int compressed_word, uint8_t format, vector<unsigned int>&decompressed_data);
void create_decompressed_file(vector <unsigned int> &decompressed_data);

int main ( int argc, char *argv[] ){
  if (strcmp(argv[1],"1")==0){
      compression_top();
  }
  else if(strcmp(argv[1],"2")==0){
      decompression_top();
  }

  return 0;
}

int string_to_int(string word, uint8_t length){
  int value = 0;
  for (int i=0;i<length;i++){
    value += stoi(word.substr(i,1))*(int)pow(2.0,(length-1-i));
  }
  return value;
}

string uint8_to_string(uint8_t value, uint8_t bit_count){
  string string_val = "";
  for (int8_t i=bit_count-1;i>=0; i--){
    string_val += to_string((value & (1<<i))>>i) ;
  }
  return string_val;
}

string uint_to_string(unsigned int value){
  string string_val = "";
  for (int8_t i=31;i>=0; i--){
    string_val += to_string((value & (1<<i))>>i) ;
  }
  return string_val;
}

/////////////// compression functions /////////////////////////

void compression_top(){
  unsigned int original_size = 0; // size of the uncompressed dataset
  ///////// get original data /////
  unsigned int* dyn_arr = Read_original(&original_size);
  unsigned int original_data[original_size] = {0};

 
  for (int i=0;i<original_size;i++){
    original_data[i] = dyn_arr[i];
  }
  delete[] dyn_arr;
  
  create_dictionary(original_size, original_data);

  vector<string> compressed_data;
  compressed_data =  compression(original_data,original_size);

  ///////// write compressed data & dictionary to a file ////////
  create_compressed_file(compressed_data);
  return;
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
    original_data[index] = (unsigned int )string_to_int(uncompressed_line,32);
    index++;   
  }
  original_file.close();
  return original_data;  
}

void create_dictionary(unsigned int original_size, unsigned int original_data[]){
  vector<unsigned int> unique_values;
  vector<int> unique_value_frequency;
  unsigned int unique_count = 0;
  ///// identify unique values //////////
  for (int i=0;i<original_size;i++){
    int j;
    for (j=0;j<=i;j++){
      if (original_data[i]==original_data[j]){
        break;
      }
    }
    if (i==j){
      unique_values.push_back(original_data[i]);
      unique_count++;
    }
  }

  ///// identify frequency of each unique value
  unsigned int repeat_count = 0;
  for (int i=0;i<unique_count;i++){
    repeat_count = 0;
    for (int j=0; j<original_size;j++){
      if (original_data[j] == unique_values[i]){
        repeat_count++;
      }
    }
    unique_value_frequency.push_back(repeat_count);
  }

  ////////////// create dictionary from unique values ////////////
  for (int i=0;i<DICTIONARY_SIZE;i++){

    int max_count = unique_value_frequency[0];
    int max_index = 0;
    
    for (int j=0; j<unique_count;j++){
      if (max_count < unique_value_frequency[j]){
        max_count = unique_value_frequency[j];
        max_index = j;
      }
    }

    unique_value_frequency[max_index] = -1;  // remove the current max value
    dictionary[i] = unique_values[max_index];
  }
  return;
}

bool bitmask_compression(unsigned int uncompressed_line, bitmask_compression_t *bc){
  
  uint8_t bitmask_size = bc->bitmask_size;
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
  ////////// return the compression details
  bc->compressed = compressed;
  bc->bitmask = bitmask;
  bc->first_mismatch_index = 31-first_mismatch_index; //start counting from left
  bc->dictionary_index = dictionary_index;

  return compressed;
}

bool direct_matching(unsigned int uncompressed_line, uint8_t *dictionary_index){
  
  *dictionary_index = 0;
  bool compressed = 0;
  for (uint8_t i=0;i<DICTIONARY_SIZE;i++){
    if (uncompressed_line == dictionary[i]){
      compressed = 1;
      *dictionary_index = i;   // return the index of matched dictionary value
      break;
    }
  }
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
    for (uint8_t index=mask_size-1;index<32;index++){
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
  cbm->compressed = compressed;
  cbm->dictionary_index = dictionary_index;
  cbm->first_mismatch_index = 31 - first_mismatch_index; // start counting from left

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
  n2bm->first_mismatch_index = 31 - first_mismatch_index;   //start counting from left
  n2bm-> second_mismatch_index = 31 - second_mismatch_index;  //start counting from left

  return compressed;
}

bool RLE_compression(unsigned int index, unsigned int uncompressed_data[], unsigned int array_size, uint8_t *repeat_count){
  bool compressed = 0;
  if (index == 0){
    compressed = 0;
    return compressed;
  }

  if (uncompressed_data[index] != uncompressed_data[index-1]){
    compressed = 0;
    return compressed;
  }
  
  *repeat_count = 0;
  unsigned int value = uncompressed_data[index-1];  // the value before current index
  for (uint8_t i = 0;i<RLE_MAX_SIZE;i++){
    if (index + i >= array_size){
      break;
    }
    else if (uncompressed_data[index + i] == value){
      *repeat_count = *repeat_count + 1;
    }
    else{           
      break;
    }
  }
  compressed = 1;
  return compressed;
}

vector<string> compression (unsigned int uncompressed_data[], unsigned int uncompressed_array_size){

  vector <string> compressed_data;
  ///////////  create initial details of each compression type //////////
  // bitmask compression 
  bitmask_compression_t bc;
  bc.bitmask_size = 4;
    // direct match 
  uint8_t dictioanry_index;
  // consecutive 1,2,4 bit mismatch compression
  consecutive_bit_mismatch_t cbm_1, cbm_2, cbm_4;
  cbm_1.mismatch_size = 1;
  cbm_2.mismatch_size = 2;
  cbm_4.mismatch_size = 4;
  // anywhere 2 bit mismatch compression
  nonconsec_2bit_mismatch_t n2bm;
  // RLE compression
  uint8_t repeat_count;

  unsigned int index = 0;
  bool compressed = 0;
  bool RLE_compressed_last_word = 0;
  while (index < uncompressed_array_size){
    compressed = 0;

    //////////// RLE compression ///////
    if (!RLE_compressed_last_word){  // RLE compression happens only when last compression is not RLE
      compressed = RLE_compression(index, uncompressed_data, uncompressed_array_size, &repeat_count);
      if (compressed){
        compressed_data.push_back("001" + uint8_to_string((repeat_count-1),3)) ; //0 means 1 repeat, 1 means 2 repeats
        cout << index << " 001"  << endl;
        index += repeat_count;
        RLE_compressed_last_word = 1;
        
        
        continue;
      }
    }
    else{
      RLE_compressed_last_word = 0;
    }

    ////////// direct matching /////////
    compressed = direct_matching(uncompressed_data[index], &dictioanry_index);
    if (compressed){
      compressed_data.push_back("111"+uint8_to_string(dictioanry_index,4));
      cout << index << " 111" << endl;
      index++;     
       
      continue;
    }
    
    ///////// 1 bit mismatch compression ////////
    compressed = consecutive_bit_mismatch(uncompressed_data[index],&cbm_1);
    if (compressed){
      compressed_data.push_back("011"+uint8_to_string(cbm_1.first_mismatch_index,5)+uint8_to_string(cbm_1.dictionary_index,4));
      cout << index << " 011" << endl;
      index++;
      
      continue;
    }

    //////// 2 bit mismatch compression //////////
    compressed = consecutive_bit_mismatch(uncompressed_data[index],&cbm_2);
    if (compressed){
      compressed_data.push_back("100"+uint8_to_string(cbm_2.first_mismatch_index,5)+uint8_to_string(cbm_2.dictionary_index,4));
      cout << index << " 100" << endl;
      index++;
      continue;
    }

    //////// 4 bit mismatch compression //////////
    compressed = consecutive_bit_mismatch(uncompressed_data[index],&cbm_4);
    if (compressed){
      compressed_data.push_back("101"+uint8_to_string(cbm_4.first_mismatch_index,5)+uint8_to_string(cbm_4.dictionary_index,4));
      cout << index << " 101" << endl;
      index++;
      continue;
    }

    //////// bitmask compression //////////
    compressed = bitmask_compression(uncompressed_data[index],&bc);
    if (compressed){
      compressed_data.push_back("010"+uint8_to_string(bc.first_mismatch_index,5)+uint8_to_string(bc.bitmask,4)+uint8_to_string(bc.dictionary_index,4));
      cout << index << " 010" << endl;
      index++;
      continue;
    }

    /////// 2 bit mismatch anywhere compression  //////////
    compressed = nonconsec_2bit_mismatch(uncompressed_data[index],&n2bm);
    if (compressed){
      compressed_data.push_back("110"+uint8_to_string(n2bm.first_mismatch_index,5)+uint8_to_string(n2bm.second_mismatch_index,5)+uint8_to_string(n2bm.dictionary_index,4));
      cout << index << " 110" << endl;
      index ++;
      continue;
    }

    //////// no compression happens
    compressed_data.push_back("000"+ uint_to_string(uncompressed_data[index]));
    cout << index << " 000" << endl;
    index++;
  }
  
  return compressed_data;
}

void create_compressed_file(vector<string> &compressed_data){
  string compressed_text = "";
  
  for (int i=0;i<compressed_data.size(); i++){
    compressed_text += compressed_data[i];
  }
  unsigned int length = compressed_text.length();
  uint8_t extra_0_count = 32 - length % 32;  // add extra zeros at the end of the compressed data to match the size
  string extra_0s = "";
  for (int i=0;i<extra_0_count;i++){
    extra_0s += "0";
  }
  compressed_text += extra_0s;

  length = compressed_text.length();  // find the length after adding extra 0s
  ofstream compressed_file("cout.txt");
  for (int i=0;i<length;i=i+32){
    compressed_file << compressed_text.substr(i,32) << endl;    // write the com pressed data
  }
  compressed_file << "xxxx" << endl;     // middle "xxxx" sign

  for (int i=0; i<DICTIONARY_SIZE; i++){
    compressed_file << bitset<32>(dictionary[i]) << endl;  // write dictionary entries
  }
  compressed_file.close();
  return;
}

/////////////////////// decompression functions ///////////////////////////
void decompression_top(){
    vector<compressed_data_t> compressed_data_vect;
    vector<unsigned int> decompressed_data;

    read_compressed_file(compressed_data_vect);
    unsigned int compressed_word_count = compressed_data_vect.size();
    unsigned int decompressed_word = 0;

    for (auto& compressed_obj:compressed_data_vect){
        decompression(compressed_obj.compressed_word,compressed_obj.compress_format,decompressed_data);
    }
    create_decompressed_file(decompressed_data);
    return;
}

void read_compressed_file(vector<compressed_data_t> &compressed_data_vect){
    string compressed_text = "";
    string compressed_line;
    ifstream compressed_file("compressed.txt");
    uint8_t dictionary_index = 0;

    bool second_part = 0;
    while (getline (compressed_file, compressed_line)){
        if (compressed_line.compare("xxxx")==0){
            second_part = 1;
            continue;
        }
        if (second_part==0){
            compressed_text += compressed_line;
        }
        else{
            dictionary[dictionary_index] = (unsigned int )string_to_int(compressed_line,32);
            dictionary_index ++;  
        }        
    }
    compressed_file.close();
    decode_string_to_compressed_lines(compressed_text,compressed_data_vect); // seperate compressed words and their compress formats
}

void decode_string_to_compressed_lines(string compressed_text, vector<compressed_data_t> &compressed_data_vect){
    unsigned int index = 0;
    unsigned int compressed_word = 0;
    string compressed_word_text = "";
    unsigned int compressed_text_size = compressed_text.size();
    string format_text = "";
    uint8_t format = 0;
    uint8_t compressed_word_size = 0;

    compressed_data_t cd;

    while (index<compressed_text_size-2){
        format_text = compressed_text.substr(index,3);   // first 3 bits of each compress line 
        if (format_text.compare("000")==0){   // no compression
            compressed_word_size = 32;
        }
        else if (format_text.compare("001")==0){  // RLE compression
            compressed_word_size = 3;
        }
        else if (format_text.compare("010")==0){  //bitmask compression
            compressed_word_size = 13;
        }
        else if (format_text.compare("011")==0){  //1 bit consecutive mismatch 
            compressed_word_size = 9;
        }else if (format_text.compare("100")==0){  //2 bit consecutive mismatch
            compressed_word_size = 9;
        }
        else if (format_text.compare("101")==0){  //4 bit consecutive mismatch
            compressed_word_size = 9;
        }
        else if (format_text.compare("110")==0){  // 2 bit mismatch anywhere
            compressed_word_size = 14;
        }
        else {
            compressed_word_size = 4;  // direct match
        }
        if (index+3+compressed_word_size>=compressed_text_size){        // check for the end of the array    
            break;                     
        }

        index += 3;
        compressed_word_text = compressed_text.substr(index,compressed_word_size);
        format = string_to_int(format_text,3);
        compressed_word = string_to_int(compressed_word_text,compressed_word_size);  // implicitly cast to uint8_t
        cd.compress_format = format;
        cd.compressed_word = compressed_word;
        compressed_data_vect.push_back(cd);

        index += compressed_word_size;    // go to the start of the next format + compressed word
    }
}

unsigned int bitmask_decompression(unsigned int compressed_word){

    ///// extracting each part of the word
    uint8_t first_mismatch_index = 31 - (compressed_word >> (BITMASK_LENGTH+DICTIONARY_INDEX_LENGTH)) & ((1<<BIT_INDEX_LENGTH)-1); // counting starts from left side
    uint8_t bitmask = (compressed_word >> DICTIONARY_INDEX_LENGTH) & ((1<<BITMASK_LENGTH)-1);
    uint8_t dictionary_index = compressed_word & ((1<<DICTIONARY_INDEX_LENGTH)-1);

    unsigned int uncompressed_word = dictionary[dictionary_index] ^ (bitmask << (first_mismatch_index-BITMASK_LENGTH+1));

    return uncompressed_word;
}

unsigned int consecutive_bit_mismatch_decompression(unsigned int compressed_word, uint8_t format){
    uint8_t mismatch_count = 0;
    switch (format){
        case(3):mismatch_count = 1;
        break;
        case(4):mismatch_count = 2;
        break;
        case(5):mismatch_count = 4;
        break;
    }

    //////// extract parts of the compressed word
    uint8_t first_mismatch_index = 31 - (compressed_word >> DICTIONARY_INDEX_LENGTH) & ((1<<BIT_INDEX_LENGTH)-1);
    uint8_t dictionary_index = compressed_word & ((1<<DICTIONARY_INDEX_LENGTH)-1);

    unsigned int uncompressed_word = dictionary[dictionary_index] ^ (((1<<mismatch_count)-1)<<(first_mismatch_index-mismatch_count+1));
     
    return uncompressed_word;
}

unsigned int nonconsec_2bit_mismatch_decompression(unsigned int compressed_word){
    uint8_t first_mismatch_location = 31 - (compressed_word >> (DICTIONARY_INDEX_LENGTH+BIT_INDEX_LENGTH)) & ((1<<BIT_INDEX_LENGTH)-1);
    uint8_t second_mismatch_location = 31 - (compressed_word >> DICTIONARY_INDEX_LENGTH) & ((1<< BIT_INDEX_LENGTH)-1);
    uint8_t dictionary_index = compressed_word & ((1<<DICTIONARY_INDEX_LENGTH)-1);

    unsigned int mask = (1<< first_mismatch_location) | (1 << second_mismatch_location);
    unsigned int uncompressed_word = dictionary[dictionary_index] ^ mask;

    return uncompressed_word;
}

void decompression (unsigned int compressed_word, uint8_t format, vector<unsigned int>&decompressed_data){
    unsigned int decompressed_word = 0;
    if (format == 0){
        decompressed_word = compressed_word;
        decompressed_data.push_back(decompressed_word);
    }
    else if (format == 1){
        decompressed_word = decompressed_data.back();
        for (int i=0;i<(compressed_word+1);i++){
            decompressed_data.push_back(decompressed_word);
        }
    }
    else if (format == 2){
        decompressed_word = bitmask_decompression(compressed_word);
        decompressed_data.push_back(decompressed_word);
    }
    else if ((format==3)||(format==4)||(format==5)){
        decompressed_word = consecutive_bit_mismatch_decompression(compressed_word,format);
        decompressed_data.push_back(decompressed_word);
    }
    else if (format == 6){
        decompressed_word = nonconsec_2bit_mismatch_decompression(compressed_word);
        decompressed_data.push_back(decompressed_word);
    }
    else {
        decompressed_word = dictionary[compressed_word];
        decompressed_data.push_back(decompressed_word);
    }
}

void create_decompressed_file(vector <unsigned int> &decompressed_data){
    
    ofstream decompressed_file("dout.txt");
    for (auto decompressed_word:decompressed_data){
        decompressed_file << bitset<32>(decompressed_word) << endl;
    }
    decompressed_file.close();
}