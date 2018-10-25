#include "unofat.h"


int main(void){
    create_dummydisk("test.bin");
    printf("%d\n",sizeof(FS_Entry));
    mount_disk("test.bin");
 
    //Let's try creating blank file
    char contents[] = "asdajshdhajsdjasjdabsdbasjdasjdbasjhd\0";
    create_file("test");
    write_to_file("test", contents);
    list();
    return 0;
}