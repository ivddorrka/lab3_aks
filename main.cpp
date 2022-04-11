#include <iostream>
#include <condition_variable>
#include <mutex>
#include <deque>
#include <filesystem>
#include <regex>
#include <fstream>
#include <map>
#include <thread>
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


void splittedtomap(const std::string &str, std::map<std::string, std::size_t> &mapProm){

    std::vector<std::string> v1;
    boost::split(v1,str,isspace);
    size_t startNum = 1;
    for (auto & i : v1) {
        std::transform(i.begin(), i.end(), i.begin(),
                   [](unsigned char c){ return std::tolower(c); });
        auto toFind = mapProm.find(i);
        if ( toFind != mapProm.end() ) {
            toFind->second += 1;
        } else {
            mapProm.insert ( std::pair<std::string,std::size_t>(i, startNum) );
        }
    }
}


// sort by second -> sort by numbers
bool sortbysec(const std::pair<std::string, size_t> &a,
                 const std::pair<std::string, size_t> &b)
{
    return (a.second < b.second);
}


// comparison by the word - to sort alphabetically
bool sortbyfirst(const std::pair<std::string, size_t> &a,
                 const std::pair<std::string, size_t> &b)
{
    return (a.first < b.first);
}

// write to file out_by_a which in alphabetic order
void writeToFileA(size_t vecSize, const std::vector<std::string> &wordA, const std::map<std::string, std::size_t> &mainMap, const std::string &filePathA){
    std::ofstream myfile;
    size_t count;
    myfile.open(filePathA);
    for (size_t j=0; j<vecSize; ++j) {
        auto toFind = mainMap.find(wordA[j]);
        count = toFind->second;
        myfile << wordA[j] << " : " << count << "\n";
    }
    myfile.close();

}





// Write to file in ascending order - if some words happen the same amount of times ->
// they will be written in alphabetic order

void writeToFileB(size_t vecSize, const std::vector<std::pair<std::string, std::size_t>> &wordB, const std::map<std::string, std::size_t> &mainMap, const std::string &filePathB){
    std::vector<std::vector<std::pair<std::string, size_t>>> vecSplitted;
    std::vector<std::pair<std::string, std::size_t>> promOne;

    promOne.emplace_back(std::make_pair(wordB[0].first, wordB[0].second));

    for (size_t i =1; i < vecSize; ++i) {

        if (wordB[i-1].second == wordB[i].second) {
            promOne.emplace_back(std::make_pair(wordB[i].first, wordB[i].second));
            if (i == vecSize-1) {
                vecSplitted.emplace_back(promOne);
            }
        } else {
            vecSplitted.emplace_back(promOne);
            promOne.clear();
            promOne.emplace_back(std::make_pair(wordB[i].first, wordB[i].second));
            if (i == vecSize-1) {
                vecSplitted.emplace_back(promOne);
            }
        }
    }

    std::ofstream myfile;
//    size_t count;
    myfile.open(filePathB);

    for (auto & j : vecSplitted) {

        std::sort(j.begin(), j.end(), sortbyfirst);

        for (auto & pairIn : j) {

            myfile << pairIn.first << " : " << pairIn.second << "\n";

        }
    }
    myfile.close();
}


bool compareString (std::string &s1, std::string &s2) {
    if (s1<s2) {
        return true;
    }
    return false;
}



// sometimes in .cfg file parameters start with " and end with it
// this would spoil the searching in directory process
// and correct file saving for further usage

void stringRefactor (std::string &str) {
    size_t lenn = str.length();
    std::string resStr;
    for (size_t i = 0; i < lenn; ++i) {
        if (str[i] != '"'){
            resStr.push_back(str[i]);
        }
    }
    str = resStr;
}


void merge_maps(std::map<std::string, size_t> &local_map, std::map<std::string, size_t> *global_map, std::mutex &mtx) {
    mtx.lock();
    std::map<std::string, size_t>::iterator itr;
    for (itr = local_map.begin(); itr != local_map.end(); itr++) {
        if (!global_map->count(itr->first))
            global_map->insert(*itr);
        else
            global_map->at(itr->first) += itr->second;
    }
    mtx.unlock();
}

void threaded_idx(mt_que<std::string> &q, std::map<std::string, size_t> &global_map, std::mutex &mtx) {
    std::map<std::string, size_t> num_words;

    std::string promString = q.deque();
    splittedtomap(promString, num_words);
    merge_maps(num_words, &global_map, std::ref(mtx));
}

void threaded_indexing(mt_que<std::string> &q, std::map<std::string, size_t> &global_map, const size_t num_thr){
    std::mutex mtx;
    std::vector<std::thread> threads_v;

    for (size_t i = 0; i < num_thr; i++)
        threads_v.emplace_back(std::thread(threaded_idx, std::ref(q), std::ref(global_map), std::ref(mtx)));

    for (size_t i = 0; i < num_thr; i++)
        threads_v[i].join();

}

