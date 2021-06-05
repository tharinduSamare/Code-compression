#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <bitset>
#include <vector>
#include <bits/stdc++.h>
using namespace std;

const int DICTIONARY_SIZE = 16;
static unsigned int dictionary_2[DICTIONARY_SIZE];
static uint8_t RLE_MAX_SIZE = 8;
static uint8_t BITMASK_LENGTH = 4;
static uint8_t BIT_INDEX_LENGTH = 5;
static uint8_t DICTIONARY_INDEX_LENGTH = 4;

typedef struct compressed_data_t{
    unsigned int compressed_word;
    uint8_t compress_format;
}compressed_data_t;


void read_compressed_file(vector<compressed_data_t> &compressed_data_vect);
int string_to_int(string word, uint8_t length);
void decode_string_to_compressed_lines(string compressed_text, vector<compressed_data_t> &compressed_data_vect);

int main(){
    vector<compressed_data_t> compressed_data_vect;
    read_compressed_file(compressed_data_vect);
    unsigned int compressed_word_count = compressed_data_vect.size();
    for (auto& it:compressed_data_vect){
        cout << "format " << unsigned( it.compress_format) << " word " << it.compressed_word << endl;
    }

    return 0;
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
            dictionary_2[dictionary_index] = (unsigned int )string_to_int(compressed_line,32);
            dictionary_index ++;  
        }        
    }
    compressed_file.close();
    decode_string_to_compressed_lines(compressed_text,compressed_data_vect); // seperate compressed words and their compress formats
}

int string_to_int(string word, uint8_t length){
  int value = 0;
  for (int i=0;i<length;i++){
    value += stoi(word.substr(i,1))*(int)pow(2.0,(length-1-i));

  }
  return value;
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
        // cout << "format " << bitset<3>(format) << " word " << bitset <32>(compressed_word_size) << endl;

        index += compressed_word_size;    // go to the start of the next format + compressed word
    }
}

unsigned int bitmask_decompression(unsigned int compressed_word){

    ///// extracting each part of the word
    uint8_t first_mismatch_index = 31 - (compressed_word >> (BITMASK_LENGTH+DICTIONARY_INDEX_LENGTH)) & ((1<<BIT_INDEX_LENGTH)-1); // counting starts from left side
    uint8_t bitmask = (compressed_word >> DICTIONARY_INDEX_LENGTH) & ((1<<BITMASK_LENGTH)-1);
    uint8_t dictionary_index = compressed_word & ((1<<DICTIONARY_INDEX_LENGTH)-1);

    unsigned int uncompressed_word = dictionary_2[dictionary_index] ^ (bitmask << first_mismatch_index);

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

    unsigned int uncompressed_word = dictionary_2[dictionary_index] ^ (((1<<mismatch_count)-1)<<first_mismatch_index);
    return uncompressed_word;
}

unsigned int nonconsec_2bit_mismatch_decompression(unsigned int compressed_word){
    uint8_t first_mismatch_location = 31 - (compressed_word >> (DICTIONARY_INDEX_LENGTH+BIT_INDEX_LENGTH)) & ((1<<BIT_INDEX_LENGTH)-1);
    uint8_t second_mismatch_location = 31 - (compressed_word >> DICTIONARY_INDEX_LENGTH) & ((1<< BIT_INDEX_LENGTH)-1);
    uint8_t dictionary_index = compressed_word & ((1<<DICTIONARY_INDEX_LENGTH)-1);

    unsigned int mask = (1<< first_mismatch_location) | (1 << second_mismatch_location);
    unsigned int uncompressed_word = dictionary_2[dictionary_index] ^ mask;

    return uncompressed_word;
}

