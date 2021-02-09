#include "FS.hpp"
#include <iostream>
#include <cstring>

using namespace std;

FS::FS()
{
    emulatedDisk = new Disk();

    //Initialize the cache, which will later copy blocks 0-6 and keep them in main memory
    cache = new unsigned char *[6];
    for (int i = 0; i < 7; i++)
    {
        cache[i] = new unsigned char[512];
        for (int j = 0; j < 512; j++)
        {
            cache[i][j] = '\0';
        }
    }

    init(); //Init won't be called by the end user at startup so the constructor can call on it to do the rest of the setup
}

void FS::init()
{

    //Initialize the memory buffer
    for (int i = 0; i < 512; i++)
    {
        M[i] = '\0';
    }

    //Using the blank memory buffer, copy that to every block of the disk to initialize it
    for (int i = 0; i < 64; i++)
    {
        emulatedDisk->write_block(i, M);
    }

    //Initialize the bitmap

    //It starts empty except for blocks 0-7, since 0 holds the bitmap and 1-6 are the file descriptors, and 7 is allocated for the directory.  Thus, the first byte should look like 11111111
    unsigned char blocks_1to8;
    blocks_1to8 |= 1 << 0;
    blocks_1to8 |= 1 << 1;
    blocks_1to8 |= 1 << 2;
    blocks_1to8 |= 1 << 3;
    blocks_1to8 |= 1 << 4;
    blocks_1to8 |= 1 << 5;
    blocks_1to8 |= 1 << 6;
    blocks_1to8 |= 1 << 7;
    M[0] = blocks_1to8;

    // //The remaining 7 bytes are all 0000000
    // for (int i = 1; i < 8; i++)
    // {
    //     unsigned char otherBlocks;
    //     otherBlocks |= 0 << 0;
    //     otherBlocks |= 0 << 1;
    //     otherBlocks |= 0 << 2;
    //     otherBlocks |= 0 << 3;
    //     otherBlocks |= 0 << 4;
    //     otherBlocks |= 0 << 5;
    //     otherBlocks |= 0 << 6;
    //     otherBlocks |= 0 << 7;
    //     M[i] = otherBlocks;
    // }

    //Then write it to the 0th block on the disk
    emulatedDisk->write_block(0, M);

    //Initialize the file descriptors
    const int negOneInt = -1;
    // unsigned char negOneArray[4] = {};
    // copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &negOneArray[0]);

    const int zeroInt = 0;
    // unsigned char zeroArray[4] = {};
    // copy(reinterpret_cast<const unsigned char *>(&zeroInt), reinterpret_cast<const unsigned char *>(&zeroInt) + 4, &zeroArray[0]);

    const int sevenInt = 7;
    // unsigned char sevenArray[4] = {};
    // copy(reinterpret_cast<const unsigned char *>(&sevenInt), reinterpret_cast<const unsigned char *>(&sevenInt) + 4, &sevenArray[0]);

    for (int i = 1; i < 7; i++)
    {
        for (int j = 0; j < 512; j = j + 16)
        {
            if (i == 1 && j == 0) //i == 1 and j == 0 indicates the directory which is initialized as {0, 7, -1, -1}
            {
                // write_memory(j, zeroArray);
                // write_memory(j + 4, sevenArray);
                // write_memory(j + 8, negOneArray);
                // write_memory(j + 12, negOneArray);
                copy(reinterpret_cast<const unsigned char *>(&zeroInt), reinterpret_cast<const unsigned char *>(&zeroInt) + 4, &M[j]);
                copy(reinterpret_cast<const unsigned char *>(&sevenInt), reinterpret_cast<const unsigned char *>(&sevenInt) + 4, &M[j + 4]);
                copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &M[j + 8]);
                copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &M[j + 12]);
            }
            else //The rest of the file descriptors are initialized as {-1, -1, -1, -1}
            {
                // write_memory(j, negOneArray);
                // write_memory(j + 4, negOneArray);
                // write_memory(j + 8, negOneArray);
                // write_memory(j + 12, negOneArray);
                copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &M[j]);
                copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &M[j + 4]);
                copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &M[j + 8]);
                copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &M[j + 12]);
            }
        }
        //Copy the buffer to the disk
        emulatedDisk->write_block(i, M);
    }
    //Initialize the OFT
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 512; j++)
        {
            oft[i].buffer[j] = '\0'; //Every byte of the buffer is initialized as '\0'
        }

        //As described in the specs, the rest of the information is initialized as -1
        oft[i].current_pos = -1;
        oft[i].descriptor_index = -1;
        oft[i].file_size = -1;
    }

    //The 0th index of the OFT is allocated to the directory and is initialized as such
    emulatedDisk->read_block(7, oft[0].buffer);
    oft[0].current_pos = 0;
    oft[0].descriptor_index = 0;
    oft[0].file_size = 0;

    //Copy blocks 0-6 to the cache since it will be accessed frequently
    for (int i = 0; i < 7; i++)
    {
        emulatedDisk->read_block(i, cache[i]);
    }

    //Clear M once again
    for (int i = 0; i < 512; i++)
    {
        M[i] = '\0';
    }
}

