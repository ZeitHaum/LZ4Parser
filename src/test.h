#ifndef _TEST_H_
#define _TEST_H_
#include "config.h"

#ifdef ENABLE_TEST
    #define TEST_ok(var) std::cout<<"TEST: [OK] "#var".\n";

    #define TEST_restore_data_overlap() \
    std::string test_str = "1";\
    assert(restore_data_overlap(test_str, 1, 8)=="11111111");\
    test_str = "124";\
    assert(restore_data_overlap(test_str, 2, 4)=="2424");\
    TEST_ok(restore_data_overlap)

    #define TEST_get_line_info(file_name) std::ifstream origin_file = std::ifstream(file_name);\
    std::string  origin_line = "";\
    for(LineInfo& line_info:line_infos1){\
        std::getline(origin_file, origin_line, '\n');\
        origin_line+='\n';\
        assert(line_info.str==origin_line);\
    }\
    TEST_ok(get_line_info)

#else
    #define TEST_ok(var)
    #define TEST_restore_data_overlap()
    #define TEST_get_line_info()
#endif
#endif


