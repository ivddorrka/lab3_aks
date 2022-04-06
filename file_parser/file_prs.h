#ifndef HEADRFILE_H
#define HEADRFILE_H

#include <boost/program_options.hpp>
#include <string>
#include <iostream>

struct Arguments {
    std::string indir;
    std::string out_by_a;
    std::string out_by_n;
    int indexing_threads;

};

int collect_arguments(const std::string &filename, Arguments &args);

int assert_file_exist(const std::string &f_name);

#endif