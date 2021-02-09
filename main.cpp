#include "FS.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include <cstring>
#include <string>

#define _CRT_SECURE_NO_WARNINGS
using namespace std;

int main(int argc, char *argv[])
{
    // string inputFileName = "";
    // cout << "Enter file name (with .txt extension): ";
    // getline(cin, inputFileName);

    ifstream inputFile;
    inputFile.open(argv[1]);
    string s;

    ofstream outputFile;
    outputFile.open("output.txt");

    FS *fileStructure = new FS();

    unsigned char fileName[4];
    if (inputFile.is_open())
    {
        while (getline(inputFile, s))
        {
            istringstream s_stream(s);
            vector<string> shellCommands{istream_iterator<string>{s_stream}, istream_iterator<string>{}};

            if (shellCommands.size() == 0)
            {
                outputFile << endl;
            }
            else if (shellCommands[0].compare("cr") == 0)
            {
                unsigned char fileName[4] = {'\0', '\0', '\0', '\0'};
                for (int i = 0; i < 3; i++)
                {
                    fileName[i] = shellCommands[1][i];
                }
                try
                {
                    fileStructure->create(fileName);
                    outputFile << shellCommands[1] << " created" << endl;
                }
                catch (const char *e)
                {
                    outputFile << "error" << endl;
                }
            }
            else if (shellCommands[0].compare("de") == 0)
            {
                unsigned char fileName[4] = {'\0', '\0', '\0', '\0'};
                for (int i = 0; i < 3; i++)
                {
                    fileName[i] = shellCommands[1][i];
                }
                try
                {
                    fileStructure->destroy(fileName);
                    outputFile << shellCommands[1] << " destroyed" << endl;
                }
                catch (const char *e)
                {
                    outputFile << "error" << endl;
                }
            }
            else if (shellCommands[0].compare("op") == 0)
            {
                unsigned char fileName[4] = {'\0', '\0', '\0', '\0'};
                for (int i = 0; i < shellCommands[1].length(); i++)
                {
                    fileName[i] = shellCommands[1][i];
                }

                try
                {
                    int oftIndex = fileStructure->open(fileName);
                    outputFile << shellCommands[1] << " opened " << oftIndex << endl;
                }
                catch (const char *e)
                {
                    outputFile << "error" << endl;
                }
            }
            else if (shellCommands[0].compare("cl") == 0)
            {
                try
                {
                    fileStructure->close(stoi(shellCommands[1]), false);
                    outputFile << shellCommands[1] << " closed" << endl;
                }
                catch (const char *e)
                {
                    outputFile << "error" << endl;
                }
            }
            else if (shellCommands[0].compare("rd") == 0)
            {
                try
                {
                    int bytesRead = stoi(shellCommands[3]) - fileStructure->read(stoi(shellCommands[1]), stoi(shellCommands[2]), stoi(shellCommands[3]));
                    outputFile << bytesRead << " bytes read from " << shellCommands[1] << endl;
                }
                catch (const char *e)
                {
                    outputFile << "error" << endl;
                }
            }
            else if (shellCommands[0].compare("wr") == 0)
            {
                try
                {
                    int bytesWritten = stoi(shellCommands[3]) - fileStructure->write(stoi(shellCommands[1]), stoi(shellCommands[2]), stoi(shellCommands[3]));
                    outputFile << bytesWritten << " bytes written to " << shellCommands[1] << endl;
                }
                catch (const char *e)
                {
                    outputFile << "error" << endl;
                }
            }
            else if (shellCommands[0].compare("sk") == 0)
            {
                try
                {
                    fileStructure->seek(stoi(shellCommands[1]), stoi(shellCommands[2]));
                    outputFile << "position is " << shellCommands[2] << endl;
                }
                catch (const char *e)
                {
                    outputFile << "error" << endl;
                }
            }
            else if (shellCommands[0].compare("dr") == 0)
            {
                outputFile << fileStructure->directory() << endl;
            }
            else if (shellCommands[0].compare("in") == 0)
            {
                outputFile << "system initialized" << endl;
                fileStructure->init();
            }
            else if (shellCommands[0].compare("rm") == 0)
            {
                // unsigned char *ch = new unsigned char[stoi(shellCommands[2]) + 1];
                // unsigned char *mem = new unsigned char[stoi(shellCommands[2]) + 1];
                // memcpy(ch, fileStructure->read_memory(stoi(shellCommands[1]), stoi(shellCommands[2])), stoi(shellCommands[2]) + 1);
                // char *convertedCh = (char *)ch;
                // outputFile << convertedCh << endl;
                outputFile << fileStructure->read_memory(stoi(shellCommands[1]), stoi(shellCommands[2])) << endl;
            }
            else if (shellCommands[0].compare("wm") == 0)
            {
                unsigned char buffer[513];
                for (int i = 0; i < shellCommands[2].length(); i++)
                {
                    buffer[i] = shellCommands[2][i];
                }
                buffer[shellCommands[2].length()] = '\0';
                fileStructure->write_memory(stoi(shellCommands[1]), buffer);
                outputFile << shellCommands[2].length() << " bytes written to M" << endl;
            }
        }
    }

    fileStructure->quit();

    return 0;
}
