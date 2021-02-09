#include <string>
#include "Disk.hpp"

using namespace std;

typedef struct
{
    unsigned char buffer[512];
    int current_pos;
    int file_size;
    int descriptor_index;
} oft_entry;

class FS
{

private:
    oft_entry oft[4];
    unsigned char M[512];
    Disk *emulatedDisk;
    unsigned char **cache;

public:
    FS();
    void init();
    void create(unsigned char *name);
    void destroy(unsigned char *name);
    int open(unsigned char *name);
    int close(int i, bool admin);
    int read(int i, int m, int n);
    int write(int i, int m, int n);
    int seek(int i, int p);
    string directory();
    string read_memory(int m, int n);
    int write_memory(int m, unsigned char *s);
    void quit();
};