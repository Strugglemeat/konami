#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <stdio.h>
#include <string.h>

#define DATA_SIZE 65536
#define MAX_RAW_SIZE 31
#define MAX_LENGTH 33
#define MAX_ZERO_LENGTH 257

int extensive_output = 1;

struct lz_t
{
    uint16_t length, offset;
};

typedef enum
{
    rle_A0 = 0xA0,
    rle_C0 = 0xC0,
    rle_E0 = 0xE0
} type_e;

struct  rle_t
{
    uint16_t size, data;
    uint8_t ar[MAX_LENGTH];
    type_e type;
};

rle_t search_rle (uint8_t*, uint8_t*, uint32_t, uint16_t);
lz_t search_lz (int ,unsigned char* , int);

uint16_t   write_lz (uint8_t*, uint16_t, lz_t);
uint16_t   write_rle (uint8_t*, uint16_t, rle_t);
uint16_t   write_raw (uint8_t*, uint16_t, uint8_t*, uint16_t);

using namespace std;

int main(int argc, char **argv)
{
    unsigned char in_buf[DATA_SIZE];
    unsigned char out_buf[DATA_SIZE];
    unsigned int in_size;

    rle_t rle;
    lz_t lz;

    cout << ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::" << endl;
    cout << ":: KONAMI SNES COMPRESSOR  by proton - modified by bankbank            ::" << endl;
    cout << ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::" << endl;

    //char pausevar;

    if (argc < 3)
    {
        fprintf(stderr, "Bad parameters");
        exit(-1);
    }

    char *type;
    unsigned int t;

    FILE *in_file;
    FILE *out_file;

    in_file = fopen(argv[1], "rb");
    if (in_file==NULL)
    {
        fputs ("Input file error",stderr);
        exit (1);
    }

    out_file = fopen(argv[2], "wb");
    if (out_file==NULL)
    {
        fputs ("Output file error",stderr);
        exit (1);
    }

    if (argc==1)
    {
        type = argv[1];
        if (type==NULL)
        {
            fputs ("Type error",stderr);
            exit (1);
        }
    }

    fseek(in_file,0,SEEK_END);
    in_size=ftell(in_file);
    fseek(in_file,0,SEEK_SET);

    cout << hex << "Game type: " << (int)t;
    cout << hex << "\nSize of data to be compressed: " << (int)in_size;

    if (in_size > DATA_SIZE)
    {
        cout << "\ninput size is larger than data size (65536)" << endl;
        return 0;
    } 

    for (int i=0; i<DATA_SIZE; i++) in_buf[i]=0x00;
    for (int i=0; i<DATA_SIZE; i++) out_buf[i]=0x00;

    fread(in_buf,sizeof(unsigned char),in_size,in_file);
    fclose(in_file);

    uint16_t  in_pos = 0;
    uint16_t  out_pos = 2; //first two bytes are to state the size of the uncompressed data

    uint8_t  raw[MAX_RAW_SIZE];
    uint16_t raw_size = 0;

    if(extensive_output==1)cout << hex << "\nBeginning compression...\n";

    while (in_pos < in_size)
    {
        //cout << hex << "\nInput position = [" << setw(4) << (int) in_pos << "]";
        //cout << hex << "\tin_buf[" << (int)in_pos << "] = " << setw(2) << "[" << (int)in_buf[in_pos] << "]";
        if(extensive_output==1)cout << hex << "\nInput position = [" << (int)in_pos << "] = " << setw(4) << "[" << (int)in_buf[in_pos] << "]";
       
        lz.offset=0;
        lz.length=0;

        if((int)in_buf[in_pos+1]!=0x20)//hacky, prevent it from wrongly compressing the tilemap data that starts with 00 20 00 20
            {   
                rle = search_rle(in_buf, in_buf + in_pos, in_size, t);
            }

        if(rle.size!=00 && extensive_output==1)
        {
            cout << "\n";
            cout << "RLE char to repeat is: 0x" << rle.data << endl;
            cout << "repeat number of times is: 0x" << rle.size << endl;
            //cin>>pausevar; //pauses for input
        }

        if (rle.size <= MAX_LENGTH && in_pos < in_size - 1) //MAX_LENGTH = 33
        {
            if(extensive_output==1)cout << "\nbeginning LZ search" << endl; 
            lz = search_lz(in_pos,in_buf,in_size);
            if(extensive_output==1 && lz.offset!=00)cout << hex << "\n*lz.offset=" << (int)lz.offset << endl;
            if(extensive_output==1 && lz.length!=00) cout << hex << "*lz.length=" << (int)lz.length << endl;
        }
        else lz.length = 0;

        if(extensive_output==1 && raw_size>0)cout << hex << "\n#raw_size:" << (int)raw_size << endl;

        if (lz.length >= 2&&lz.length>rle.size)
        {
            if (lz.length == 2&&raw_size>0)
            {
                raw[raw_size++] = in_buf[in_pos++];
                if (raw_size == MAX_RAW_SIZE)
                {
                    out_pos += write_raw(out_buf, out_pos, raw, raw_size);
                    if(extensive_output==1)cout << "write raw 1" << endl;
                    raw_size = 0;
                }
            }
            else
            {
                out_pos += write_raw(out_buf, out_pos, raw, raw_size);
                if(extensive_output==1)cout << "write raw 2" << endl;
                raw_size = 0;
                out_pos += write_lz(out_buf, out_pos, lz);
                if(extensive_output==1)cout << "write lz 1" << endl;
                in_pos += lz.length;
            }
        }
        else
        {
            if (rle.size >= 2 && rle.size >= lz.length)
            {
                if (rle.size >= 2 && rle.size >= lz.length && rle.type==rle_E0)
                {
                    out_pos += write_raw(out_buf, out_pos, raw, raw_size);
                    if(extensive_output==1)cout << "write raw 3" << endl;
                    raw_size = 0;
                    out_pos += write_rle(out_buf, out_pos, rle);
                    if(extensive_output==1)cout << "write rle 1" << endl;
                    in_pos += rle.size;
                }
                else
                {
                    if(rle.size==2)
                    {
                        if(raw_size!=0)
                        {
                            raw[raw_size++] = in_buf[in_pos++];
                            if (raw_size == MAX_RAW_SIZE)
                            {
                                out_pos += write_raw(out_buf, out_pos, raw, raw_size);
                                if(extensive_output==1)cout << "write raw 4" << endl;
                                raw_size = 0;
                            }
                        }
                        else
                        {
                            out_pos += write_raw(out_buf, out_pos, raw, raw_size);
                            if(extensive_output==1)cout << "write raw 5" << endl;
                            raw_size = 0;
                            out_pos += write_rle(out_buf, out_pos, rle);
                            if(extensive_output==1)cout << "write rle 2" << endl;
                            in_pos += rle.size;
                        }
                    }
                    else
                    {
                        out_pos += write_raw(out_buf, out_pos, raw, raw_size);
                        if(extensive_output==1)cout << "write raw 6" << endl;
                        raw_size = 0;
                        out_pos += write_rle(out_buf, out_pos, rle);
                        if(extensive_output==1)cout << "write raw 3" << endl;
                        in_pos += rle.size;
                    }
                }
            }
            else
            {
                raw[raw_size++] = in_buf[in_pos++];
                if (raw_size == MAX_RAW_SIZE)
                {

                    out_pos += write_raw(out_buf, out_pos, raw, raw_size);
                    if(extensive_output==1)cout << "write raw 7" << endl;
                    raw_size = 0;
                }
            }
        }

    //if(extensive_output==1)cout << hex << "\nlast wrote: " << "[" << (int)out_buf[out_pos-1] << "] at out_pos: "<< (int)out_pos-1 << endl;

    }

    out_pos += write_raw(out_buf, out_pos, raw, raw_size);
    if(extensive_output==1)cout << hex << "\nlast wrote: " << "[" << (int)out_buf[out_pos-1] << "] at out_pos: "<< (int)out_pos-1 << " (final write)"<< endl;

    cout << hex << "\nCompressed data size: " << (int)out_pos << endl;

    out_buf[0]=(out_pos)&0xff;
    out_buf[1]=((out_pos)&0xff00)>>8;

    fwrite(out_buf,1,out_pos,out_file); //fwrite ( const void * ptr, size_t size, size_t count, FILE * stream );
    fclose(out_file);
    return 0;
}

