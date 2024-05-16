#include "parser.h"
#include "test.h"
#include <map>

struct LineInfo{
    std::vector<SequenceInfo*> seqs;
    std::string str;
    double actual_size;
    double compressed_bytes;
    double ref_size;
    LineInfo();
    void clear();
};