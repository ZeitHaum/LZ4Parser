#pragma once
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstring>
#include <algorithm>

//define region
#define get_byte(var) uint8_t var=*(scan_ptr);++scan_ptr
#define get_uInt16(var) uint16_t var=*((uint16_t*)(scan_ptr));scan_ptr+=2
#define get_uInt32(var) uint32_t var=*((uint32_t*)(scan_ptr));scan_ptr+=4
#define get_uInt64(var) uint64_t var=*((uint64_t*)(scan_ptr));scan_ptr+=8
#define get_byteF(var) get_byte(var);parse_stat.frame_size+=1
#define get_uInt16F(var) get_uInt16(var);parse_stat.frame_size+=2
#define get_uInt32F(var) get_uInt32(var);parse_stat.frame_size+=4
#define get_uInt64F(var) get_uInt64(var);parse_stat.frame_size+=8
#define get_byteD(var) get_byte(var);parse_stat.data_blocks_size+=1
#define get_uInt16D(var) get_uInt16(var);parse_stat.data_blocks_size+=2
#define get_uInt32D(var) get_uInt32(var);parse_stat.data_blocks_size+=4
#define get_uInt64D(var) get_uInt64(var);parse_stat.data_blocks_size+=8
#define testbit_byte(byte, i) ((byte>>i)&0x01)
#define get_high_nibble(byte) ((byte>>4)&0x0F)
#define get_low_nibble(byte) (byte&0x0F)
#define log_parsed(var)  std::cout<<"INFO:"<<"Parsed variable "#var" "<<var<<".\n"
#define log_parse_ok(str) std::cout<<"INFO: [OK] "<<"Parsed "<<str<<".\n";

struct FileData{
    uint8_t* data_ptr = nullptr;
    uint64_t size = 0;
    bool valid = true;
    void clear();
};

struct SequenceInfo{
    bool is_valid;
    bool is_compressed;
    uint64_t actual_size;
    uint64_t origin_size;
    uint64_t ref_offeset;
    uint64_t ref_length;
    std::string sequence_str;
    SequenceInfo();
    bool operator<(const SequenceInfo&)const;
};

class Parser{
private:
    FileData file_data;
    struct {
        int frame_size;
        int data_blocks_size;
        int block_count;
        int sequence_count;
        std::vector<SequenceInfo> sequence_infos;
    } parse_stat;
    std::string restore_data(const std::string& , uint16_t , int );
    std::string restore_data_overlap(const std::string& , uint16_t , int );
public:
    ~Parser();
    void parse_and_decompress(const std::string& dep_file_name);
    void init(const std::string& file_name);
    void dump_stat(const std::string&);
};  