rle_t search_rle (uint8_t *start, uint8_t *current, uint32_t insize, uint16_t type)
{
    rle_t candidate;
    candidate.size=0;
    candidate.data=0;
    size_t size;

    uint8_t val[MAX_LENGTH];


//rle_A0...0xA0-0xBF

    if (current[0]==0x00 && current[1]!=0x00 && current[2]==0x00 && current[3]!=0x00)
    {
        size_t rep;
        if(extensive_output==1)cout << "\n//rle_A0 0xA0-0xBF";
        for (size = 0; size <= (MAX_LENGTH*2) && current + size < start + insize-1; size += 2)
        {
            if (current[size]!=0x00) break;
        }

        int s=0;

        for (rep = 1; rep <= size && current + rep < start + insize-1; rep += 2)
        {

            if (current[rep]==0x00) break;
            val[s++]=current[rep];
        }

        rep=rep-1;

        for (int i=0; i<(rep/2); i++)
        {
            candidate.ar[i] = val[i];
        }

        if (rep > (MAX_LENGTH*2)) rep = (MAX_LENGTH*2);
        if (rep >= 4 && rep > candidate.size)
        {
            candidate.size = rep;
            candidate.type = rle_A0;

            if(extensive_output==1)cout << hex << "\nsearch_rle: size = " << size << ", type = rle_A0)";
        }

    }


//rle_C0...0xC0-0xDF


    for (size = 0; size <= MAX_LENGTH && current + size < start + insize && current[0]!=0x00; size++)
        if (current[size] != current[0]) break;

    if (size > MAX_LENGTH) size = MAX_LENGTH;
    if (size >= 2 && size > candidate.size)
    {
        candidate.size = size;
        candidate.data = current[0];
        candidate.type = rle_C0;

        if(extensive_output==1)cout << hex << "\nsearch_rle: (size = " << size << ", type = rle_C0)";

    }

//rle_E0...0xE0-0xFF


    if (type==1)
    {

        for (size = 0; size <= MAX_ZERO_LENGTH && current + size < start + insize && current[0]==0x00; size++)
            if (current[size] != current[0]) break;


        if (size > MAX_ZERO_LENGTH) size = MAX_ZERO_LENGTH;
        if (size >= 2 && size > candidate.size)
        {
            candidate.size = size;
            candidate.data = current[0];
            candidate.type = rle_E0;

            if(extensive_output==1)cout << hex << "\nsearch_rle: (size = " << size << ", type = rle_E0) (type1)";

        }

    }

    else
    {


        for (size = 0; size <= MAX_LENGTH && current + size < start + insize && current[0]==0x00; size++)
            if (current[size] != current[0]) break;

        if (size > MAX_LENGTH) size = MAX_LENGTH;
        if (size >= 2 && size > candidate.size)
        {
            candidate.size = size;
            candidate.data = current[0];
            candidate.type = rle_E0;

            if(extensive_output==1)cout << hex << "\nsearch_rle: (size = " << size << ", type = rle_E0) (type0)";


        }

    }

    return candidate;
};