void FS::create(unsigned char *name)
{
    seek(0, 0);                                   //Moving to the beginning of the directory buffer
    while (oft[0].current_pos < oft[0].file_size) //Search through the directory to see if the file exists
    {
        read(0, 100, 8); //As suggested on piazza, starting at m = 100 should not overwrite the bytes used for files
        //Retrieve the name of the directory entry
        unsigned char entryName[4];
        for (int i = 0; i < 4; i++)
        {
            entryName[i] = M[100 + i];
        }
        bool nameMatch = true;

        for (int i = 0; i < 4; i++)
        {
            if (entryName[i] != name[i])
            {
                nameMatch = false;
            }
        }

        if (nameMatch)
        {
            throw "Error: Duplicate file name";
        }
    }

    int freeDescriptor = -1;

    //loop through all of the descriptor blocks and indexes to find a free file descriptor
    for (int i = 1; i < 7; i++)
    {
        for (int j = 0; j < 512; j = j + 16)
        {
            int descriptorSize = 0;
            copy(&cache[i][j], &cache[i][j + 4], reinterpret_cast<unsigned char *>(&descriptorSize)); //Access the file size and convert to an int

            //If a free descriptor is found, save the index, change the descriptor file size to 0, and break
            if (descriptorSize == -1)
            {
                freeDescriptor = ((i - 1) * 32) + (j / 16); //Save the index

                //Setting the descriptor file size to 0
                const int zeroInt = 0;
                copy(reinterpret_cast<const unsigned char *>(&zeroInt), reinterpret_cast<const unsigned char *>(&zeroInt) + 4, &cache[i][j]);

                break;
            }
        }

        //Once again we have to break since it's a nested for loop
        if (freeDescriptor != -1)
        {
            break;
        }
    }

    if (freeDescriptor == -1)
    {
        throw "Error: Too many files";
    }

    //Create the contents for the new directory entry and put them into the memory buffer to write to the directory block later
    unsigned char newDirectoryEntry[4] = {};
    const int newDescriptorIndex = freeDescriptor;
    for (int i = 0; i < 4; i++) //Copy the name into the first four bytes of the directory entry
    {
        newDirectoryEntry[i] = name[i];
    }

    for (int i = 100; i < 104; i++) 
    {
        M[i] = '\0';
    }

    write_memory(100, newDirectoryEntry);

    //Wipe the array before writing again to avoid conflicts
    for (int i = 0; i < 4; i++)
    {
        newDirectoryEntry[i] = '\0';
    }

    copy(reinterpret_cast<const unsigned char *>(&newDescriptorIndex), reinterpret_cast<const unsigned char *>(&newDescriptorIndex) + 4, &newDirectoryEntry[0]); //Convert the descriptor index into a char array and put it into the last four bytes of the entry
    write_memory(104, newDirectoryEntry);

    seek(0, 0); //Moving to the beginning of the directory buffer
    while (oft[0].current_pos < oft[0].file_size)
    {
        read(0, 108, 8);
        int entryNameAsInt = 0;
        copy(&M[108], &M[108] + 4, reinterpret_cast<unsigned char *>(&entryNameAsInt)); //Convert the name field to an int
        if (entryNameAsInt == 0)                                                        //If the name field is 0, it is an open spot in the directory
        {
            break; //Break to stop at the current position of the directory buffer
        }
    }

    if (oft[0].current_pos == 1536)
    {
        throw "Error: No free directory entry found";
    }
    if (oft[0].current_pos < oft[0].file_size)
    {
        seek(0, oft[0].current_pos - 8);
    }
    write(0, 100, 8); //Write the new directory entry into the directory at the current position - reminder: M[100] - M[107] stores the entry
}