void createFileQueue(const std::string &indir, mt_que<std::filesystem::path> &fileNameTXTqueue, mt_que<std::string> &filesTOstring) {
    size_t numfiles = 0;
    fileNameTXTqueue.set_maxsize(10000);
    filesTOstring.set_maxsize(10000);
    std::string pattern = ".+\\.txt$";
    for (const auto & file : std::filesystem::recursive_directory_iterator(indir)){
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
}


int linearAlgorithm( mt_que<std::string> &filesTOstring, std::map<std::string, std::size_t> &mainMap ){

    //    std::map<std::string, std::size_t> promMap;
    //    vector where maps for each
    std::cout << "Linear" << "\n";
    std::vector<std::map<std::string, std::size_t>> vecForPromMaps;
    while (filesTOstring.get_size() > 0) {
        std::string str = filesTOstring.deque();
        std::map<std::string, std::size_t> promMap;
        std::cout<< "Size is = " << filesTOstring.get_size() << "\n";
        // promMap is std::map<std::string, size_t> stores
        // from the each txt file - word and the number of its repetitions
        splittedtomap(str, promMap);

        for ( const auto &[key, value]: promMap ) {
            auto toFind = mainMap.find(key);
            if ( toFind != mainMap.end() ) {
                toFind->second += value;
            } else {
                mainMap.insert ( std::pair<std::string,std::size_t>(key, value) );
            }
        }
    }


}


int main(int argc, char** argv) {


    ////////////////////////////////////////////////////////////
    // collecting arguments
    std::string fl;
    if (argc <2) {
        assert_file_exist("./data/index.cfg");
        fl = "./data/index.cfg";
    } else {
        fl = argv[1];
    }


    Arguments arguments{};


    collect_arguments(fl, arguments);

    std::string indir = arguments.indir;
    int index_threads = arguments.indexing_threads;

    std::string out_by_a = arguments.out_by_a;
    std::string out_by_n = arguments.out_by_n;

    stringRefactor(indir);
    stringRefactor(out_by_a);
    stringRefactor(out_by_n);

    mt_que<std::filesystem::path> fileNameTXTqueue;
    std::map<std::string, std::size_t> mainMap;
    mt_que<std::string> filesTOstring;

//    createFileQueue(indir, fileNameTXTqueue, filesTOstring);/
    createFileQueue(indir, fileNameTXTqueue, filesTOstring);
    if (index_threads==1) {
        linearAlgorithm(filesTOstring, mainMap);
    } else {
        threaded_indexing(filesTOstring, mainMap, index_threads);
    }

    std::vector<std::string> words;

    for(auto & it : mainMap) {
        words.push_back(it.first);
    }

    std::sort(words.begin(), words.end(), compareString);

    writeToFileA(words.size(), words, mainMap, out_by_a);


    std::vector<std::pair<std::string, size_t>> pairs(mainMap.begin(), mainMap.end());
    std::sort(pairs.begin(), pairs.end(), sortbysec);
    size_t lenVec = pairs.size();

    writeToFileB(lenVec, pairs, mainMap, out_by_n);

    ////////////////////////////////////////////////////////////
    //  here file contents as strings are passed to filesTOstring Queue
    //  fileNameTXTqueue - queue for txt filenames from the given directory and all its subdirectories


//    mt_que<std::filesystem::path> fileNameTXTqueue;
//    mt_que<std::string> filesTOstring;
//    size_t numfiles = 0;
//    std::string path = indir;
//    fileNameTXTqueue.set_maxsize(10000);
//    filesTOstring.set_maxsize(10000);
//    std::string pattern = ".+\\.txt$";
//    for (const auto & file : std::filesystem::recursive_directory_iterator(path)){
//        std::string subject = file.path();
//        const std::regex e(pattern);
//        if (regex_match(subject, e)) {
//            fileNameTXTqueue.enque(subject);
//            numfiles++;
//        }
//    }
//
//    std::string permlink;
//    std::string fileName;
//
//    for (size_t i =0; i< numfiles; ++i) {
//
//        fileName = fileNameTXTqueue.deque();
//        read(fileName, permlink);
//        filesTOstring.enque(permlink);
//
//    }


/*
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

    std::vector<std::string> words;
    ///////////////////////// merging all maps into one
    for (size_t i =0; i< vecSize; ++i) {
        for ( const auto &[key, value]: vecForPromMaps[i] ) {
            auto toFind = mainMap.find(key);
            if ( toFind != mainMap.end() ) {
                toFind->second += value;
            } else {
                mainMap.insert ( std::pair<std::string,std::size_t>(key, value) );
                words.push_back(key);
            }
        }
    }

    std::sort(words.begin(), words.end(), compareString);

    std::vector<std::pair<std::string, size_t>> pairs(mainMap.begin(), mainMap.end());
    std::sort(pairs.begin(), pairs.end(), sortbysec);

    size_t lenVec = pairs.size();


    writeToFileA(words.size(), words, mainMap, out_by_a);
    writeToFileB(lenVec, pairs, mainMap, out_by_n);

*/

    return 0;
}
