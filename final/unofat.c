#include "unofat.h"

FS_Fat alloc_table[MAX_FILES];
uint16_t* remaining_block = &alloc_table[MAX_FILES].data_block;

void create_dummydisk(const char *filename){
    printf("Creating a blank disk.\n");
    file = fopen(filename, "wb");
    fseek(file, EEPROM_FS_SIZE - 1, SEEK_SET);
    fputc('\0', file);
    fclose(file);
}

void mount_disk(const char *filename){
    //Extracting Data from the disk.
    //Format it if it's blank
    printf("Checking content of the disk..\n");

    file = fopen(filename, "rb");
    FS_Mbr mbr;
    fread(&mbr, sizeof(FS_Mbr), 1, file);
    fclose(file);

    if (mbr.disk_size != EEPROM_FS_SIZE ||
    mbr.block_size != EEPROM_FS_BLOCK_SIZE){
        printf("Unrecognized disk.\n Formatting it now..\n");
        format_disk(filename);
    }

}

void format_disk(const char * filename){
    //1. Write the Boot sector - the first 16 Bytes
    file = fopen(filename, "rb+");
    FS_Mbr boot;
    boot.block_size = BLOCK_SIZE;
    boot.fat_offset = FAT_OFFSET;
    boot.data_offset = DATA_OFFSET;
    boot.max_files = MAX_FILES;
    boot.cluster_size = BLOCKS_PER_FILE;
    boot.disk_size = DISK_SIZE;
    boot.disk_letter[0] = 'A';
    boot.disk_letter[1] = '\0';
    boot.root_location = ROOT_ADDRESS;
    
    printf("Writing boot sector..\n");
    fwrite(&boot,sizeof(FS_Mbr),1,file);
    printf("Done..\n");
    printf("Allocating FAT Table\n");

    //2. Allocating FAT Table
    FS_Fat fat;
    fat.data_block = -1;
    fat.filesize = 0x0;
    strcpy(fat.filename, "blnk");
    for (uint16_t i = 0; i < MAX_FILES; i++){
        alloc_table[i] = fat;
    }

    FS_Fat free;
    free.data_block = MAX_BLOCKS - 1;
    free.filesize = 0x0;
    strcpy(fat.filename, "dirc");
    alloc_table[MAX_FILES] = free;

    file = fopen(filename, "rb+");
    fseek(file, (FAT_OFFSET), SEEK_SET);
    fwrite(&alloc_table, sizeof(alloc_table),1,file);
    fclose(file);

    //3. Allocating User Data
    printf("No. of free disk: %d Bytes\n", MAX_BLOCKS * BLOCK_SIZE);

    file = fopen(filename, "rb+");
    fseek(file, (DATA_OFFSET + sizeof(alloc_table)), SEEK_SET);
    //printf("Alloc Table Size: %d\n", sizeof(alloc_table));

    // 4. Creating Root directory
    FS_Entry *root = NULL, **pp = &root;
    for (uint16_t i = 0; i < MAX_BLOCKS; i++){
        *pp = calloc(1, sizeof(**pp));
        (*pp)->id = i;
        memset((*pp)->data,0,sizeof((*pp)->data));
        pp = &(*pp)->next;
    }
    *pp = NULL;
    fseek(file, 80, SEEK_SET);

    for(FS_Entry *ptr = root; ptr; ptr = ptr->next){
        fwrite(ptr, sizeof(FS_Entry), 1, file);
    }

    for (FS_Entry *ptr = root; ptr; ptr = ptr->next){
        if (ptr->id == 2){
            memset(ptr->data,0,sizeof(ptr->data));
            fseek(file,80+(32*ptr->id), SEEK_SET);
            fwrite(ptr, sizeof(FS_Entry),1,file);
            printf("%d",sizeof(FS_Entry));
        }
    }
    //5. Allocate the rest of the data blocks
    fclose(file);
    printf("Done!\n");
    printf("Successfully formatted the disk.\n");
}

void create_file(const char* filename){
    //1. Checking the free allocation table
    printf("Creating file with name: %s\n", filename);
    strcpy(alloc_table[0].filename, filename);
    // Will replace logical mapping soon
    alloc_table[0].filesize = 0x0;  // Blank file
    alloc_table[0].data_block = 80; //Next free block;
    file = fopen("test.bin","rb+");
    fseek(file,16,SEEK_SET);
    fwrite(&alloc_table[0], sizeof(FS_Fat),1,file);
    fclose(file);
}

void write_to_file(const char* filename, char contents[]){
    for (uint16_t i = 0; i < MAX_FILES; i++){
        if (strcmp(alloc_table[i].filename, filename) == 0){
            printf("Found it!\n");
            uint16_t logical_block = alloc_table[i].data_block;
            file = fopen("test.bin","rb+");
            fseek(file,logical_block, SEEK_SET);
        } 
    }

    printf("Writing %s to file %s with %d Bytes\n", contents, filename, sizeof(contents));
    //1. Writing to the allocated FAT
    FS_Entry new_entry;
    memcpy(new_entry.data, contents, sizeof(contents));
    new_entry.id = 0;
    new_entry.next = NULL;

    fwrite(&new_entry, sizeof(FS_Entry), 1, file);
    rewind(file);

    //2. Update the FAT table
    fseek(file, sizeof(FS_Mbr), SEEK_SET);
    alloc_table[0].filesize = sizeof(contents);
    fwrite(&alloc_table[0], sizeof(FS_Fat),1,file);
    fclose(file);

}

void read_file(const char *filename){
    
}

void list(){
    FS_Fat checking_table[MAX_FILES];
    file = fopen("test.bin","rb");
    fseek(file, sizeof(FS_Mbr), SEEK_SET);
    for (uint16_t i = 0; i < MAX_FILES; i++){
        fread(&checking_table[i], sizeof(FS_Fat), 1, file);
    }

    //Prints the content
    for(uint16_t i = 0; i < MAX_FILES; i++){
        printf("%d: <%s> <%d Bytes>\n",i,checking_table[i].filename, checking_table[i].filesize);
    }
}