#include "main.h"

Parser* parse_file(const std::string& file_name){
    Parser* parser = new Parser();
    parser->init(file_name);
    parser->parse_and_decompress();
    return parser;
}

LineInfo::LineInfo(){
    clear();
}

void LineInfo::clear(){
    str="";
    seqs.clear();
}

void get_line_infos(std::vector<SequenceInfo>* seqs, std::vector<LineInfo>& line_info){
    LineInfo now_line;
    line_info.clear();
    for(auto& seq: (*seqs)){
        std::string now_str = seq.sequence_str+seq.ref.str;
        now_line.seqs.push_back(&seq);
        for(int i = 0; i<now_str.size(); ++i){
            char c = now_str[i];
            now_line.str.push_back(c);
            if(c=='\n' || c==EOF){
                //line_end.
                line_info.push_back(now_line);
                now_line = LineInfo();
                if(i!= now_str.size() - 1){
                    now_line.seqs.push_back(&seq);
                }
            }
        }
    }
}

void sign_matched_line(LineInfo& line1, LineInfo& line2){
    //双指针
    assert(!line1.seqs.empty());
    assert(!line2.seqs.empty());
    assert(line1.str.size() == line2.str.size());
    //扫描 line, 找到所有引用
    std::vector<int> inwhichRef[2];
    inwhichRef[0].resize(line1.str.size(), -1);
    inwhichRef[1].resize(line2.str.size(), -1);
    auto scan_line = [&](std::vector<int>& sign_ref, LineInfo& line){
        struct{
            int line_ind = 0;
            int seq_ind = 0;
            int in_seqstr_ind = -1;
            std::string seq_str;
        } scanner;
        //initialize;
        std::string first_str = line.seqs[0]->get_full_str();
        for(int i = 0; i<first_str.size(); ++i){
            if(first_str[i]=='\n'){
                scanner.in_seqstr_ind = i+1;
                break;
            }
        }
        if(scanner.in_seqstr_ind==-1) scanner.in_seqstr_ind=0;
        scanner.seq_str = line.seqs[0]->get_full_str();
        //initialize finished.
        while(1){
            //handle
            SequenceInfo& seq_info = *(line.seqs[scanner.seq_ind]);
            if(scanner.in_seqstr_ind>seq_info.sequence_str.size()){
                // is ref
                sign_ref[scanner.line_ind]=scanner.seq_ind;
            }
            //update
            ++scanner.in_seqstr_ind;
            ++scanner.line_ind;
            if(scanner.line_ind==line.str.size()){
                break;
            }
            if(scanner.in_seqstr_ind==scanner.seq_str.size()){
                ++scanner.seq_ind;
                scanner.in_seqstr_ind=0;
                scanner.seq_str=line.seqs[scanner.seq_ind]->get_full_str();
            }
        }
    };
    scan_line(inwhichRef[0], line1);
    scan_line(inwhichRef[1],line2);
    //sign
    for(int i = 0; i<line1.str.size(); ++i){
        if(inwhichRef[0][i]*inwhichRef[1][i]<=0){
            if(inwhichRef[1][i]==-1){
                line1.seqs[inwhichRef[0][i]]->ref.is_diff = true;
            }
            else{
                line2.seqs[inwhichRef[1][i]]->ref.is_diff = true;
            }
        }
        else{
            if(inwhichRef[0][i]+inwhichRef[1][i]>=0 && line1.seqs[inwhichRef[0][i]]->ref.bitsize != line2.seqs[inwhichRef[1][i]]->ref.bitsize){
                line1.seqs[inwhichRef[0][i]]->ref.is_diff = true;
                line2.seqs[inwhichRef[1][i]]->ref.is_diff = true;
            }
        }
    }
}

void diff_reference(std::vector<SequenceInfo>* refs1, std::vector<SequenceInfo>*refs2){
    std::vector<LineInfo> line_infos1;
    std::vector<LineInfo> line_infos2;
    get_line_infos(refs1, line_infos1);
    get_line_infos(refs2, line_infos2);

    /*** test get_line_info ***/
    TEST_get_line_info("../test/lineitem_50X2.csv")

    for(int i = 0; i<line_infos1.size(); ++i){
        for(int j = 0; j<line_infos2.size(); ++j){
            if(line_infos1[i].str == line_infos2[j].str){
                //Match
                sign_matched_line(line_infos1[i], line_infos2[j]);
                break;
            }
        }
    }
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