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

TokenInfo::TokenInfo(){
    memset(this, 0, sizeof(TokenInfo));
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
    #define ptr file_data.data_ptr
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
    uint8_t* begin_block_ptr = nullptr;
    uint8_t* begin_token_ptr = nullptr;
    uint32_t block_size = 0;
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
        //If is last token, have no offset.
        if(scan_ptr-begin_block_ptr==block_size){
            TokenInfo token_info;
            token_info.actual_size=scan_ptr-begin_token_ptr;
            token_info.origin_size=literals.length();
            token_info.is_compressed=false;
            token_info.is_valid=true;
            token_info.token_str=literals;
            parse_stat.token_infos.push_back(token_info);
            return;
        }
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
        TokenInfo token_info;
        token_info.actual_size=scan_ptr-begin_token_ptr;
        token_info.is_compressed=true;
        token_info.origin_size=dep_token_str.length();
        token_info.is_valid=true;
        token_info.ref_length=total_match_length;
        token_info.ref_offeset=offset;
        token_info.token_str=dep_token_str;
        parse_stat.token_infos.push_back(token_info);
        log_parsed(dep_token_str);
    };
    while(1){
        //clear
        dep_block_str.clear();
        block_size=0;
        //get BlockSize
        get_uInt32(block_size_data);
        begin_block_ptr = scan_ptr;
        if(block_size_data==0x0){
            parse_stat.frame_size+=4;
            std::cout<<"INFO: End mark find.\n";
            break;
        }
        else{
            parse_stat.data_blocks_size+=4;
        }
        uint32_t _split_num = 0x80000000;
        block_size = block_size_data;
        bool is_compressed = true;
        if(block_size_data>_split_num){
            //uncompressed
            block_size -= _split_num;
            is_compressed = false;
        }
        log_parsed(block_size);
        log_parsed(is_compressed);
        //ParseData
        if(!is_compressed){
            for(int i = 0; i<block_size; ++i){
                get_byteD(tmp_byte);
                dep_block_str.push_back((char)tmp_byte);
            }
            TokenInfo token_info;
            token_info.is_compressed = false;
            token_info.origin_size=block_size;
            token_info.is_valid = true;
            token_info.actual_size=4+block_size;
            token_info.token_str=dep_block_str;
            parse_stat.token_infos.push_back(token_info);
            log_parsed(dep_block_str);
            dep_file<<dep_block_str;
        }
        else{
            //parse tokens.
            while(1){
                begin_token_ptr = scan_ptr;
                parse_token();
                log_parse_ok("One Token");
                ++parse_stat.token_count;
                if(scan_ptr-begin_block_ptr==block_size)break;
                //limited scan area, assert if segmetation fault.
                assert(scan_ptr-begin_block_ptr<=block_size);
            }
        }
        dep_file<< dep_block_str;
        if(testbit_byte(flag, 4)){
            get_uInt32D(block_checksum);
            log_parsed(block_checksum);
        }
        assert(scan_ptr - begin_block_ptr==block_size);
        log_parse_ok("One Block");
        ++parse_stat.block_count;
    }
    //Parse CheckSum.
    if(testbit_byte(flag, 2)){
        get_uInt32F(content_check_sum);
        log_parsed(content_check_sum);
    }
    dep_file.close();
    assert(scan_ptr-file_data.data_ptr==file_data.size);
    assert(parse_stat.data_blocks_size + parse_stat.frame_size==file_data.size);
    log_parse_ok("Parse Succeccfully ended");
}

bool TokenInfo::operator<(const TokenInfo& other)const{
    return token_str<other.token_str;
}

void Parser::dump_stat(const std::string& dump_token_file_name){
    std::cout<<"Stat:"<<"frame_size is " << parse_stat.frame_size <<".\n";
    std::cout<<"Stat:"<<"block_count is " << parse_stat.block_count <<".\n";
    std::cout<<"Stat:"<<"data_blocks_size is " << parse_stat.data_blocks_size <<".\n";
    std::cout<<"Stat:"<<"token_count is " << parse_stat.token_count <<".\n";
    //Dump token infos.
    //Check valid
    assert(parse_stat.token_count==parse_stat.token_infos.size());
    uint64_t total_token_sz = 0;
    for(auto& token: parse_stat.token_infos){
        assert(token.is_valid);
        total_token_sz+=token.actual_size;
    }
    std::cout<<"Stat:"<<"total_token_sz is " << total_token_sz <<".\n";
    std::cout<<"Stat: [OK] Check TokenInfos.\n";
    //dump token_infos.
    sort(parse_stat.token_infos.begin(), parse_stat.token_infos.end());
    std::ofstream dump_token_file = std::ofstream(dump_token_file_name);
    int token_cnt = 0;
    for(auto& token:parse_stat.token_infos){
        // dump_token_file<<"$Token"<<token_cnt++<<": ";
        dump_token_file<<token.token_str<<"\n";
    }
    std::cout<<"Stat: [OK] Dump sorted token infos into file: "<< dump_token_file_name << " .\n";
    dump_token_file.close();
}

int main(){
    const std::string file_name = "lineitem_50X2.csv.lz4";
    const std::string dep_file_name = file_name+".dep";
    const std::string dump_token_file_name=file_name+".tokens";
    Parser parser;
    parser.init(file_name);
    parser.parse_and_decompress(dep_file_name);
    parser.dump_stat(dump_token_file_name);
    return 0;
}