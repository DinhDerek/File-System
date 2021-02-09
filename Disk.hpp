class Disk
{
private:
    unsigned char **disk;

public:
    Disk();
    void read_block(int b, unsigned char *rw_buffer);  //convert to unsigned char*
    void write_block(int b, unsigned char *rw_buffer); //convert to unsigned char*
};