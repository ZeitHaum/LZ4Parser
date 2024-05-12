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

//Core Function.
void Parser::parse_and_decompress(const std::string& dep_file_name){

}

void Parser::dump_stat(){

}

int main(){
    const std::string file_name = "lineitem_50X2.csv.lz4";
    const std::string dep_file_name = "lineitem_50X2.csv.lz4.dep";
    Parser parser;
    parser.init(file_name);
    parser.parse_and_decompress(dep_file_name);
    return 0;
}