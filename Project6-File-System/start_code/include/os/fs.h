#ifndef INCLUDE_FS_H_
#define INCLUDE_FS_H_
#include "sched.h"
#include "time.h"

#define MAX_NAME_LEN    24
#define MAX_DIR_BLK     7

//mode of file
typedef enum {
    RD,
    WR,
    RDWR
} file_mode_t;

//initial addr
#define SUPERBLK_ADDR   0xa0f00200
#define BLKMAP_ADDR     0xa0f00400
#define INODEMAP_ADDR   0xa0f04400
#define INODE_ADDR      0xa0f04600
#define DATABLK_ADDR    0xa0f44600

#define VTP(X)  ((uint32_t)(X) & 0x1fffffff)

#define VBLK_SD_ADDR       0xffff000
#define SUPERBLK_SD_ADDR   0x20000000   //offset =512MB
#define BLKMAP_SD_ADDR     0x20000200   //16KB = 0x4000
#define INODEMAP_SD_ADDR   0x20004200   //512B = 0x200
#define INODE_SD_ADDR      0x20004400   //256KB = 0x40000
#define DATABLK_SD_ADDR    0x20044400

//size
#define SECTOR_SZ   0x200               //512B
#define BLK_SZ      0x1000              //4KB

//Magic number
#define MAGICNUM    0x4653              //"FS"

//superblock
typedef struct superblock{
    uint32_t magic_num; 
    uint32_t fs_sz; //file system size
    uint32_t start_sector;

    uint32_t blockmap_addr;
    uint32_t blockmap_offset;
    uint32_t blockmap_num;

    uint32_t inodemap_addr;
    uint32_t inodemap_offset;
    uint32_t inodemap_num;

    uint32_t inodes_addr;
    uint32_t inodes_offset;
    uint32_t inodes_num;

    uint32_t datablock_addr;
    uint32_t datablock_offset;
    uint32_t datablock_num;

    uint32_t inode_sz;
    uint32_t dentry_sz;
} superblock_t;

//16*4=64B
#define INODE_PERBLK    64
#define INODE_SIZE      64
typedef struct inode{
    uint32_t inum;
    file_mode_t mode; //r, r/w
    uint32_t ref;
    uint32_t used_sz;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t num; //used num of data-block (for file) or dentry (for dentry)
    uint32_t direct[MAX_DIR_BLK]; //direct
    uint32_t first;     //1st 
    uint32_t second;    //2nd
} inode_t;

//1 datablock : 4KB / 32B = 128
#define DENTRY_SIZE      64
typedef struct dentry {
    char name[MAX_NAME_LEN];
    uint32_t  type; //0: NONE, 1: file, 2: dentry, 3:".", 4:".."
    uint32_t inode_id;
} dentry_t;

#define MAX_NFD     64
typedef struct filedesc{
    uint32_t inum;
    uint32_t availability;
    uint32_t start_addr;
    uint32_t r_cur_pos;
    uint32_t w_cur_pos;
} filedesc_t;

superblock_t *superblock;
//uint32_t *superblk_buf;
uint8_t *blkmap;
uint8_t *inodemap;
uint32_t *inode_buf;
inode_t *inode;
inode_t *cur_inode;
dentry_t *cur_dentry;
int cur_dep;
char path[4][MAX_NAME_LEN];
filedesc_t opfile[MAX_NFD];


#define WB_INODE(inode_id)                                                
{                                                                         
    sdwrite((char *)(INODE_ADDR + BLK_SZ*((inode_id)/INODES_PERBLK)),  
    INODE_SD_ADDR + BLK_SZ*((inode_id)/INODES_PERBLK), BLK_SZ);           
}

#define WB_BLKSZ(datablk_addr, datablk_sd_addr)                        
{                                                                      
    sdwrite((char *)VTP(datablk_addr), datablk_sd_addr, BLK_SZ);      
}

#define CLEAR_BLKMAP(sd_addr)                                           
{                                                                       
    blkmap[((sd_addr) - INODE_SD_ADDR)/BLK_SZ/8] &=  ~(1 << ((sd_addr) - INODE_SD_ADDR)/BLK_SZ%8);                       
    sdwrite((char *)((INODEMAP_ADDR + ((sd_addr) - INODE_SD_ADDR)/BLK_SZ/8)&0xfffffe00), 
    (sd_addr) & 0xfffffe00, 0x200);                                          
}

#define cur_time (time_elapsed / 10000000)

int init_fs();
int do_mkfs();
int do_mkdir(char *sname);
int do_rmdir(char *sname);
int do_cd(char *dir);
void do_statfs();
int do_ls(char *dir);
int do_touch(char *sname);
int do_cat(char *sname);
int do_fopen(char *sname, int acess);
int do_fread(int fd, char *buff, int size);
int do_fwrite(int fd, char *buff, int size);
void do_close(int fd);
void do_cfs();
void write_inode_map(inode_id);
#endif