uint16_t write_lz (uint8_t *out, uint16_t out_pos, lz_t lz)
{
    uint16_t size = lz.length - 2;
    int outsize;
    int lz_pointer;
    lz.offset=(lz.offset+0x3df)&0x3ff;
    size=(((size)<<2)&0xFC)<<8;
    lz_pointer=lz.offset+size;
    out[out_pos++]=lz_pointer>>8;
    out[out_pos++]=lz_pointer&0xff;
    outsize = 2;

    //if(extensive_output==1)cout << "write write_lz func" << endl;
    if(extensive_output==1)cout << hex << "write_lz func wrote: " << "[" << (int)out[out_pos-1] << "] at out_pos: "<< (int)out_pos-1 << endl;


    return outsize;
};

uint16_t write_rle (uint8_t *out, uint16_t out_pos, rle_t rle)
{
    uint16_t size;
    int outsize;
    if (rle.type == rle_E0)
    {
        if(rle.size<=MAX_LENGTH)
        {
            size=rle.size-2;
            out[out_pos++] =  rle.type | size;
            outsize = 1;
        }
        else
        {
            size=rle.size-2;
            out[out_pos++] =  0xff;
            out[out_pos++] =  size;
            outsize = 2;
        }
    }
    else
    {
        if (rle.type == rle_A0)
        {
            size=(rle.size/2)-2;
            out[out_pos++] =  rle.type | size;
            for (int i=0; i<(rle.size/2); i++)
                out[out_pos++] = rle.ar[i];
            outsize = 1+(rle.size/2);
        }
        else
        {
            size=rle.size-2;
            out[out_pos++] =  rle.type | size;
            out[out_pos++] = rle.data;
            outsize = 2;
        }
    }

    //if(extensive_output==1)cout << "write write_rle func" << endl;
    
    if(extensive_output==1)cout << hex << "write_rle func wrote: " << "[" << (int)out[out_pos-1] << "] at out_pos: "<< (int)out_pos-1 << endl;

    return outsize;
};

