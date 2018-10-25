#ifndef UNOFAT_H
#define UNOFAT_H

#include <stdint.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#define ROOT_ADDRESS 0x0
#define BLOCK_SIZE 32
#define DATA_BLOCK_SIZE (CLUSTER_SIZE)
#define DISK_SIZE 1024
#define FAT_OFFSET 16
#define DATA_OFFSET (ROOT_ADDRESS + FAT_OFFSET)
#define MAX_FILES 7
#define BLOCKS_PER_FILE 4
#define MAX_BLOCKS ((DISK_SIZE - 80)/BLOCK_SIZE)
#define EEPROM_FS_START 0x0
#define EEPROM_FS_SIZE 1024
#define EEPROM_FS_BLOCK_SIZE 21
#define EEPROM_FS_MAX_BLOCKS_PER_FILE 4
/* Prime number is recommended, but not mandatory */
#define EEPROM_FS_MAX_FILES 13

#define EEPROM_FS_META_OFFSET 0
#define EEPROM_FS_ALLOC_TABLE_OFFSET 10
#define EEPROM_FS_DATA_OFFSET 66
#define EEPROM_FS_NUM_BLOCKS ((EEPROM_FS_SIZE - EEPROM_FS_DATA_OFFSET) / EEPROM_FS_BLOCK_SIZE)
#define EEPROM_FS_BLOCK_DATA_SIZE (EEPROM_FS_BLOCK_SIZE - 2)

/*
    Data type sizes
    Unsigned short/uint16_t - 2 Bytes
    Unsigned char/uint8_t - 1 Byte

*/
FILE *file;

typedef struct __attribute__((__packed__)){
    uint16_t cluster_size;  //No of blocks per file
    uint16_t block_size;    //Bytes per block
    uint16_t disk_size;     //Disk Size
    uint16_t fat_offset;     //Offset to FAT
    uint16_t data_offset;    //Offset to Data
    uint16_t max_files;     //Max files in Disk
    uint16_t root_location; //Parent location
    unsigned char disk_letter[2];
} FS_Mbr; //16 Bytes - Boot sector;

typedef struct __attribute__((__packed__)){
    uint8_t id;
    char data[22];
    struct FS_Entry *next;
} FS_Entry; // 32 Bytes - File entry

typedef struct __attribute__((__packed__)){
    uint8_t filename[4];    // Name of the file
    uint16_t filesize;      // Size of the file
    uint16_t data_block;    // Location of physical block of data
} FS_Fat; //8 Bytes - FAT Table

//Disk handling
void create_dummydisk(const char *filename);
void mount_disk(const char *filename);
void format_disk(const char *filename);
void clear_disk();

//File Handling
void create_file(const char* filename);
void write_to_file(const char* filename, char contents[]);
void read_file(const char* filename);
void write(uint16_t filename);
void read(uint16_t filename);

//CLI Handling
void list();

#endif