void FS::destroy(unsigned char *name)
{
    seek(0, 0); //Move current position to 0 in the directory
    int descriptorIndex = -1;
    while (oft[0].current_pos < oft[0].file_size) //Search through the directory to see if the file exists
    {
        read(0, 100, 8); //As suggested on piazza, starting at m = 100 should not overwrite the bytes used for files

        //Retrieve the name of the directory entry
        unsigned char entryName[4];
        for (int i = 0; i < 4; i++)
        {
            entryName[i] = M[100 + i];
        }
        bool nameMatch = true;

        //Check if the name in the directory matches the name given
        for (int i = 0; i < 4; i++)
        {
            if (entryName[i] != name[i])
            {
                nameMatch = false;
            }
        }

        if (nameMatch)
        {
            copy(&M[104], &M[108], reinterpret_cast<unsigned char *>(&descriptorIndex));
            break;
        }
    }

    if (descriptorIndex == -1)
    {
        throw "Error: File does not exist";
    }

    for (int i = 1; i < 4; i++) //In the event that the file hasn't been closed before destroy is called, destroy will automatically close it in the oft
    {
        if (oft[i].descriptor_index == descriptorIndex)
        {
            close(i, false);
        }
    }

    int fileDescriptor[4] = {-1, -1, -1, -1};        //Used to store the file descriptor values after converting back from char arrays
    int descriptorBlock = (descriptorIndex / 32) + 1; //Reminder: descriptorBlock should be a value from 1-6
    int descriptorBlockIndex = (descriptorIndex % 32) * 16;

    unsigned char *cacheDescriptorBlock = cache[descriptorBlock]; //Grab the correct block of file descriptors and put into another variable for ease of use

    //Convert the char arrays back to ints and store them into the file descriptor array
    copy(&cacheDescriptorBlock[descriptorBlockIndex], &cacheDescriptorBlock[descriptorBlockIndex + 4], reinterpret_cast<unsigned char *>(&fileDescriptor[0]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 4], &cacheDescriptorBlock[descriptorBlockIndex + 8], reinterpret_cast<unsigned char *>(&fileDescriptor[1]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 8], &cacheDescriptorBlock[descriptorBlockIndex + 12], reinterpret_cast<unsigned char *>(&fileDescriptor[2]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 12], &cacheDescriptorBlock[descriptorBlockIndex + 16], reinterpret_cast<unsigned char *>(&fileDescriptor[3]));

    const int zeroInt = 0;
    const int negOneInt = -1;

    copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &cacheDescriptorBlock[descriptorBlockIndex]); //Set the size field to -1

    for (int i = 1; i < 4; i++)
    {
        if (fileDescriptor[i] != -1)
        {
            //Set the bit to 0 in the bitmap
            int bitmapByte = fileDescriptor[i] / 8;
            int bitmapIndex = fileDescriptor[i] % 8;
            cache[0][bitmapByte] &= ~(1 << bitmapIndex);

            //Set the block to -1 in the file descriptor
            copy(reinterpret_cast<const unsigned char *>(&negOneInt), reinterpret_cast<const unsigned char *>(&negOneInt) + 4, &cacheDescriptorBlock[descriptorBlockIndex + (4 * i)]);
        }
    }

    //Go back to the beginning of the directory entry
    seek(0, oft[0].current_pos - 8);
    copy(reinterpret_cast<const unsigned char *>(&zeroInt), reinterpret_cast<const unsigned char *>(&zeroInt) + 4, &M[100]);
    copy(reinterpret_cast<const unsigned char *>(&zeroInt), reinterpret_cast<const unsigned char *>(&zeroInt) + 4, &M[104]);
    write(0, 100, 8);              //Write the blank entry to the directory's buffer in the OFT
}