uint16_t write_raw (uint8_t *out, uint16_t out_pos, uint8_t *in, uint16_t insize)
{
    if (!insize) return 0;
    uint16_t size = insize;
    int outsize;
    out[out_pos++] = 0x80|size;
    outsize = insize + 1;
    memcpy(&out[out_pos], in, insize);

    //if(extensive_output==1)cout << "write write_raw func" << endl;
    if(extensive_output==1)cout << hex << "write_raw func wrote: " << "[" << (int)out[out_pos-1] << "] at out_pos: "<< (int)out_pos-1 << endl;
    
    return outsize;
}


lz_t search_lz (int pos,unsigned char* buffer, int inputsize) //lz = search_lz(in_pos,in_buf,in_size);
{
    lz_t variant;
    variant.length=0;
    variant.offset=0;
    int lz_off=0;
    int lz_len=0;
    int match=0;

    int win;
    win=pos-0x3df;
    if (win<0)
    {
        if(extensive_output==1)cout << "search_lz win was reset to 0" << endl;
        win=0; 
    } 

    for (int i=win; i<pos+MAX_LENGTH && MAX_LENGTH + i < MAX_LENGTH + pos && i+MAX_LENGTH < inputsize && pos < inputsize; i++)
    {
        if ((buffer[i]==buffer[pos]))
        {
            match=1;
            while(buffer[i+match]==buffer[pos+match])
            {
                if (match>=MAX_LENGTH)
                {
                    if(extensive_output==1)cout << hex << "search_lz WE BROKE due to match>=MAX_LENGTH (33 bytes) (a)" << endl;
                    break;
                }
                match++;
            }

            if(match>lz_len)
            {
                lz_len=match;
                lz_off=i;
            }
        }

        if(match>=MAX_LENGTH)
        {
            if(extensive_output==1)cout << hex << "search_lz WE BROKE due to match>=MAX_LENGTH (33 bytes) (b)" << endl;
            lz_len=MAX_LENGTH;
            break;
        }
    }

    if (lz_len >= 2&&(lz_len+pos) )
    //if (lz_len >= 2&&(lz_len+pos) < inputsize)
    {
        variant.length=lz_len;
        variant.offset=lz_off;
        if(extensive_output==1)cout << hex << "search_lz: (length = " << lz_len << ", offset = " << lz_off << ")";
    }
    else 
    {
        if(extensive_output==1)cout << "search_lz: lz_len was greater than or equal to current pos plus in_size of "<< (int)inputsize << endl;
    }

    return variant;
}