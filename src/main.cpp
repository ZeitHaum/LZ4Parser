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
            //update
            char c = now_str[i];
            now_line.str.push_back(c);
            if(i<seq.sequence_str.size()) now_line.actual_size+=1.0F;
            else now_line.actual_size += seq.ref.bitsize /(8.0F*seq.ref.str.size());
            //transfor
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
    std::map<SequenceInfo, int> seq_mp;
    for(auto& seq:(*refs1)){
        if(seq_mp.find(seq)==seq_mp.end()){
            seq_mp[seq] = 1;
        }
        else{
            seq_mp[seq]++;
        }
    }
    for(auto& seq:(*refs2)){
        if(seq_mp.find(seq)==seq_mp.end()){
            seq.ref.is_diff = true;
        }
        else{
            if((--seq_mp[seq])==0){
                seq_mp.erase(seq);
            }
        }
    }
    for(auto& seq:(*refs1)){
        if(seq_mp.find(seq)!=seq_mp.end()){
            seq.ref.is_diff = true;
            if((--seq_mp[seq])==0){
                seq_mp.erase(seq);
            }
        }
    }

    //逐行分析
    std::vector<LineInfo> lines1;
    std::vector<LineInfo> lines2;
    get_line_infos(refs1, lines1);
    get_line_infos(refs2, lines2);
    int line_count = 0;
    double line1_accum = 0.0;
    double line2_accum = 0.0;
    struct DiffSat{
        double line1_size;
        double line2_size;
        double accum1_size;
        double accum2_size;
    };

    std::vector<DiffSat> diff_stats;
    
    for(auto& line1: lines1){
        for(auto& line2: lines2){
            if(line1.str == line2.str){
                ++line_count;
                line1_accum += line1.actual_size;
                line2_accum += line2.actual_size;
                std::cout <<"Line "<<line_count<<" Matched. Occupied space(Bytes):" << line1.actual_size <<";" << line2.actual_size << ", total size(Bytes) "<<line1_accum <<";"<<line2_accum<<".\n"; 
                diff_stats.push_back({line1.actual_size, line2.actual_size, line1_accum, line2_accum});
                break;
            }
        }
    }

    //Out
    std::string line1_size_str = "\"line1_size\":[";
    std::string line2_size_str = "\"line2_size\":[";
    std::string accum1_size_str = "\"accum1_size\":[";
    std::string accum2_size_str = "\"accum2_size\":[";
    for(auto& stat:diff_stats){
        line1_size_str+= std::to_string(stat.line1_size) + ", ";
        line2_size_str+= std::to_string(stat.line2_size) + ", ";
        accum1_size_str+= std::to_string(stat.accum1_size) + ", ";
        accum2_size_str+= std::to_string(stat.accum2_size) + ", ";
    }
    line1_size_str += "]";
    line2_size_str += "]";
    accum1_size_str += "]";
    accum2_size_str += "]";
    std::cout << line1_size_str << ",\n";
    std::cout << line2_size_str << ",\n";
    std::cout << accum1_size_str << ",\n";
    std::cout << accum2_size_str << ",\n";

}

int main(){
    const std::string highcomp_file_name = "../test/lineitem_50X10.csv.lz4";
    const std::string lowcomp_file_name = "../test/lineitem_50X2.csv.lz4";
    Parser* parser_low = parse_file(lowcomp_file_name);
    Parser* parser_high = parse_file(highcomp_file_name);
    diff_reference(parser_low->get_seqinfo_ptr(), parser_high->get_seqinfo_ptr());
    std::cout<<"Diff: space of parser_low is "<< parser_low->get_diff_size() *1.0F/8.0F<< "Bytes. \n"; 
    std::cout<<"Diff: space of parser_high is "<< parser_high->get_diff_size() *1.0F/8.0F<<"Bytes. \n"; 
    parser_low->dump_stat();
    parser_high->dump_stat();
    delete parser_low;
    delete parser_high;
    return 0;
}