#include "Disk.hpp"

using namespace std;

Disk::Disk()
{
    disk = new unsigned char *[64];
    for (int i = 0; i < 64; i++)
    {
        disk[i] = new unsigned char[512];
    }
}

void Disk::read_block(int b, unsigned char *rw_buffer)
{
    for (int i = 0; i < 512; i++)
    {
        rw_buffer[i] = disk[b][i];
    }
}

void Disk::write_block(int b, unsigned char *rw_buffer)
{
    for (int i = 0; i < 512; i++)
    {
        disk[b][i] = rw_buffer[i];
    }
}