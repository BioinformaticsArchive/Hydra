#include <iostream>
#include <ctime>
//#include <omp.h>
#include "Hydra.h"
#include "Version.h"

using namespace std;

// define our program name
const string PROGRAM_NAME  = "hydra";

// define our parameter checking macro
#define PARAMETER_CHECK(param, paramLen, actualLen) (strncmp(argv[i], param, min(actualLen, paramLen))== 0) && (actualLen == paramLen)


// function declarations
void ShowHelp(void);
bool IsNumber(const std::string& s);

int main(int argc, char* argv[]) {

    // initialization
    ostringstream sb;
    char* end_ptr = NULL;

    // our configuration variables
    bool showHelp          = false;

    int memory = (int) ((8) * pow(2,30));  // 1Gb = 2^30, use 8Gb by default.
    int minSupport        = 2;
    int maxLinkedDistance = 1000000; // 1Mb by default
    int numThreads = 1;
    
    // TAB-separated configuration file listing the samples, files and their statistics.
    // 1. sample ID (string)
    // 2. path/filename (string)
    // 3. median/mean (float)
    // 4. mad/std (float)
    // 5. num mads/stds. (float)
    string configFile;
    string routedFiles;
    string routedFileList;
    string posSortedFiles;
    vector<DNALIB> sampleLibs;

    // sample ids
    vector<string> samples;    
    // input files
    vector<string> discordantFiles;
    
    // library parameters for each file.
    vector<int> expectedSizes;
    vector<int> variances;
    vector<int> degreesOfVariance;

    // output files
    string outFile;

    // checks for existence of parameters
    bool haveConfigFile         = false;    
    bool haveDiscordants        = false;
    bool haveOutFile            = false;
    bool haveRoutedFiles        = false;
    bool haveRoutedFileList     = false;
    bool havePosSortedFiles     = false;
    bool haveLD                 = false;  // deprecated 
    bool haveSD                 = false;  // deprecated
    bool haveSizes              = false;
    bool haveVariances          = false;
    bool haveDegressOfVariances = false;
    bool haveMinSupport         = false;
    bool haveMaxLinkedDistance  = false;
    bool lumpInversions         = false;
    bool ignoreSize             = false;
    bool useGivenMappings            = false;

    string mappingUsage         = "best";
    unsigned int editBeyondBest = 0;
    

    // check to see if we should print out some help
    if(argc <= 1) showHelp = true;

    for(int i = 1; i < argc; i++) {
        int parameterLength = (int)strlen(argv[i]);

        if(PARAMETER_CHECK("-h", 2, parameterLength) || 
        PARAMETER_CHECK("--help", 5, parameterLength)) {
            showHelp = true;
        }
    }

    if(showHelp) ShowHelp();

    // do some parsing (all of these parameters require 2 strings)
    cerr << endl << "Parameters:" << endl;
    for(int i = 1; i < argc; i++) {

        int parameterLength = (int)strlen(argv[i]);

        if(PARAMETER_CHECK("-config", 7, parameterLength)) {
            if ((i+1) < argc) {
                haveConfigFile = true;
                configFile = argv[i + 1];
                cerr << "  Configuration file (-config): " << configFile << endl;
                i++;
            }
        }
        else if(PARAMETER_CHECK("-routedList", 11, parameterLength)) {
            if ((i+1) < argc) {
                haveRoutedFileList = true;
                routedFileList = argv[i + 1];
                cerr << "  Routed file list (-routedList): " << routedFileList << endl;
                i++;
            }
        }
        else if(PARAMETER_CHECK("-useMappings", 12, parameterLength)) {
            useGivenMappings = true;
        }
        else {
            cerr << "*****ERROR: Unrecognized parameter: " << argv[i] << " *****" << endl << endl;
            showHelp = true;
        }       
    }

    if (!haveConfigFile) {
        cerr << "*****ERROR: You must specify an input configuration file.*****" << endl << endl;
        showHelp = true;
    }
    
    if (!haveRoutedFileList) {
        cerr << "*****ERROR: You must specify an output file to contain a list of the routed files.*****" << endl << endl;
        showHelp = true;
    }

    if (!showHelp) {

        cerr << endl << "Processing: " << endl;

        if (haveConfigFile == true) {
            // open the mapping files for reading
            ifstream config(configFile.c_str(), ios::in);

            if ( !config ) {
                cerr << "Error: The configuration file (" << configFile << ") could not be opened. Exiting!" << endl;
                exit (1);
            }
            else {
                string sample, file;
                float  mean, std, numstd;
                while (config >> sample >> file >> mean >> std >> numstd) {
                    DNALIB lib(sample, file, mean, std, numstd);
                    sampleLibs.push_back(lib);
                }
            }
        }
        
        HydraPE *events = new HydraPE(sampleLibs, routedFileList, minSupport,
                                      maxLinkedDistance, ignoreSize, lumpInversions,
                                      mappingUsage, editBeyondBest, memory, useGivenMappings);
        
        cerr << "  Routing discordant mappings to master chrom/chrom/strand/strand files." << endl;  
        events->RouteDiscordantMappings();
        events->WriteRoutedFiles();
        return 0;
    }
    else {
        ShowHelp();
    }
}


void ShowHelp(void) {
    cerr << endl << "Program: " << PROGRAM_NAME << " (v" << VERSION << ")" << endl;
    
    cerr << "Author:  Aaron Quinlan (aaronquinlan@gmail.com)" << endl;

    cerr << "Summary: Calls SV breakpoints from discordant paired-end mappings." << endl << endl;

    cerr << "Usage:   " << PROGRAM_NAME << " -config" << endl << endl;

    cerr << "Options:" << endl;
    cerr << "  -config\tConfiguration file. (req'd)" << endl;
    cerr << "       \t\tCol 1. Sample Id (string)" << endl;
    cerr << "       \t\tCol 2. Mapping file (path/file)" << endl;
    cerr << "       \t\tCol 3. Expected insert size (integer)" << endl;
    cerr << "       \t\tCol 4. Variance (integer)" << endl;
    cerr << "       \t\tCol 5. Num. variances (integer)" << endl << endl;

    cerr << "  -routedList\tOutput file containing the list of routed files (req'd)." << endl << endl;
    
    cerr << "  -useMappings\tDon't count mappings, use the num_mappings fields." << endl << endl;

    // end the program here
    exit(1);
}


bool IsNumber(const std::string& s) {
   for (int i = 0; i < s.length(); i++) {
       if (!std::isdigit(s[i]))
           return false;
   }
   return true;
}