int FS::open(unsigned char *name)
{
    seek(0, 0); //Moving to the beginning of the directory buffer
    int descriptorIndex = -1;
    while (oft[0].current_pos < oft[0].file_size) //Search through the directory to see if the file exists
    {
        read(0, 100, 8); //As suggested on piazza, starting at m = 100 should not overwrite the bytes used for files

        //Retrieve the name of the directory entry
        unsigned char entryName[4];
        for (int i = 0; i < 4; i++)
        {
            entryName[i] = M[100 + i];
        }
        bool nameMatch = true;

        //Check if the name in the directory matches the name givne
        for (int i = 0; i < 4; i++)
        {
            if (entryName[i] != name[i])
            {
                nameMatch = false;
            }
        }

        if (nameMatch)
        {
            copy(&M[104], &M[108], reinterpret_cast<unsigned char *>(&descriptorIndex));
            break;
        }
    }

    if (descriptorIndex == -1)
    {
        throw "Error: File does not exist";
    }

    for (int i = 1; i < 4; i++) 
    {
        if (oft[i].descriptor_index == descriptorIndex)
        {
            throw "Error: File already open";
        }
    }

    //Find the first available slot in the OFT
    int openOFT = -1;
    for (int i = 1; i < 4; i++)
    {
        if (oft[i].file_size == -1)
        {
            openOFT = i;
            break;
        }
    }

    if (openOFT == -1)
    {
        throw "Error: Too many files open";
    }

    int fileDescriptor[4] = {-1, -1, -1, -1};        //Used to store the file descriptor values after converting back from char arrays
    int descriptorBlock = (descriptorIndex / 32) + 1; //Reminder: descriptorBlock should be a value from 1-6
    int descriptorBlockIndex = (descriptorIndex % 32) * 16;

    unsigned char *cacheDescriptorBlock = cache[descriptorBlock]; //Grab the correct block of file descriptors and put into another variable for ease of use

    //Convert the char arrays back to ints and store them into the file descriptor array
    copy(&cacheDescriptorBlock[descriptorBlockIndex], &cacheDescriptorBlock[descriptorBlockIndex + 4], reinterpret_cast<unsigned char *>(&fileDescriptor[0]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 4], &cacheDescriptorBlock[descriptorBlockIndex + 8], reinterpret_cast<unsigned char *>(&fileDescriptor[1]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 8], &cacheDescriptorBlock[descriptorBlockIndex + 12], reinterpret_cast<unsigned char *>(&fileDescriptor[2]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 12], &cacheDescriptorBlock[descriptorBlockIndex + 16], reinterpret_cast<unsigned char *>(&fileDescriptor[3]));

    //Set the current position to 0 and copy the file size and descriptor index over
    oft[openOFT].current_pos = 0;
    oft[openOFT].file_size = fileDescriptor[0];
    oft[openOFT].descriptor_index = descriptorIndex;

    //If the file size is 0 then the initial block needs to be allocated
    if (oft[openOFT].file_size == 0 && fileDescriptor[1] == -1)
    {
        int freeBlock = -1;
        for (int i = 0; i < 8; i++) //Loop through the 8 bytes
        {
            for (int j = 0; j < 8; j++) //Loop through the 8 bits in each byte
            {
                if (((cache[0][i] >> j) & 1) == 0) //Find the first bit that's marked 0 (a free block)
                {
                    freeBlock = (i * 8) + j; //Compute which block is the free one
                    cache[0][i] |= 1 << j;   //Set the bit to 1 to indicate that it is now allocated
                    break;
                }
            }
            if (freeBlock != -1)
            {
                break;
            }
        }
        if (freeBlock == -1)
        {
            oft[openOFT].current_pos = -1;
            oft[openOFT].file_size = -1;
            oft[openOFT].descriptor_index = -1;
            throw "Error: No free block to allocate for file";
        }
        const int fileDescriptor1 = freeBlock;
        copy(reinterpret_cast<const unsigned char *>(&fileDescriptor1), reinterpret_cast<const unsigned char *>(&fileDescriptor1) + 4, &cacheDescriptorBlock[descriptorBlockIndex + 4]); //Record the new allocated block in the file descriptor

        //Bring in the initial block into the oft at index i
        emulatedDisk->read_block(freeBlock, oft[openOFT].buffer);
    }
    else //Otherwise just bring in the first allocated block into the buffer
    {
        emulatedDisk->read_block(fileDescriptor[1], oft[openOFT].buffer);
    }

    return openOFT;
}

