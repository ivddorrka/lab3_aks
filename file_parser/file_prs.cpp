#include <iostream>
#include "file_prs.h"
#include <boost/program_options.hpp>
#include <filesystem>


namespace opt = boost::program_options;


int collect_arguments(const std::string &filename, Arguments &args) {
    opt::options_description desc("All options");
    desc.add_options()
            ("indir", opt::value<std::string>())
            ("out_by_a", opt::value<std::string>())
            ("out_by_n", opt::value<std::string>())
            ("indexing_threads", opt::value<int>())

            ;
    opt::variables_map vm;
    try {
        opt::store(
                opt::parse_config_file<char>(filename.c_str(), desc, true),
                vm
        );
    } catch (const opt::reading_file& e) {
        std::cout<< "ERROR!!" << e.what() << "\n";
    }

    try {
        opt::store(
                opt::parse_config_file<char>(filename.c_str(), desc), vm
        );
    } catch (const opt::reading_file& e) {
        std::cout<< "ERROR!!" << e.what() << "\n";

    }
    if (vm.count("indir")) {
    args.indir = vm["indir"].as<std::string>();
    }else {
        std::cerr << " No indir arg-t specified" << "\n";
        return 3;
    }


    if (vm.count("out_by_a")) {
        args.out_by_a = vm["out_by_a"].as<std::string>();
    }else {
        std::cerr << " No out_by_a arg-t specified" << "\n";
        return 3;
    }

    if (vm.count("out_by_n")) {
        args.out_by_n = vm["out_by_n"].as<std::string>();
    }else {
        std::cerr << " No out_by_n arg-t specified" << "\n";
        return 3;
    }


    if (vm.count("indexing_threads")) {
        args.indexing_threads = vm["indexing_threads"].as<int>();
    }else {
        std::cerr << " No indexing_threads arg-t specified" << "\n";
        return 3;
    }


    return 0;
}

int assert_file_exist(const std::string &f_name) {
    if (!std::filesystem::exists(f_name)) {
        std::cerr << "No such file" << "\n";
        return 2;
    }
    return 0;
}
