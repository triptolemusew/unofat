#ifndef FSMAIN_H
#define FSMAIN_H

#define EEPROM_FS_START 0x0
#define EEPROM_FS_SIZE 1024
#define EEPROM_FS_BLOCK_SIZE 16
#define EEPROM_FS_MAX_BLOCKS_PER_FILE 4
/* Prime number is recommended, but not mandatory */
#define EEPROM_FS_MAX_FILES 13

#define EEPROM_FS_META_OFFSET 0
#define EEPROM_FS_ALLOC_TABLE_OFFSET 10
#define EEPROM_FS_DATA_OFFSET 66
#define EEPROM_FS_NUM_BLOCKS ((EEPROM_FS_SIZE - EEPROM_FS_DATA_OFFSET) / EEPROM_FS_BLOCK_SIZE)
#define EEPROM_FS_BLOCK_DATA_SIZE (EEPROM_FS_BLOCK_SIZE - 2)
typedef int16_t lba_t;
typedef uint16_t fname_t;
typedef char fdata_t;
#pragma pack(push,1)

typedef struct __attribute__((__packed__)){
    lba_t next_block;
    fdata_t data[EEPROM_FS_BLOCK_DATA_SIZE];
} block_t;

typedef struct __attribute__((__packed__)){
    uint16_t filesize;
    lba_t data_block;
} file_alloc_t;

typedef struct __attribute__((__packed__)){
    uint16_t block_size;
    uint16_t start_address;
    uint16_t fs_size;
    uint16_t max_files;
    uint16_t max_blocks_per_file;
} fs_meta_t;

enum handle_type{
    FH_READ, FH_WRITE, FH_APPEND
};

typedef struct __attribute__((__packed__)){
    fname_t filename;
    uint16_t filesize;
    enum handle_type type;
    lba_t first_block;
    lba_t last_block;
} file_handle_t;

#pragma pack(pop)

typedef enum format_type{
    FORMAT_FULL, FORMAT_QUICK, FORMAT_WIPE
} format_type_t;


void init_eepromfs(const char *);
void format_eepromfs(format_type_t f);
file_handle_t open_for_write(fname_t filename);
file_handle_t open_for_append(fname_t filename);
file_handle_t open_for_read(fname_t filename);

void close(file_handle_t *fn);
void write(file_handle_t *fn, const fdata_t* data, uint16_t size);
void read(file_handle_t * fh, fdata_t* buf);
void delete(fname_t filename);
void dump_eeprom();
void wipe_eeprom();

#endif