int FS::close(int i, bool admin)
{
    if (i < 0 || i > 3) //throw an exception because the index is out of range
    {
        throw "Error: Invalid OFT index";
    }
    if (i == 0 && !admin) //throw an exception because this OFT[0] is reserved for the directory and cannot be closed
    {
        throw "Error: Directory cannot be closed";
    }
    if (oft[i].file_size == -1) //throw an exception because there is no file open here
    {
        throw "Error: No file opened at this OFT index";
    }

    int fileDescriptor[4] = {-1, -1, -1, -1};                 //Used to store the file descriptor values after converting back from char arrays
    int descriptorBlock = (oft[i].descriptor_index / 32) + 1; //Reminder: descriptorBlock should be a value from 1-6
    int descriptorBlockIndex = (oft[i].descriptor_index % 32) * 16;

    unsigned char *cacheDescriptorBlock = cache[descriptorBlock]; //Grab the correct block of file descriptors and put into another variable for ease of use

    //Convert the char arrays back to ints and store them into the file descriptor array
    copy(&cacheDescriptorBlock[descriptorBlockIndex], &cacheDescriptorBlock[descriptorBlockIndex + 4], reinterpret_cast<unsigned char *>(&fileDescriptor[0]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 4], &cacheDescriptorBlock[descriptorBlockIndex + 8], reinterpret_cast<unsigned char *>(&fileDescriptor[1]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 8], &cacheDescriptorBlock[descriptorBlockIndex + 12], reinterpret_cast<unsigned char *>(&fileDescriptor[2]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 12], &cacheDescriptorBlock[descriptorBlockIndex + 16], reinterpret_cast<unsigned char *>(&fileDescriptor[3]));

    //Determine the block that is held in the r/w buffer and copy it to the correct disk on the block
    if (oft[i].current_pos > 1024)
    {
        emulatedDisk->write_block(fileDescriptor[3], oft[i].buffer);
    }
    else if (oft[i].current_pos > 512)
    {
        emulatedDisk->write_block(fileDescriptor[2], oft[i].buffer);
    }
    else
    {
        emulatedDisk->write_block(fileDescriptor[1], oft[i].buffer);
    }

    //Wipe the buffer at index i of the OFT
    for (int j = 0; j < 512; j++)
    {
        oft[i].buffer[j] = '\0';
    }

    //Update the file size in the descriptor and set the file size at OFT[i] to -1
    const int newFileSize = oft[i].file_size; //Then update it in its file descriptor
    copy(reinterpret_cast<const unsigned char *>(&newFileSize), reinterpret_cast<const unsigned char *>(&newFileSize) + 4, &cacheDescriptorBlock[descriptorBlockIndex]);
    oft[i].file_size = -1;

    oft[i].current_pos = -1;      //Set current position to -1 to mark the OFT entry as free
    oft[i].descriptor_index = -1; //Set descriptor index to -1 to wipe the OFT entry

    return i;
}

int FS::read(int i, int m, int n)
{
    if (i < 0 || i > 3)
    {
        throw "Error: Invalid OFT index";
    }
    if (oft[i].file_size == -1)
    {
        throw "Error: No file opened at this OFT index";
    }
    if (n < 0)
    {
        throw "Error: Cannot read a negative number of bytes";
    }
    if (m < 0 || m >= 512) {
        throw "Error: Invalid memory index";
    }

    int bufferPosition = oft[i].current_pos % 512;

    int fileDescriptor[4] = {-1, -1, -1, -1};                 //Used to store the file descriptor values after converting back from char arrays
    int descriptorBlock = (oft[i].descriptor_index / 32) + 1; //Reminder: descriptorBlock should be a value from 1-6
    int descriptorBlockIndex = (oft[i].descriptor_index % 32) * 16;

    unsigned char *cacheDescriptorBlock = cache[descriptorBlock]; //Grab the correct block of file descriptors and put into another variable for ease of use

    //Convert the char arrays back to ints and store them into the file descriptor array
    copy(&cacheDescriptorBlock[descriptorBlockIndex], &cacheDescriptorBlock[descriptorBlockIndex + 4], reinterpret_cast<unsigned char *>(&fileDescriptor[0]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 4], &cacheDescriptorBlock[descriptorBlockIndex + 8], reinterpret_cast<unsigned char *>(&fileDescriptor[1]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 8], &cacheDescriptorBlock[descriptorBlockIndex + 12], reinterpret_cast<unsigned char *>(&fileDescriptor[2]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 12], &cacheDescriptorBlock[descriptorBlockIndex + 16], reinterpret_cast<unsigned char *>(&fileDescriptor[3]));

    while (n > 0 && oft[i].current_pos < oft[i].file_size && m < 512)
    {
        if (oft[i].current_pos == 1024) //If the current position reaches 1024, the oft buffer saves the current block (2nd block) and brings in the new block (3rd block)
        {
            //Save current block (2nd block) to the disk
            emulatedDisk->write_block(fileDescriptor[2], oft[i].buffer);

            //Bring the new block (3rd block) into the oft at index i
            emulatedDisk->read_block(fileDescriptor[3], oft[i].buffer);
        }
        else if (oft[i].current_pos == 512) //If the current position reaches 512, the oft buffer saves the current block (1st block) and brings in the new block (2nd block)
        {
            //Save current block (1st block) to the disk
            emulatedDisk->write_block(fileDescriptor[1], oft[i].buffer);

            //Bring the new block (2nd block) into the oft at index i
            emulatedDisk->read_block(fileDescriptor[2], oft[i].buffer);
        }

        M[m] = oft[i].buffer[bufferPosition];      //Copy the corresponding byte from the buffer at oft[i] to M[m]
        m++;                                       //Increment m to get to the next index of the memory buffer
        n--;                                       //Decrement n, the number of bytes left to read
        oft[i].current_pos++;                      //Increment the current position
        bufferPosition = oft[i].current_pos % 512; //Recalculate the buffer position
    }
    return n;
}

