#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cassert>
#include <cstring>
#include <algorithm>
#include "config.h"
#include "test.h"

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
#ifdef ENABLE_PARSE_LOG
    #define log_parsed(var)  std::cout<<"INFO:"<<"Parsed variable "#var" "<<var<<".\n"
    #define log_parse_ok(str) std::cout<<"INFO: [OK] "<<"Parsed "<<str<<".\n";
#else 
    #define log_parsed(var)
    #define log_parse_ok(str)
#endif

struct FileData{
    uint8_t* data_ptr = nullptr;
    uint64_t size = 0;
    bool valid = true;
    void clear();
};

struct Reference{
    uint32_t offset;
    uint32_t length;
    std::string str;
    uint32_t bitsize;
    bool valid;
    bool is_diff;// extern attribute.
    Reference();
};

struct SequenceInfo{
    bool is_valid;
    bool is_compressed;
    uint64_t actual_size;
    uint64_t origin_size;
    Reference ref;
    std::string sequence_str;
    SequenceInfo();
    bool operator<(const SequenceInfo&)const;
    std::string get_full_str();
};

class Parser{
private:
    FileData file_data;
    struct {
        int frame_size = 0;
        int data_blocks_size = 0;
        int block_count = 0;
        int sequence_count = 0;
        std::vector<SequenceInfo> sequence_infos;
        int origin_size = 0;
    } parse_stat;
    std::string file_name;
    std::string restore_data(const std::string& , uint16_t , int );
    std::string restore_data_overlap(const std::string& , uint16_t , int );
public:
    Parser();
    ~Parser();
    void parse_and_decompress();
    void init(const std::string& file_name);
    void dump_stat();
    std::vector<SequenceInfo>* get_seqinfo_ptr();
    int get_origin_size();
    void dump_diff_size();
    int get_file_size();
};  
