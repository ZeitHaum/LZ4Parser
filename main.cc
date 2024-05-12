#include "main.hh"

void FileData::clear(){
    delete[] data_ptr;
    data_ptr = nullptr;
    size = 0;
    valid = false;
}

Parser::~Parser(){
    file_data.clear();
}

void Parser::init(const std::string& file_name){
    // 打开文件流
    std::ifstream file_stream(file_name, std::ios::binary);
    if (!file_stream.is_open()) {
        std::cerr << "Error: Failed to open file " << file_name << std::endl;
        file_data.data_ptr = nullptr;
        file_data.size = 0;
        file_data.valid = false;
    }
    // 获取文件大小
    file_stream.seekg(0, std::ios::end);
    file_data.size = file_stream.tellg();
    file_stream.seekg(0, std::ios::beg);
    // 分配内存并读取文件数据
    file_data.data_ptr = new uint8_t[file_data.size];
    file_stream.read(reinterpret_cast<char*>(file_data.data_ptr), file_data.size);
    // 关闭文件流
    file_stream.close();
    file_data.valid = true;
    std::cout<<"INFO:" << "init input files, read data Bytes:" << file_data.size << "\n";
}

DataBlockInfo::DataBlockInfo(){
    memset(this, 0, sizeof(DataBlockInfo));
}

std::string Parser::restore_data(const std::string& origin_str, uint16_t offset, int match_length){
    //check
    assert(offset<=origin_str.length());
    assert(match_length<=(origin_str.length()-offset+1));
    std::string sub_str = origin_str.substr(origin_str.length() - offset, match_length);
    return sub_str;
}

//Core Function.
void Parser::parse_and_decompress(const std::string& dep_file_name){
    std::ofstream dep_file = std::ofstream(dep_file_name);
    uint8_t* scan_ptr = ptr;
    //parse Magic Number.
    get_uInt32F(magic_num);
    assert(magic_num==0x184D2204);
    log_parsed(magic_num);
    log_parse_ok("Magic Number");
    //parse File descriptor
    get_byteF(flag);
    get_byteF(BDbyte);
    if(testbit_byte(flag, 3)){
        get_uInt64F(content_size);
        log_parsed(content_size);
    }
    if(testbit_byte(flag, 0)){
        get_uInt32F(dic_id);
        log_parsed(dic_id);
    }
    get_byteF(header_checksum);
    log_parsed((int)header_checksum);
    log_parse_ok("File descriptor");
    /*
     parse DataBlock.
    */
    std::string dep_block_str;
    auto parse_token = [&](){
        //get literals length
        get_byteD(token);
        uint8_t prev_literals_length = get_high_nibble(token);
        int total_literals_length = (int)(prev_literals_length);
        if(prev_literals_length==0x0F){
            while(1){
                get_byteD(tmp_byte);
                total_literals_length+=(int)tmp_byte;
                if(tmp_byte<0xFF) break;
            }
        }
        log_parsed(total_literals_length);
        std::string literals;
        for(int i = 0;i<total_literals_length; ++i){
            get_byteD(tmp_literals);
            literals.push_back((char)(tmp_literals));
        }
        dep_block_str+=literals;
        log_parsed(literals);
        //get offset
        get_uInt16D(offset);
        log_parsed(offset);
        assert(offset<=0xFFFF);
        //get match lenghth.
        uint8_t prev_match_length = get_low_nibble(token);
        int total_match_length = (int)(prev_match_length);
        if(prev_match_length==0x0F){
            while(1){
                get_byteD(tmp_byte);
                total_match_length+=(int)tmp_byte;
                if(tmp_byte<0xFF) break;
            }
        }
        total_match_length+=4;// more 4 byte is enabled.
        log_parsed(total_match_length);
        std::string comp_str =  restore_data(dep_block_str, offset, total_match_length);
        dep_block_str+=comp_str;
        std::string dep_token_str=literals+comp_str;
        log_parsed(dep_token_str);
    };
    while(1){
        //get BlockSize
        get_uInt32D(block_size_data);
        if(block_size_data==0x0){
            std::cout<<"INFO: End mark find.\n";
            break;
        }
        uint32_t _split_num = 0x80000000;
        uint32_t block_size = block_size_data;
        bool is_compressed = true;
        if(block_size_data>_split_num){
            //uncompressed
            block_size -= _split_num;
            is_compressed = false;
        }
        log_parsed(block_size);
        log_parsed(is_compressed);
        //ParseData
        DataBlockInfo data_block_info;
        dep_block_str.clear();
        if(!is_compressed){
            data_block_info.is_compressed = false;
            data_block_info.origin_size=block_size;
            data_block_info.is_valid = true;
            data_block_info.actual_size=4+block_size;
            for(int i = 0; i<block_size; ++i){
                get_byteD(tmp_byte);
                dep_block_str.push_back((char)tmp_byte);
            }
            log_parsed(dep_block_str);
            dep_file<<dep_block_str;
        }
        else{
            //parse tockens.
            uint32_t now_dep_len = 0;
            while(1){
                int begin_sz = parse_stat.data_blocks_size;
                parse_token();
                log_parse_ok("One Token");
                now_dep_len += (uint32_t)(parse_stat.data_blocks_size - begin_sz);
                if(now_dep_len>=block_size){
                    break;
                }
            }
        }
        dep_file<< dep_block_str;
        log_parse_ok("One Block");
        if(testbit_byte(flag, 4)){
            get_uInt32D(block_checksum);
            log_parsed(block_checksum);
        }
        break;
    }
    //Parse CheckSum.
    if(testbit_byte(flag, 2)){
        get_uInt32F(content_check_sum);
        log_parsed(content_check_sum);
    }
    dep_file.close();
    get_uInt16(extra_bytes);
    std::cout<<extra_bytes<<"\n";
    assert(scan_ptr-file_data.data_ptr==file_data.size);
    assert(parse_stat.data_blocks_size + parse_stat.frame_size==file_data.size);
    log_parse_ok("Parse Succeccfully ended.");
}

void Parser::dump_stat(){
    std::cout<<"Stat:"<<"frame_size is " << parse_stat.frame_size <<".\n";
    std::cout<<"Stat:"<<"data_blocks_size is " << parse_stat.data_blocks_size <<".\n";
    //Dump dataBlock infos.
}

int main(){
    const std::string file_name = "lineitem_50X2.csv.lz4";
    const std::string dep_file_name = "lineitem_50X2.csv.lz4.dep";
    Parser parser;
    parser.init(file_name);
    parser.parse_and_decompress(dep_file_name);
    parser.dump_stat();
    return 0;
}