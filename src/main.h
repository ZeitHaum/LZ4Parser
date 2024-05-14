#include "parser.h"
#include "test.h"

struct LineInfo{
    std::vector<SequenceInfo*> seqs;
    std::string str;
    LineInfo();
    void clear();
};