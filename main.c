// #include <stdio.h>
// #include <stdlib.h>
// #include <time.h>
// #include <string.h>
// #include <sys/stat.h>
// #include <stdint.h>
// #include <math.h>
// #pragma pack(1)

// //Global variable
// #define TRUE  1
// #define FALSE 0
// int g_StartData;
// int g_ReservedCluster = 8;
// int g_ReservedEntries = 8;
// int g_ReservedBytes = 512;
// /*
//     EEPROM Size - 1024 Bytes
//     256 Bytes - Root Directory
// */

// typedef struct __attribute__ ((__packed__)){
//     uint8_t type;
//     uint16_t block_location;
//     uint16_t file_size;
//     uint8_t name[7];
//     uint8_t terminator[1];
//     uint8_t ext[3];
// } FS_Entry;

// typedef struct __attribute__ ((__packed__)){
//     unsigned short parent_location;
//     unsigned short directory_location;
//     unsigned short bitmap_location;
//     unsigned short device_size;
// } FS_DirectoryEntry;

// // typedef struct __attribute__ ((__packed__)) {
// //     uint16_t sector_size;
// //     uint16_t cluster_size;
// //     uint16_t disk_size;
// //     uint16_t fat_start;
// //     uint16_t fat_length;
// //     uint16_t data_start;
// //     uint16_t data_length;
// //     char disk_name[16];
// // } FS_Boot;

// void create_partition(uint16_t sector_size, uint16_t cluster_size, uint16_t disk_size){
//     //Creating a blank 1024 size of file - EEPROM 1024 Bytes
//     printf("Creating New Disk..\n");

//     /*  --------------------- 
//         |   Directory Root  |   - 256 Bytes
//         ---------------------
//         |   FAT Entries     |   - 256 Bytes
//         ---------------------
//         |   User Data       |   - 512 Bytes
//         ---------------------
//         Directory Root - 256 / 32 = 8 clusters
//         FAT Entries    - 256 / 32 = 8 clusters
//         User Data      - 32 - 16 = 16 clusters

//         In all; 32 clusters in the disk
//         reserved space is 16 clusters. Data begins at cluster 17.
//     */
//     printf("Size of disk is: %d\n", disk_size);
//     printf("Size of a sector is: %d\n", sector_size);
//     printf("Size of a cluster in disk is: %d\n", cluster_size);
//     g_StartData = disk_size - g_ReservedBytes;
//     // 1. Erase the whole disk
//     FILE *fp = fopen("test.bin", "wb");
//     fseek(fp,(disk_size - 1), SEEK_SET);
//     fputc('\0',fp);

//     //2. Fill the clusters at cluster 9;
//     rewind(fp);
//     char *buf = calloc(cluster_size, sector_size);
//     printf("Allocating data in cluster 17 at Byte: %d\n", g_StartData);
//     for (int i = g_StartData; i < (disk_size - g_StartData); i++){
//         fwrite(buf, sizeof(uint8_t), sector_size * (cluster_size - (g_ReservedCluster + g_ReservedEntries)), fp);
//     }
//     printf("Finished writing Data clusters.\n");

//     //3. Creating Root Directory
//     FS_Entry *root = (FS_Entry*)malloc(sizeof(FS_Entry));
//     root->type = 0x03;
//     root->block_location = 0x0100;
//     root->file_size = 0x0615;
//     strcpy(root->name, "root");
//     strcpy(root->ext, "");
//     strcpy(root->terminator, "\0");
//     fseek(fp, 0, SEEK_SET);
//     fwrite(root, sizeof(FS_Entry), 1, fp);
//     printf("Root Directory created starting at byte: %d\n", 0);

//     //4. Creating FAT Entries


//     fclose(fp);
//     free(root);
// }

// int main(){
//     uint16_t sector_size = 32;
//     uint16_t cluster_size = 32;
//     uint16_t disk_size = 1024;

//     create_partition(sector_size, cluster_size, disk_size);
//     return 0;
// }

// void 

//Include AVR library
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "fsmain.h"

#define NULL_PTR -1

void create_dummydisk(const char *filename);
int get_block_pointer(lba_t block);
lba_t last_block_in_chain(lba_t block);
lba_t write_block_data(fdata_t* data);
void m_link(file_handle_t* fh);
void m_unlink(lba_t block);
void m_relink(lba_t block, lba_t target);

