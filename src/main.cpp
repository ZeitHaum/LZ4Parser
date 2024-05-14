#include "main.h"

int main(){
    const std::string file_name = "../test/lineitem_50X2.csv.lz4";
    const std::string dep_file_name = file_name+".dep";
    const std::string dump_sequence_file_name=file_name+".sequences";
    Parser parser;
    parser.init(file_name);
    parser.parse_and_decompress(dep_file_name);
    parser.dump_stat(dump_sequence_file_name);
    return 0;
}