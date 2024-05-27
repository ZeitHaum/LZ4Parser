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
            if(i<seq.sequence_str.size()) {
                now_line.actual_size+=1.0F;
            }
            else {
                double comp_size = seq.ref.bitsize /(8.0F*seq.ref.str.size());
                now_line.compressed_bytes+=1.0F;
                now_line.actual_size += comp_size;
                now_line.ref_size += comp_size;
            }
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

void dump_double_vector(std::vector<double>& vec){
    std::string out = "[";
    for(double& d: vec){
        out += std::to_string(d);
        out += " ,";
    }
    std::cout<< out << "]\n";
}

void dump_line_infos(std::vector<SequenceInfo>* refs){
    //逐行分析
    std::vector<LineInfo> lines;
    get_line_infos(refs, lines);
    int line_count = 0;
    struct {
        std::vector<double> origin_size;
        std::vector<double> line_size;
        std::vector<double> accum_size;
        std::vector<double> compressed_bytes;
        std::vector<double> ref_size;
        void dump_all(){
            dump_double_vector(origin_size);
            dump_double_vector(line_size);
            dump_double_vector(accum_size);
            dump_double_vector(compressed_bytes);
            dump_double_vector(ref_size);
        }
    } diff_sat;

    #define add_accum(val, vec) if(vec.empty()){vec.push_back(val);}else{vec.push_back(vec.back()+val);}

    for(auto& line: lines){
        diff_sat.line_size.push_back(line.actual_size);
        add_accum(((double)line.str.size()), diff_sat.origin_size);
        add_accum(line.actual_size, diff_sat.accum_size);
        add_accum(line.compressed_bytes, diff_sat.compressed_bytes);
        add_accum(line.ref_size, diff_sat.ref_size);
    }

    diff_sat.dump_all();
    std::cout << "--------End line infos--------\n";
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
}

int main(){
    const std::string lowcomp_file_name = "../test/test2.csv.lz4";
    const std::string highcomp_file_name = "../test/test1.csv.lz4";
    Parser* parser_low = parse_file(lowcomp_file_name);
    Parser* parser_high = parse_file(highcomp_file_name);
    diff_reference(parser_low->get_seqinfo_ptr(), parser_high->get_seqinfo_ptr());
    // dump_line_infos(parser_low->get_seqinfo_ptr());
    // dump_line_infos(parser_high->get_seqinfo_ptr());
    parser_low->dump_diff_size();
    parser_high->dump_diff_size();
    parser_low->dump_stat();
    parser_high->dump_stat();
    delete parser_low;
    delete parser_high;
    return 0;
}