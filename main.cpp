#include <iostream>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <filesystem>
#include <regex>
#include <fstream>
#include <map>
#include <boost/algorithm/string.hpp>
#include "file_parser/file_prs.h"


template<typename T>
class mt_que{
public:
    mt_que() = default;
    ~mt_que() = default;
    mt_que(const mt_que&) = delete;
    mt_que& operator=(const mt_que&) = delete;

    void enque(const T& a){
        {

            std::unique_lock<std::mutex> lock(m_m);

            while (que_m.size() > maxsize ) {

                cv_m.wait(lock);

            }

            que_m.push_back(a);
        }
        cv_m.notify_one();
    }

    T deque(){
        std::unique_lock<std::mutex> lock(m_m);
        while (que_m.empty() ) {
            cv_m.wait(lock);
        }
        auto a = que_m.front();
        que_m.pop_front();
        cv_m.notify_one();
        return a;
    }

    void set_maxsize(int mxsize) {
        std::unique_lock<std::mutex> lock(m_m);
        maxsize = mxsize;
    }

    size_t get_size() const {
        std::lock_guard<std::mutex> lock(m_m);
        return que_m.size();
    }

private:
    std::deque<T> que_m;
    mutable std::mutex m_m;
    std::condition_variable cv_m;
    int maxsize{};
};




int read(const std::string &inpFile, std::string &result) {

    std::ifstream inFile(inpFile);

    if (!inFile){
        std::cerr << "Error opening input file" << "\n";
        return 1;
    }
    std::stringstream strStream;
    if (inFile.is_open() && inFile.good()){
        strStream << inFile.rdbuf();
    } else {
        std::cerr << "Error reading input file " << "\n";
        return 2;
    }
    result = strStream.str();
    inFile.close();
    return 0;
}


void splittedIntoVec(const std::string &str, std::map<std::string, std::size_t> &mapProm){

    std::vector<std::string> v1;
    boost::split(v1,str,isspace);
    size_t startNum = 1;
    for (auto & i : v1) {
        auto toFind = mapProm.find(i);
        if ( toFind != mapProm.end() ) {
            toFind->second += 1;
        } else {
            mapProm.insert ( std::pair<std::string,std::size_t>(i, startNum) );
        }
    }
}



int main(int argc, char** argv) {

    ////////////////////////////////////////////////////////////
    // collecting arguments
    std::string fl;
    if (argc <2) {
        assert_file_exist("./index.cfg");
        fl = "./index.cfg";
    } else {
        fl = argv[1];
    }


    Arguments arguments{};


    collect_arguments(fl, arguments);

    std::string indir = arguments.indir;
    int index_threads = arguments.indexing_threads;

    std::string out_by_a = arguments.out_by_a;
    std::string yfromInit = arguments.out_by_n;



    ////////////////////////////////////////////////////////////
    //  here file contents as strings are passed to filesTOstring Queue
    //  fileNameTXTqueue - queue for txt filenames from the given directory and all its subdirectories


    mt_que<std::filesystem::path> fileNameTXTqueue;
    mt_que<std::string> filesTOstring;
    size_t numfiles = 0;
    std::string path = "./data/";
    fileNameTXTqueue.set_maxsize(10000);
    filesTOstring.set_maxsize(10000);
    std::string pattern = ".+\\.txt$";
    for (const auto & file : std::filesystem::recursive_directory_iterator(path)){
        std::string subject = file.path();
        const std::regex e(pattern);
        if (regex_match(subject, e)) {
            fileNameTXTqueue.enque(subject);
            numfiles++;
        }
    }

    std::string permlink;
    std::string fileName;

    for (size_t i =0; i< numfiles; ++i) {

        fileName = fileNameTXTqueue.deque();
        read(fileName, permlink);
        filesTOstring.enque(permlink);

    }



    /////////////////////////////////////////////////////////////////


    //    counting here in map from each file
    //    and then merging into one map
    std::map<std::string, std::size_t> mainMap;

    //    std::map<std::string, std::size_t> promMap;
    //    vector where maps for each

    std::vector<std::map<std::string, std::size_t>> vecForPromMaps;;
    size_t vecSize = 0;
    while (filesTOstring.get_size() > 0) {
        std::string str = filesTOstring.deque();
        std::map<std::string, std::size_t> promMap;

        // promMap is std::map<std::string, size_t> stores
        // from the each txt file - word and the number of its repetitions
        splittedIntoVec(str, promMap);

        vecForPromMaps.push_back(promMap);
        vecSize++;
    }


    ///////////////////////// merging all maps into one
    for (size_t i =0; i< vecSize; ++i) {
        for ( const auto &[key, value]: vecForPromMaps[i] ) {
            auto toFind = mainMap.find(key);
            if ( toFind != mainMap.end() ) {
                toFind->second += value;
            } else {
                mainMap.insert ( std::pair<std::string,std::size_t>(key, value) );
            }
        }
    }

    for ( const auto &[key, value]: mainMap ) {
        std::cout << "Key: " << key << ", Value: " << value << '\n';
    }

    return 0;
}