file_alloc_t alloc_table[EEPROM_FS_MAX_FILES + 1];
// Shortcut to next free block
lba_t* const next_free_block = &alloc_table[EEPROM_FS_MAX_FILES].data_block;

// Fetching the metadata from the Arduino
void init_eepromfs(const char *filename){
    printf("Initializing filesystem...\n");

    //Checking the boot sector;
    //eeprom_read_block(()) -> when using on duino, for now just using blank files
    FILE *fp;
    fp = fopen(filename, "rb");
    //fs_meta_t *stored_meta = (fs_meta_t*)malloc(sizeof(fs_meta_t));
    fseek(fp,0,SEEK_SET);
    fs_meta_t stored_meta;
    fread(&stored_meta, sizeof(stored_meta),1,fp);
    fclose(fp);
    printf("block size: %d\n", stored_meta.fs_size);
    if (stored_meta.block_size != EEPROM_FS_BLOCK_SIZE ||
    stored_meta.start_address != EEPROM_FS_START ||
    stored_meta.fs_size != EEPROM_FS_SIZE ||
    stored_meta.max_files != EEPROM_FS_MAX_FILES ||
    stored_meta.max_blocks_per_file != EEPROM_FS_MAX_BLOCKS_PER_FILE){
        format_eepromfs(FORMAT_QUICK);
    }
    // Need to check with Arduino!
    //eeprom_read_block((void*) &stored_meta, (void*)(EEPROM_FS_START + EEPROM_FS_META_OFFSET), sizeof(fs_meta_t));

    //Loading the FAT
    fp = fopen(filename, "rb");
    fseek(fp, (EEPROM_FS_START + EEPROM_FS_ALLOC_TABLE_OFFSET), SEEK_SET);
    fread(alloc_table, sizeof(alloc_table), 1, fp);
    printf("Done.\n");

    printf("Next free block: %d Bytes\n", (*next_free_block) * EEPROM_FS_BLOCK_SIZE);
    printf("Filesystem initialized!\n");
}

void create_dummydisk(const char *filename){
    // Create dummy disk - 1024 Bytes
    FILE *fp;
    fp = fopen(filename, "wb");
    fseek(fp, EEPROM_FS_SIZE - 1, SEEK_SET);
    fputc('\0', fp);
    fclose(fp);
}

void format_eepromfs(format_type_t f){
    printf("Formatting filesystem..\n");
    if (f == FORMAT_WIPE){
        //wipe_eeprom();
    }

    block_t block;
    if (f == FORMAT_FULL){
        //Clear block data
        for (uint16_t i = 0; i < EEPROM_FS_BLOCK_DATA_SIZE; i++){
            block.data[i] == 0;
        }
    }

    for (lba_t i = 0; i < EEPROM_FS_NUM_BLOCKS; i++){
        block.next_block = i - 1;
        if (f == FORMAT_FULL){
            //eeprom_update_block((void*) &block, get_block_pointer(i), EEPROM_FS_BLOCK_SIZE);
        } else {
            //m_relink(i, block.next_block);
            m_relink(i, block.next_block);
        }
    }

    printf("Printing allocation table\n");
    //Set all the allocation to 'null'
    file_alloc_t null;
    null.data_block = NULL_PTR;
    null.filesize = 0;

    for (uint16_t i = 0; i < EEPROM_FS_MAX_FILES; i++){
        alloc_table[i] = null;
    }

    file_alloc_t free;
    free.data_block = EEPROM_FS_NUM_BLOCKS - 1;
    free.filesize = 0;
    alloc_table[EEPROM_FS_MAX_FILES] = free;

    FILE *fp;
    fp = fopen("test_1.bin", "rb+");
    fseek(fp, (EEPROM_FS_START + EEPROM_FS_ALLOC_TABLE_OFFSET), SEEK_SET);
    fwrite(&alloc_table,sizeof(alloc_table), 1, fp);
    fclose(fp);

    printf("Done.\n");

    printf("Writing metadata...");
    fs_meta_t this_meta;
    this_meta.block_size = EEPROM_FS_BLOCK_SIZE;
    this_meta.start_address = EEPROM_FS_START;
    this_meta.fs_size = EEPROM_FS_SIZE;
    this_meta.max_files = EEPROM_FS_MAX_FILES;
    this_meta.max_blocks_per_file = EEPROM_FS_MAX_BLOCKS_PER_FILE;
    fp = fopen("test_1.bin","rb+");
    fseek(fp,(EEPROM_FS_START + EEPROM_FS_META_OFFSET),SEEK_SET);
    fwrite(&this_meta, sizeof(fs_meta_t), 1, fp);
    
    printf("Done..\n");
    printf("Successfully formatted the disk!\n");
    fclose(fp);
}

