/**
 * A program to use multiple threads to count words from data obtained
 * via a given URL.
 * 
 * Copyright [2020] <gouldkj@miamioh.edu>
 */

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <iterator>
#include <cctype>
#include <algorithm>
#include <thread>
#include <unordered_map>

// Using namespace to streamline working with Boost socket.
using namespace boost::asio;
using namespace boost::system;

// Shortcut an unordered_map of words. 
using Dictionary = std::unordered_map<std::string, bool>; 
// Forward declaration for method. 
Dictionary loadDictionary(const std::string& filePath); 
// The global dictionary of valid words.
const Dictionary dictionary = loadDictionary("english.txt"); 

/**
 * Loads a supplied file into an unordered map to be used as a dictionary.
 * 
 * \param[in] filePath The path for the file that will be loaded into the 
 * Dictionary.
 * 
 * \return[out] result The dictionary containing the contents of the supplied
 * file.
 */
Dictionary loadDictionary(const std::string& filePath) {
    // Creates an input file stream to read data from.
    std::ifstream in(filePath);
    // String that will retrieve each line of input from the file.
    std::string line;
    // Creates the dictionary that will hold the file contents.
    Dictionary result;
    
    // Take each line from the text file and add it to the dictionary.
    while (in >> line) {
        result[line] = true;
    }
    
    // Returns the result dictionary.
    return result;
}

/**
 * Checks a given word against a dictionary unordered_map to determine if the
 * given word is a valid English word or not.
 * 
 * \param[in] word String that contains a word that will be checked against the
 * dictionary to determine if it is a valid English word.
 * 
 * \return isEng Returns whether or not the supplied word is a valid English 
 * word or not.
 */
bool checkWord(std::string word) {
    // Boolean variable that will store true if the word is English and false 
    // if the word is not English. The variable is false by default.
    bool isEng = false;
    // Iterates through the dictionary.
    for (auto a : dictionary) {
        // If the word stored in the dictionary matches with the given word and
        // the dictionary says that is a valid word, isEng is changed to true.
        if (a.first == word && a.second == true) {
            isEng = true;
        }
    }
    // Returns whether or not the given word is a valid English word.
    return isEng;
}

/**
 * This method processes data from the given file through an input stream. It
 * counts the total number of words and valid English words in the file.
 * 
 * \param[in] is The input stream used to read data from the file.
 * 
 * \param[in] file The file name that will be added to the result string to
 * be returned to the getStats method. 
 * 
 * \param[out] os The output stream used to output data processed from the file 
 * to the console.
 * 
 * \return results Returns a string with statistics gathered from the file.
 */
std::string processFile(std::istream& is, const std::string& file, 
        std::ostream& os = std::cout) {
    // String that will retrieve each line of output.
    std::string line, word;
    // Integer variables that will hold the count of total words and of 
    // English words.
    int wordCount = 0; 
    int englishWords = 0;
    
    // Reads and ignores http headers.
    for (std::string hdr; std::getline(is, hdr) && !hdr.empty() && 
            hdr != "\r";) {}
    
    // Processes input from file line-by-line.
    while (is >> line) {
        // Remove punctuation in a line.
        std::replace_if(line.begin(), line.end(), ::ispunct, ' ');
        // Convert the word to lower case to check against the dictionary. 
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        // Creates a string stream that will be used to break each line into
        // individual words.
        std::stringstream ss(line);
        // Breaks each line of input into individual words.
        while (ss >> word) {
            // Sends to the word to the checkWord method to verify if the word
            // is a valid English word or not.
            bool isEng = checkWord(word);
            // If the word is English, englishWords increases by one.
            if (isEng == true) {
                englishWords++;
            }
            // Word count is increased by one for each word retrieved.
            wordCount++;
        }
    }
    // String of statistics is compiled to be returned to the getStats method.
    std::string results = file + ": words=" + std::to_string(wordCount) + 
            ", English words=" + std::to_string(englishWords);
    // Returns results string.
    return results;
}

/**
 * Helper method to get a string containing the count of words for a
 * given URL.
 *
 * This method internally uses a Boost TCP stream to send an HTTP GET
 * request to the server and print the response.  It then uses
 * doWordCount method to computing the desired statistics.
 *
 * \param[in] file A string containing the file to be processed --
 * e.g., "cpp.txt".  This file is to be added to the base-URL 
 * "http://os1.csi.miamioh.edu/~raodm/cse381/hw6/SlowGet.cgi?file="
 *
 * \return Returns the formatted statistics about the data contained
 * in the given URL.
 */
std::string getStats(const std::string& file) {
    // Sets the port number to 80 by default.
    std::string port = "80";
    // String to contain the host name.
    std::string host = "os1.csi.miamioh.edu";
    // String containing the file path.
    std::string path = "/~raodm/cse381/hw4/SlowGet.cgi?file=" + file;
    
    // Create input/output stream to read data from the file.
    boost::asio::ip::tcp::iostream data(host, port);
    data << "GET " << path << " HTTP/1.1\r\n"
            << "Host: " << host << "\r\n"
            << "Connection: Close\r\n\r\n";
    
    // Retrieves the file statistics from the process file method and stores
    // the statistics in a string.
    std::string results = processFile(data, file);
    // Return a string containing word statistics for the specified
    // file downloaded from a fixed base-URL.
    return results;
}

/** The main method.
 *
 * \param[in] argc The number of command-line arguments
 *
 * \param[in] argv The list of command-line arguments.
 */
int main(int argc, char *argv[]) {
    // Assume each command-line argument is a file to be processed.

    std::vector<std::string> stats(argc);  // Results from each thread.
    std::vector<std::thread> thrList;      // List of threads.

    // First create threads -- 1-thread per file.
    for (int i = 1; (i < argc); i++) {
        // We use a lambda below instead of an explicit
        // threadMain. The lambda takes all arguments by reference
        // (denoted by &) except i, which is taken by value (as i
        // changes in each iteration of the for-loop we want a copy of
        // i)
        thrList.push_back(std::thread([&, i]{ stats[i] = getStats(argv[i]); }));
    }
    // Wait for each thread to finish
    for (auto& thr : thrList) {
        thr.join();  // Wait for thread to finish
    }
    // Finally print the results from each thread.
    for (int i = 1; (i < argc); i++) {
        std::cout << stats[i] << '\n';
    }
    
    // All done!
    return 0;
}
