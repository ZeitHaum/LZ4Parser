#include "main.h"

Parser* parse_file(const std::string& file_name){
    Parser* parser = new Parser();
    parser->init(file_name);
    parser->parse_and_decompress();
    return parser;
}

void diff_reference(std::vector<SequenceInfo>* refs1, std::vector<SequenceInfo>*refs2){
    
}

int main(){
    const std::string highcomp_file_name = "../test/lineitem_50X10.csv.lz4";
    const std::string lowcomp_file_name = "../test/lineitem_50X2.csv.lz4";
    Parser* parser_low = parse_file(lowcomp_file_name);
    Parser* parser_high = parse_file(highcomp_file_name);
    diff_reference(parser_low->get_seqinfo_ptr(), parser_high->get_seqinfo_ptr());
    parser_low->dump_stat();
    parser_high->dump_stat();
    delete parser_low;
    delete parser_high;
    return 0;
}