lba_t last_block_in_chain(lba_t block){
    if (block >= 0 && block < (lba_t) EEPROM_FS_NUM_BLOCKS){
        printf("Searching for last block in chain.. \n");

        block_t current_block;
        current_block.next_block = block;

        do{
            block = current_block.next_block;
            printf("checking.. %d\n", block);
            FILE *fp = fopen("test_1.bin","rb");
            fseek(fp,get_block_pointer(block),SEEK_SET);
            fread(&current_block,EEPROM_FS_BLOCK_SIZE,1,fp);
            fclose(fp);
        } while (current_block.next_block != NULL_PTR);
        printf("Last block in chain: %d\n", block);
        return block;
    } else {
        printf("Block %d is not part of a block chain.\n", block);
        return NULL_PTR;
    }
}

file_handle_t open_for_write(fname_t filename){
    printf("Preparing file %d for writing.\n", filename);

    if (filename > EEPROM_FS_MAX_FILES){
        filename = filename % EEPROM_FS_MAX_FILES;
        printf("Filename too large - truncated to %d.\n", filename);
    }

    file_handle_t fh;
    fh.filename = filename;
    fh.filesize = 0;
    fh.type = FH_WRITE;
    fh.first_block = NULL_PTR;
    fh.last_block = NULL_PTR;

    printf("File ready.\n");
    return fh;
}

void m_relink(lba_t block, lba_t target){
    if (block >= 0 && block < (lba_t) EEPROM_FS_NUM_BLOCKS){
        if (target >= NULL_PTR && target < (lba_t) EEPROM_FS_NUM_BLOCKS)
        {
            //write address only
            FILE *fp;
            fp = fopen("test_1.bin", "rb+");
            printf("Relinking block %d -> %d..", block, target);
            fseek(fp, get_block_pointer(block), SEEK_SET);
            fwrite(&target,sizeof(lba_t),1,fp);
            fclose(fp);
            printf("Done.. and Done.\n");
            
        } else {
            printf("Attempted to relink to invalid block %d.\n", target);
        }
    } else {
        printf("Attempted to write to invalid block %d.\n", block);
    }
}

int get_block_pointer(lba_t block){
   return  ((EEPROM_FS_START + EEPROM_FS_DATA_OFFSET
			+ ((block * EEPROM_FS_BLOCK_SIZE) % EEPROM_FS_SIZE)));
}

void write(file_handle_t* fh, const fdata_t* data, uint16_t size){
    if (fh->type == FH_WRITE || fh->type == FH_APPEND){
        if (fh->type == FH_APPEND && fh->filesize % EEPROM_FS_BLOCK_DATA_SIZE > 0)
        {
            // Last block of current file is incomplete.
            uint16_t overflow = fh->filesize % EEPROM_FS_BLOCK_DATA_SIZE;
            fdata_t* to_write[overflow + size];

            //Read from last block
            file_handle_t last_block_fh = *fh;
            last_block_fh.first_block = last_block_in_chain(alloc_table[fh->filename].data_block);
            last_block_fh.filesize = overflow;
            read(&last_block_fh, (fdata_t*) to_write);

            //Copy new data in to new array
            uint16_t offset = (uint16_t) to_write + (uint16_t)(overflow * sizeof(fdata_t));
            memcpy((void*)offset,data,size * sizeof(fdata_t));
            
            //Update existing variables;
            data = (const fdata_t*)to_write;
            size = overflow + size;
        }

        printf("Writing %d bytes to file %d.\n", size, fh->filename);

        uint16_t num_blocks;

        //Don't allow files any bigger than max blocks
        uint16_t blocks_in_use = alloc_table[fh->filename].filesize / EEPROM_FS_BLOCK_DATA_SIZE;
        if (blocks_in_use + size / EEPROM_FS_BLOCK_DATA_SIZE > EEPROM_FS_MAX_BLOCKS_PER_FILE)
        {
            num_blocks = EEPROM_FS_MAX_BLOCKS_PER_FILE - blocks_in_use;
            printf("File too large - write truncated to %d bytes.\n", num_blocks * EEPROM_FS_BLOCK_DATA_SIZE);
        }
        else {
            num_blocks = (size / EEPROM_FS_BLOCK_DATA_SIZE) + 1;
        }

        if (num_blocks > 0)
        {
            //Split data into blocks
            block_t block;
            uint16_t num_bytes = EEPROM_FS_BLOCK_DATA_SIZE;
            fh->first_block = *next_free_block;
            for (uint16_t i = 0; i < num_blocks; i++){
                // Don't write more than file's size
                if ((i + 1) * EEPROM_FS_BLOCK_DATA_SIZE > size)
                {
                    num_bytes = size % EEPROM_FS_BLOCK_DATA_SIZE;
                }

                for (uint16_t j = 0; j < num_bytes; j++)
                {
                    block.data[j] = data[i * EEPROM_FS_BLOCK_DATA_SIZE + j];
                    printf("data[%d] = %c\n", i * EEPROM_FS_BLOCK_DATA_SIZE + j, block.data[j]);
                }

                //update file handle data
                fh->last_block = write_block_data(block.data);
            }

            //In case the data was truncated, recalculate size
            if (size > num_blocks * EEPROM_FS_BLOCK_DATA_SIZE)
            {
                fh->filesize = num_blocks * EEPROM_FS_BLOCK_DATA_SIZE;
            } else {
                fh->filesize = size;
            }

            printf("File %d successfully written.\n", fh->filename);
        } else {
            printf("No more space available for file %d.\n", fh->filename);
        }
    } else {
        printf("Tried to write to readonly file handle '%d'\n", fh->filename);
    }
}