int FS::write(int i, int m, int n)
{
    if (i < 0 || i > 3)
    {
        throw "Error: Invalid OFT index";
    }
    if (oft[i].file_size == -1)
    {
        throw "Error: No file opened at this OFT index";
    }
    if (n < 0)
    {
        throw "Error: Cannot write a negative number of bytes";
    }
    if (m < 0 || m >= 512) {
        throw "Error: Invalid memory index";
    }
    int fileDescriptor[4] = {-1, -1, -1, -1};                 //Used to store the file descriptor values after converting back from char arrays
    int descriptorBlock = (oft[i].descriptor_index / 32) + 1; //Reminder: descriptorBlock should be a value from 1-6
    int descriptorBlockIndex = (oft[i].descriptor_index % 32) * 16;

    unsigned char *cacheDescriptorBlock = cache[descriptorBlock]; //Grab the correct block of file descriptors and put into another variable for ease of use

    //Convert the char arrays back to ints and store them into the file descriptor array
    copy(&cacheDescriptorBlock[descriptorBlockIndex], &cacheDescriptorBlock[descriptorBlockIndex + 4], reinterpret_cast<unsigned char *>(&fileDescriptor[0]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 4], &cacheDescriptorBlock[descriptorBlockIndex + 8], reinterpret_cast<unsigned char *>(&fileDescriptor[1]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 8], &cacheDescriptorBlock[descriptorBlockIndex + 12], reinterpret_cast<unsigned char *>(&fileDescriptor[2]));
    copy(&cacheDescriptorBlock[descriptorBlockIndex + 12], &cacheDescriptorBlock[descriptorBlockIndex + 16], reinterpret_cast<unsigned char *>(&fileDescriptor[3]));

    int bufferPosition = oft[i].current_pos % 512;
    int freeBlock = -1;

    while (n > 0 && oft[i].current_pos < 1536)
    {
        if (oft[i].current_pos == 1024) //If the current position reaches 1024, the oft buffer saves the current block (2nd block) and brings in the new block (3rd block)
        {
            if (fileDescriptor[3] == -1) //Need to allocate third block
            {
                for (int i = 0; i < 8; i++) //Loop through the 8 bytes
                {
                    for (int j = 0; j < 8; j++) //Loop through the 8 bits in each byte
                    {
                        if (((cache[0][i] >> j) & 1) == 0) //Find the first bit that's marked 0 (a free block)
                        {
                            freeBlock = (i * 8) + j; //Compute which block is the free one
                            cache[0][i] |= 1 << j;   //Set the bit to 1 to indicate that it is now allocated
                            break;
                        }
                    }
                    if (freeBlock != -1)
                    {
                        break;
                    }
                }
                if (freeBlock == -1)
                {
                    throw "Error: No free block to allocate for file";
                }
                const int fileDescriptor3 = freeBlock;
                copy(reinterpret_cast<const unsigned char *>(&fileDescriptor3), reinterpret_cast<const unsigned char *>(&fileDescriptor3) + 4, &cacheDescriptorBlock[descriptorBlockIndex + 12]); //Record the new allocated block in the file descriptor
            }
            //Save current block (2nd block) to the disk
            emulatedDisk->write_block(fileDescriptor[2], oft[i].buffer);

            //Bring the new block (3rd block) into the oft at index i
            emulatedDisk->read_block(freeBlock, oft[i].buffer);
        }
        else if (oft[i].current_pos == 512) //If the current position reaches 512, the oft buffer saves the current block (1st block) and brings in the new block (2nd block)
        {
            if (fileDescriptor[2] == -1) //Need to allocate second block
            {
                for (int i = 0; i < 8; i++) //Loop through the 8 bytes
                {
                    for (int j = 0; j < 8; j++) //Loop through the 8 bits in each byte
                    {
                        if (((cache[0][i] >> j) & 1) == 0) //Find the first bit that's marked 0 (a free block)
                        {
                            freeBlock = (i * 8) + j; //Compute which block is the free one
                            cache[0][i] |= 1 << j;   //Set the bit to 1 to indicate that it is now allocated
                            break;
                        }
                    }
                    if (freeBlock != -1)
                    {
                        break;
                    }
                }
                if (freeBlock == -1)
                {
                    throw "Error: No free block to allocate for file";
                }
                const int fileDescriptor2 = freeBlock;
                copy(reinterpret_cast<const unsigned char *>(&fileDescriptor2), reinterpret_cast<const unsigned char *>(&fileDescriptor2) + 4, &cacheDescriptorBlock[descriptorBlockIndex + 8]); //Record the new allocated block in the file descriptor
            }
            //Save current block (1st block) to the disk
            emulatedDisk->write_block(fileDescriptor[1], oft[i].buffer);

            //Bring the new block (2nd block) into the oft at index i
            emulatedDisk->read_block(freeBlock, oft[i].buffer);
        }

        oft[i].buffer[bufferPosition] = M[m];      //Copy the corresponding byte from M[m] to buffer at oft[i]
        m++;                                       //Increment m to get to the next index of the memory buffer
        n--;                                       //Decrement n, the number of bytes left to read
        oft[i].current_pos++;                      //Increment the current position
        bufferPosition = oft[i].current_pos % 512; //Recalculate the buffer position
    }

    if (oft[i].current_pos > oft[i].file_size)
    {
        oft[i].file_size = oft[i].current_pos; //Update it in the OFT first

        const int newFileSize = oft[i].current_pos; //Then update it in its file descriptor
        copy(reinterpret_cast<const unsigned char *>(&newFileSize), reinterpret_cast<const unsigned char *>(&newFileSize) + 4, &cacheDescriptorBlock[descriptorBlockIndex]);
    }

    return n;
}

