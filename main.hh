#include <iostream>
#include <string>
#include <fstream>
#include <vector>

struct FileData{
    uint8_t* data_ptr = nullptr;
    uint64_t size = 0;
    bool valid = true;
    void clear();
};

enum class DataBlockType{
    DATA, REFERENCE,
};

struct DataBlock{
    DataBlockType data_block_type;
    uint64_t actual_size;
    uint64_t origin_size;
    uint64_t ref_offeset;
    uint64_t ref_length;
};

class Parser{
private:
    FileData file_data;
    struct {
        int frame_size;
        int data_blocks_size;
        std::vector<DataBlock> data_blocks;
    } parse_stat;
    std::ofstream dep_file;
public:
    ~Parser();
    void parse_and_decompress(const std::string& dep_file_name);
    void init(const std::string& file_name);
    void dump_stat();
};  