void read(file_handle_t *fh, fdata_t* buf){
    if (fh->first_block >= 0 && fh->first_block < (lba_t) EEPROM_FS_NUM_BLOCKS)
    {
        block_t block;
        block.next_block = fh->first_block;

        uint16_t i = 0;
        uint16_t num_bytes = EEPROM_FS_BLOCK_DATA_SIZE;

        do {
            printf("Reading from block %d...", block.next_block);
            FILE *fp = fopen("test_1.bin","rb");
            fseek(fp, get_block_pointer(block.next_block), SEEK_SET);
            fread(&block, EEPROM_FS_BLOCK_SIZE,1,fp);
            fclose(fp);
            printf("Done!\n");

            if ((i + 1) * EEPROM_FS_BLOCK_DATA_SIZE > fh->filesize)
            {
                num_bytes = fh->filesize % EEPROM_FS_BLOCK_DATA_SIZE;
            }

            //copy this block's data to the buffer
            for (uint16_t j = 0; j < num_bytes; j++){
                printf("buf[%d] = %c\n",i * EEPROM_FS_BLOCK_DATA_SIZE + j, block.data[j]);
                buf[i * EEPROM_FS_BLOCK_DATA_SIZE + j] = block.data[j];
            }
            i++;
        } while (block.next_block != NULL_PTR);
    } else {
        printf("Tried to read from null file handle.\n");
    }
}

lba_t write_block_data(fdata_t* data){
    lba_t write_to = *next_free_block;

    if (write_to >= 0 && write_to < (lba_t) EEPROM_FS_NUM_BLOCKS)
    {
        block_t current_block_data;
        FILE *fp = fopen("test_1.bin","rb");
        fseek(fp, get_block_pointer(write_to), SEEK_SET);
        fread(&current_block_data,EEPROM_FS_BLOCK_SIZE,1,fp);
        fclose(fp);
        *next_free_block = current_block_data.next_block;

        printf("Overwriting block %d...", write_to);

        void *addr = get_block_pointer(write_to) + (EEPROM_FS_BLOCK_SIZE - EEPROM_FS_BLOCK_DATA_SIZE);
        fp = fopen("test_1.bin","rb+");
        fseek(fp, addr,SEEK_SET);
        fwrite(data, EEPROM_FS_BLOCK_DATA_SIZE, 1, fp);
        fclose(fp);
        printf("Done.\n");
        printf("Next free block: %d\n", *next_free_block);

        return write_to;
    } else {
        printf("Attempted to write to invalid block %d.\n", write_to);
        return NULL_PTR;
    }
}

file_handle_t open_for_read(fname_t filename){
    printf("Preparing file %d for reading.\n", filename);

    if (filename > EEPROM_FS_MAX_FILES)
    {
        filename = filename % EEPROM_FS_MAX_FILES;
        printf("filename too large - truncated to %d.\n", filename);
    }

    file_handle_t fh;
    fh.filename = filename;
    fh.filesize = alloc_table[filename].filesize;
    fh.type = FH_READ;
    fh.first_block = alloc_table[filename].data_block;
    fh.last_block = NULL_PTR;

    if (fh.first_block == NULL_PTR){
        printf("File %d not found.\n", filename);
    } else {
        printf("File ready.\n");
    }

    return fh;
}