int FS::seek(int i, int p)
{
    if (i < 0 || i > 3)
    {
        throw "Error: Invalid OFT index";
    }

    if (oft[i].file_size == -1)
    {
        throw "Error: No file opened at this OFT index";
    }
    if (p < 0 || p > oft[i].file_size)
    {
        throw "Error: Current position is past the end of file";
    }

    if ((oft[i].current_pos > 0 && ((oft[i].current_pos - 1) / 512) == p / 512) || (oft[i].current_pos / 512 == p / 512)) //If they belong in the same block then no other work is necessary, just change the current pos
    {
        oft[i].current_pos = p;
    }
    else //If p is in a different block than current_pos, save the current block to the disk and bring the new block into the oft
    {
        int fileDescriptor[4] = {-1, -1, -1, -1};                 //Used to store the file descriptor values after converting back from char arrays
        int descriptorBlock = (oft[i].descriptor_index / 32) + 1; //Reminder: descriptorBlock should be a value from 1-6
        int descriptorBlockIndex = (oft[i].descriptor_index % 32) * 16;

        unsigned char *cacheDescriptorBlock = cache[descriptorBlock]; //Grab the correct block of file descriptors and put into another variable for ease of use

        //Convert the char arrays back to ints and store them into the file descriptor array
        copy(&cacheDescriptorBlock[descriptorBlockIndex], &cacheDescriptorBlock[descriptorBlockIndex + 4], reinterpret_cast<unsigned char *>(&fileDescriptor[0]));
        copy(&cacheDescriptorBlock[descriptorBlockIndex + 4], &cacheDescriptorBlock[descriptorBlockIndex + 8], reinterpret_cast<unsigned char *>(&fileDescriptor[1]));
        copy(&cacheDescriptorBlock[descriptorBlockIndex + 8], &cacheDescriptorBlock[descriptorBlockIndex + 12], reinterpret_cast<unsigned char *>(&fileDescriptor[2]));
        copy(&cacheDescriptorBlock[descriptorBlockIndex + 12], &cacheDescriptorBlock[descriptorBlockIndex + 16], reinterpret_cast<unsigned char *>(&fileDescriptor[3]));

        int blockOfOldPos;
        if (oft[i].current_pos > 0 && oft[i].current_pos % 512 == 0)
        {
            blockOfOldPos = fileDescriptor[(oft[i].current_pos / 512)];
        }
        else
        {
            blockOfOldPos = fileDescriptor[(oft[i].current_pos / 512) + 1];
        }
        int blockOfNewPos;
        if (p > 0 && p % 512 == 0)
        {
            blockOfNewPos = fileDescriptor[(p / 512)];
        }
        else
        {
            blockOfNewPos = fileDescriptor[(p / 512) + 1];
        }

        //Save current block to the disk
        emulatedDisk->write_block(blockOfOldPos, oft[i].buffer);

        //Bring the new block into the oft at index i
        emulatedDisk->read_block(blockOfNewPos, oft[i].buffer);

        //Finally change the position after the new block is brought in
        oft[i].current_pos = p;
    }
    return p;
}

