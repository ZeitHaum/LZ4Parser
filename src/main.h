#include "parser.h"
#include "test.h"
#include <map>

struct LineInfo{
    std::vector<SequenceInfo*> seqs;
    std::string str;
    double actual_size;
    LineInfo();
    void clear();
};