void close(file_handle_t *fh){
    printf("Finalising file %d.\n", fh->filename);
    if (fh->type == FH_APPEND)
    {
        fh->filesize += alloc_table[fh->filename].filesize;
        if (alloc_table[fh->filename].filesize > EEPROM_FS_BLOCK_DATA_SIZE)
        {
            lba_t last = last_block_in_chain(alloc_table[fh->filename].data_block);
            printf("Appending block %d to block %d..\n", fh->first_block, last);
            m_relink(last, fh->first_block);
            printf("Done.\n");
        } else {
            if (alloc_table[fh->filename].filesize > 0)
            {
                m_unlink(alloc_table[fh->filename].data_block);
            }
            m_link(fh);
        }
    } else {
        m_link(fh);
    }

    printf("Marking end of file %d.\n", fh->filename);

    //Mark the end of file
    m_relink(fh->last_block, NULL_PTR);

    printf("File %d successfully creted.\n", fh->filename);

}

void m_unlink(lba_t block){
    if (block >= 0 && block < (lba_t) EEPROM_FS_NUM_BLOCKS)
    {
        printf("Unlinking block %d.\n", block);

        //Get the current last free block;
        lba_t last_free = last_block_in_chain(*next_free_block);
        block_t last_free_block;
        FILE *fp = fopen("test_1.bin", "rb");
        fseek(fp, get_block_pointer(last_free), SEEK_SET);
        fread(&last_free_block, EEPROM_FS_BLOCK_SIZE, 1, fp);
        fclose(fp);
        last_free_block.next_block = block;

        //Write back address only
        fp = fopen("test_1.bin", "rb+");
        fseek(fp, get_block_pointer(last_free), SEEK_SET);
        fwrite(&last_free_block,sizeof(lba_t), 1, fp);
        fclose(fp);
        printf("Unlink successful.\n");
    } else {
        printf("Cannot unlink invalid block %d\n", block);
    }
}

void m_link(file_handle_t *fh){
    if (fh->first_block >= 0 && fh->first_block < (lba_t) EEPROM_FS_NUM_BLOCKS)
    {
        printf("Linking file %d to block %d.\n", fh->filename, fh->first_block);
        fname_t filename = fh->filename % EEPROM_FS_MAX_FILES;

        alloc_table[filename].filesize = fh->filesize;
        alloc_table[filename].data_block = fh->first_block;

        //Update table in memory
        void * alloc_offset = (void *)(EEPROM_FS_START + EEPROM_FS_ALLOC_TABLE_OFFSET +
        filename * sizeof(file_alloc_t));
        FILE *fp = fopen("test_1.bin","rb+");
        fseek(fp, alloc_offset, SEEK_SET);
        fwrite(&alloc_table[filename], sizeof(file_alloc_t), 1, fp);
        fclose(fp);

        //New free 
        void* free_offset = (void*) (EEPROM_FS_START + EEPROM_FS_ALLOC_TABLE_OFFSET
        + EEPROM_FS_MAX_FILES * sizeof(file_alloc_t));
        fp = fopen("test_1.bin", "rb+");
        fseek(fp, free_offset, SEEK_SET);
        fwrite(&alloc_table[EEPROM_FS_MAX_FILES], sizeof(file_alloc_t), 1, fp);
        fclose(fp);
        printf("Link successful.\n");
    } else {
        printf("Cannot link file %d to invalid block %d.\n", fh->filename,
        fh->first_block);
    }
}

int main(){
   // create_dummydisk("test_1.bin");
    init_eepromfs("test_1.bin");
    //        printf("Size of fs_meta: %d", sizeof(alloc_table));
    printf("\nLet's try creating a simple file..\n");
    file_handle_t fh = open_for_write(4);
    fdata_t contents[] = "jhhjvhjvjhgjgjhgjh!\n\0";
    write(&fh, contents, sizeof(contents));
    close(&fh);

    printf("That's about all!\n");

    printf("Reading file 6...\n");
    fh = open_for_read(4);
    fdata_t stored_contents[fh.filesize];
    read(&fh, stored_contents);
    printf("--> %s", stored_contents);
    return 0;
}