string FS::directory()
{
    seek(0, 0); //Seek to position 0 in the directory
    string returnDirectory = "";
    while (oft[0].current_pos < oft[0].file_size)
    {
        read(0, 100, 8);
        bool nameEmpty = true;
        for (int i = 100; i < 104; i++)
        {
            if (M[100] != '\0')
            {
                nameEmpty = false;
                break;
            }
        }
        //copy(&M[100], &M[104] + 4, reinterpret_cast<unsigned char *>(&nameAsInt)); //Convert the name field to an int
        if (!nameEmpty) //If the name field isn't 0 there is an entry at this position
        {
            int charCount = 0;
            for (int i = 0; i < 4; i++) {
                if (M[100 + i] == '\000') 
                {
                    break;
                }
                charCount++;
            }
            //First write the name into the string
            for (int i = 0; i < charCount; i++)
            {
                returnDirectory += M[100 + i];
            }

            int descriptorIndex = -1;
            copy(&M[104], &M[104] + 4, reinterpret_cast<unsigned char *>(&descriptorIndex));

            int fileSize = -1;                               //Used to store the file size after converting back from the char array
            int descriptorBlock = (descriptorIndex / 32) + 1; //Reminder: descriptorBlock should be a value from 1-6
            int descriptorBlockIndex = (descriptorIndex % 32) * 16;

            unsigned char *cacheDescriptorBlock = cache[descriptorBlock]; //Grab the correct block of file descriptors and put into another variable for ease of use

            //Convert the char arrays back to ints and store them into the file descriptor array
            copy(&cacheDescriptorBlock[descriptorBlockIndex], &cacheDescriptorBlock[descriptorBlockIndex + 4], reinterpret_cast<unsigned char *>(&fileSize));
            returnDirectory += " ";
            returnDirectory += to_string(fileSize);
            returnDirectory += " ";
        }
    }
    return returnDirectory.substr(0, returnDirectory.length() - 1);
}

string FS::read_memory(int m, int n)
{
    string memoryContents = "";
    for (int i = m; i < m + n; i++)
    {
        memoryContents += M[i];
        if (M[i] == '\0') {
            break;
        }
    }
    return memoryContents;
}

int FS::write_memory(int m, unsigned char *s)
{
    int s_length = strlen((char *)s);
    int s_index = 0;
    for (int i = m; i < m + s_length; i++)
    {
        M[i] = s[s_index];
        s_index++;
    }
    return s_length;
}

void FS::quit()
{
    //Close all of the OFT entries upon quitting
    for (int i = 0; i < 4; i++)
    {
        try //Try-catch block is necessary because not all OFT indices might be filled out.
        {
            close(i, true);
        }
        catch (const char *e)
        {
            continue;
        }
    }

    //Copy all of the contents of the cache back to the first 6 blocks of the disk
    for (int i = 0; i < 7; i++)
    {
        emulatedDisk->write_block(i, cache[i]);
    }
}