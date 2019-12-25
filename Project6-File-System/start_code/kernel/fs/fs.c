#include "irq.h"
#include "type.h"
#include "sched.h"
#include "fs.h"
#include "time.h"
#include "string.h"

uint32_t inode_id_cur;
uint32_t valid_datablk_cur=45;
dentry_t dbuf[128];
static    char head_dir[30];
static    char tail_dir[30];

void write_inode_map(inode_id)
{
    /* inodemap = (uint8_t *)INODEMAP_ADDR;
    sdread((char *)inodemap, (uint32_t)INODEMAP_SD_ADDR, 0x200); */
    inodemap[inode_id/8]=inodemap[inode_id/8] | (1<<(7-(inode_id % 8)));
    sdwrite((char *)inodemap, (uint32_t)INODEMAP_SD_ADDR, 0x200);
}
void write_block_inode(inode_id)
{
    sdwrite((char *)(INODE_ADDR + BLK_SZ*((inode_id)/INODE_PERBLK)),  
    INODE_SD_ADDR + BLK_SZ*((inode_id)/INODE_PERBLK), BLK_SZ);          
}

void write_block_map(block_id)
{
    /* blkmap = (uint8_t *)BLKMAP_ADDR;
    sdread((char *)blkmap, (uint32_t)BLKMAP_SD_ADDR, 0x4000); */
    blkmap[block_id/8]=blkmap[block_id/8] | (1<<(7-(block_id % 8)));
    sdwrite((char *)blkmap, (uint32_t)BLKMAP_SD_ADDR, 0x4000);
}

void init_block_map(block_id)
{
    /* blkmap = (uint8_t *)BLKMAP_ADDR;
    sdread((char *)blkmap, (uint32_t)BLKMAP_SD_ADDR, 0x4000); */
    int i;
    for(i=0;i<block_id;i++)
        blkmap[i/8]=blkmap[i/8] | (1<<(7-(i % 8)));
    sdwrite((char *)blkmap, (uint32_t)BLKMAP_SD_ADDR, 0x4000);
}

uint32_t find_free_inode(){
    int i,j;
    uint32_t free_inode;
    for(i = 0; i < superblock->inodemap_num*0x200/8; i++)
        for(j = 0; j < 8; j++)
            if(!(inodemap[i] &  (0x80 >>j))){
                free_inode= 8*i+j;
                return free_inode;
            }
    return -1;
}

uint32_t find_free_block(){
    int i,j;
    uint32_t free_block;
    for(i = 0; i < superblock->blockmap_num*0x200/8; i++)
        for(j = 0; j < 8; j++)
           if(!(blkmap[i] &  (0x80 >>j))){
                free_block= 8*i+j;
                return free_block;
           }
    return -1;
}

uint32_t alloc_datablock(){
    //uint32_t datablock_cur= valid_datablk_cur++;
    uint32_t datablock_cur = find_free_block();
    write_block_map(datablock_cur);
    //do_print("find block cur addr:%x\n",DATABLK_SD_ADDR + (datablock_cur-45)*0x1000);
    return DATABLK_SD_ADDR + (datablock_cur-45)*0x1000;
}

uint32_t alloc_inode(){
    uint32_t inode_cur = find_free_inode();
    write_inode_map(inode_cur);
    return inode_cur;
}

void free_datablock(uint32_t block_addr){   //free data block map
    uint32_t datablock_cur = (block_addr-DATABLK_ADDR)/0x1000 +45;
/*     blkmap = (uint8_t *)BLKMAP_ADDR;
    sdread((char *)blkmap, (uint32_t)BLKMAP_SD_ADDR, 0x4000); */
    blkmap[datablock_cur/8]=blkmap[datablock_cur/8] & (~ ((uint8_t)(1<<(7-(datablock_cur % 8)))));
    sdwrite((char *)blkmap, (uint32_t)BLKMAP_SD_ADDR, 0x4000);
}

void free_inode(uint32_t inode_id){     //free inode map
    int i;
    for(i=0;i<MAX_DIR_BLK;i++)
        if(inode[inode_id].direct[i])
            free_datablock(inode[inode_id].direct[i]);
    
    inodemap[inode_id/8]=inodemap[inode_id/8] & (~ ((uint8_t)(1<<(7-(inode_id % 8)))));
    sdwrite((char *)inodemap, (uint32_t)INODEMAP_SD_ADDR, 0x200);
}


void init_entry(uint32_t ptr, uint32_t cur_inode_id, uint32_t parent_inode_id)
{

    bzero(dbuf, sizeof(dbuf));
    strcpy((char *)dbuf[0].name, (char *)".");
    dbuf[0].type = 3;
    dbuf[0].inode_id = cur_inode_id;
    strcpy((char *)dbuf[1].name, (char *)"..");
    dbuf[1].type = 4;
    dbuf[1].inode_id = parent_inode_id;
    sdwrite((char *)(dbuf), ptr, BLK_SZ);
    sdread((char *)(dbuf), ptr, BLK_SZ);    
}

int init_fs(){
	superblock = (superblock_t *)SUPERBLK_ADDR;
	sdread((char *)superblock,SUPERBLK_SD_ADDR,0x200);
	if(superblock->magic_num != MAGICNUM)
		return 0;
	//read block map
    blkmap = (uint8_t *)BLKMAP_ADDR;
    sdread((char *)(BLKMAP_ADDR), (uint32_t)BLKMAP_SD_ADDR, 0x4000);
    //read inode map
    inodemap = (uint8_t *)INODEMAP_ADDR;
    sdread((char *)(INODEMAP_ADDR), (uint32_t)INODEMAP_SD_ADDR, 0x200);
    //read inodes
    //inode_buf = (uint32_t *)INODE_ADDR;
    inode = (inode_t *)INODE_ADDR;
    sdread((char *)(INODE_ADDR), (uint32_t)INODE_SD_ADDR, 0x2000);
    //current dir
    cur_inode = inode;
    bzero((char *)current_running->opfile, sizeof(current_running->opfile));
    return 1;
}

int do_mkfs(){
	do_print("[FS] Start initialize filesystem!          \n");
    do_print("[FS] Setting superblock...                 \n");
    screen_reflush();
    bzero((void *)superblock, 0x200);

	superblock->magic_num=MAGICNUM;
    
    superblock->fs_sz=0x20000000;               // offset: 512MB
    superblock->start_sector=1048576;           // 512MB/512B = 1M

    superblock->blockmap_addr =BLKMAP_ADDR;
    superblock->blockmap_offset=1;
    superblock->blockmap_num=32;                // 16KB/512B = 32

    superblock->inodemap_addr=INODEMAP_ADDR;
    superblock->inodemap_offset=33;
    superblock->inodemap_num=1;                 // 512B/512B = 1

    superblock->inodes_addr=INODE_ADDR;
    superblock->inodes_offset=34;
    superblock->inodes_num=512;                 // 256KB/512B = 512

    superblock->datablock_addr=DATABLK_ADDR;
    superblock->datablock_offset=546;
    superblock->datablock_num=1048001;

    superblock->inode_sz=INODE_SIZE;
    superblock->dentry_sz=DENTRY_SIZE;
    sdwrite((char *)superblock, SUPERBLK_SD_ADDR, 0x200);

    do_print("magic : %x                                        \n",superblock->magic_num);
    do_print("num sector : %d ,start_sector : %d                \n",superblock->fs_sz/0x200,superblock->start_sector);
    do_print("block map offset : %d (%d)                        \n",superblock->blockmap_offset,superblock->blockmap_num);
    do_print("inode map offset : %d (%d)                        \n",superblock->inodemap_offset,superblock->inodemap_num);
    do_print("data offset : %d (%d)                             \n",superblock->datablock_offset,superblock->datablock_num);
    do_print("inode entry size : 64B, dir entry size : 32B      \n");

    do_print("[FS] Setting inode-map...                 \n");
    screen_reflush();
    inodemap = (uint8_t *)INODEMAP_ADDR;
    bzero((void *)inodemap, superblock->inodemap_num*0x200);
    sdwrite((char *)inodemap,INODEMAP_SD_ADDR, 0x200);
    write_inode_map(0);

    do_print("[FS] Setting block-map...                 \n");
    screen_reflush();
    blkmap = (uint8_t *)BLKMAP_ADDR;
    bzero((void *)blkmap, superblock->blockmap_num*0x200);
    sdwrite((char *)blkmap,BLKMAP_SD_ADDR, superblock->blockmap_num*0x200);
    int cnt;
    init_block_map(45);

    do_print("[FS] Setting inode...                     \n");
    inode = (inode_t *)INODE_ADDR;
    bzero((void *)inode, superblock->inodes_num*0x200);
    //sdwrite((char *)inode,INODE_SD_ADDR, superblock->inodes_num*0x200);
    sdread((char *)(INODE_ADDR), (uint32_t)INODE_SD_ADDR, 0x2000);
    inode_id_cur=0;
    inode[0].inum = inode_id_cur++;
    inode[0].i_mode = 1;
    inode[0].mode = O_RDWR;
    inode[0].ref =0;
    inode[0].used_sz =8;
    inode[0].ctime=get_timer();
    inode[0].mtime=get_timer();
    inode[0].num=2;
    inode->direct[0] = alloc_datablock(); 
    int i;
    for(i=1;i<MAX_DIR_BLK;i++)
        inode->direct[i] =0;
    write_block_inode(0);
    init_entry(inode[0].direct[0], 0, -1);

    do_print("[FS] Initialize filesystem finished!       \n");
    screen_reflush();

    cur_inode = inode;
    return 1;

}

void do_statfs(){
    if(superblock->magic_num != MAGICNUM){
        do_print("[ERROR] No File System!\n");
        return ;
    }
    int i, j;
    uint32_t used_block = 0, used_inodes = 0;
    for(i = 0; i < superblock->blockmap_num*0x200/8; i++)
        for(j = 0; j < 8; j++)
            used_block += (blkmap[i] >> j) & 1;
    for(i = 0; i < superblock->inodemap_num*0x200/8; i++)
        for(j = 0; j < 8; j++)
            used_inodes += (inodemap[i] >> j) & 1;
    do_print("Magic Number: 0x%x(FSFS)\n", superblock->magic_num);
    do_print("used block: %d/%d, start sector: %d (0x%x),\n", used_block, superblock->datablock_num, superblock->start_sector, superblock->start_sector);
    do_print("block map offset: %d, occupied sector: %d\n", superblock->blockmap_offset, superblock->datablock_num);
    do_print("inode map offset: %d, occupied sector: %d, used: %d\n", superblock->inodemap_offset, superblock->inodemap_num, used_inodes);
    do_print("inode offset: %d, occupied sector: %d\n", superblock->inodes_offset, superblock->inodes_num);
    do_print("data offset: %d, occupied sector: %d\n", superblock->datablock_offset, superblock->datablock_num);
    do_print("inode entry size: %dB, dir entry size : %dB\n",  sizeof(inode_t),sizeof(dentry_t));
    screen_reflush();
}

int do_mkdir(char *sname){
    if(superblock->magic_num != MAGICNUM){
        do_print("[ERROR] No File System!\n");
        return -1;
    }
    int i,j,k;
    for(i=0;i<cur_inode->num;i++){
        j = i / INODE_PERBLK;
        k = i % INODE_PERBLK;
        if(k==0){
            sdread((char *)dbuf, cur_inode -> direct[j],BLK_SZ);
        }
        if(dbuf[k].type==2&&!strcmp((char *)dbuf[k].name,sname)){
            return 0;
        }
    }
    j = i / INODE_PERBLK;
    k = i % INODE_PERBLK;
    if(k==0){
            sdread((char *)dbuf, cur_inode -> direct[j],BLK_SZ);
    }
    strcpy((char *)dbuf[k].name,sname);
    dbuf[k].type=2;
    i=dbuf[k].inode_id=alloc_inode();
    sdwrite((char *)dbuf,cur_inode->direct[j],BLK_SZ);
    cur_inode->ref++; 
    cur_inode->num++;
    cur_inode->used_sz += 4;
    write_block_inode(cur_inode->inum);
    inode[i].inum=i;
    inode[i].i_mode=1;
    inode[i].mode=O_RDWR;
    inode[i].used_sz = 8;
    inode[i].ctime=get_timer();
    inode[i].mtime=get_timer();
    inode[i].num=2;
    for(k = 0; k < MAX_DIR_BLK; k++)
        inode[i].direct[k] = 0;
    inode[i].direct[0]=alloc_datablock();
    write_block_inode(i);
    init_entry(inode[i].direct[j],inode[i].inum,cur_inode->inum);
    do_print("Successed! inum:%d addr:%x\n", inode[i].inum,inode[i].direct[j]);
    return 1;    
}

int do_rmdir(char *sname){
    if(superblock->magic_num != MAGICNUM){
        do_print("[ERROR] No File System!\n");
        return -1;
    }
    int i,j,k;
    for(i=0;i<cur_inode->num;i++){
        j = i / INODE_PERBLK;
        k = i % INODE_PERBLK;
        if(k==0){
            sdread((char *)dbuf, cur_inode -> direct[j],BLK_SZ);
        }
        if(dbuf[k].type==2&&!strcmp((char *)dbuf[k].name,sname)){
            dbuf[k].name[0] = '\0';
            dbuf[k].type = 0;
            dbuf[k].inode_id = -1;
            sdwrite((char *)dbuf, cur_inode->direct[j], BLK_SZ);      
            cur_inode->ref--;
            cur_inode->mtime = get_timer();
            cur_inode->used_sz -= 4;
            cur_inode->num--;
            write_block_inode(cur_inode->inum);
            free_inode(dbuf[k].inode_id);
            return 1;
        }
    }
    return 0;
}
char * get_head_dir(char *head,char *dir){
    int i=0,j;
    for(j=0;i<strlen(dir)&&dir[i]!='/';i++,j++){
        head[j]=dir[i];
    }
    head[j++]='\0';
    return head;
}

char * get_tail_dir(char *tail,char *dir){
    int i,j;
    for(i=0;i<strlen(dir)&&dir[i]!='/';i++)
        ;
    i++;
    for(j=0;i<strlen(dir);i++,j++)
        tail[j]=dir[i];
    tail[j]='\0';
    return tail;
}
int find=0;
int find_path(char * dir){
    static int dep=0;
    int inode_id_temp=cur_inode->inum;

    if(dep==0)
        find=0;
    
    if(dir[0]=='/'){
        cur_inode=&inode[0];
        if(strlen(dir)==1)
            return 1;
        int m;
        for(m=0;m<strlen(dir);m++)
            dir[m]=dir[m+1];
    }
    get_head_dir(head_dir,dir);
	get_tail_dir(tail_dir,dir);
    do_print("dep:%d head: %s, ?:%d, tail:%s  \n",dep,head_dir,tail_dir[0]=='\0',tail_dir);
    if(head_dir[0]=='\0')
		return 0;

    int i,j,k;
    for(i=0;i<cur_inode->num;i++){
        j = i / INODE_PERBLK;
        k = i % INODE_PERBLK;
        if(k==0){
            sdread((char *)dbuf, cur_inode -> direct[j],BLK_SZ);
            do_print("dbuf:%s cur_inum:%d\n",dbuf[2].name,cur_inode->inum);
        }
        if(!strcmp((char *)dbuf[k].name,head_dir)){
            do_print("dep:%d ?:%d \n",dep,tail_dir[0]=='\0');
            cur_inode = &inode[dbuf[k].inode_id];
            if(tail_dir[0]=='\0'){
                find=1;
		        return 1;
            }
            dep++;
            find_path(tail_dir);
        }
    }
    if(find==0){
        do_print("[ERROR] PATH NOT FOUND!\n");
        cur_inode = &inode[inode_id_temp];
        return 0;
    }
    else
        return 1;

}

int do_cd(char *dir){
    int inode_id_temp=cur_inode->inum;
    if(superblock->magic_num != MAGICNUM){
        do_print("[ERROR] No File System!\n");
        return -1;
    }
    if(dir[0]!='\0'){
        //find_path(dir);
        if(find_path(dir)==0 || cur_inode->i_mode != IMODE_DENTRY){
        do_print("[ERROR] PATH NOT FOUND!\n");
        cur_inode = &inode[inode_id_temp];
        return 0;
        }
    }
    return 1;
    // int inode_id_temp=cur_inode->inum;
    // int find=0;
    // if(superblock->magic_num != MAGICNUM){
    //     do_print("[ERROR] No File System!\n");
    //     return -1;
    // }
    
    // if(dir[0]=='/'){
    //     cur_inode=&inode[0];
    //     int m;
    //     for(m=0;m<strlen(dir);m++)
    //         dir[m]=dir[m+1];
    // }

    // get_head_dir(head_dir,dir);
    // //do_print("head:%s\n",head_dir);
	// get_tail_dir(tail_dir,dir);
    // //do_print("tail:%s\n",tail_dir);

    // if(head_dir[0]=='\0')
	// 	return 0;

    // int i,j,k;
    // for(i=0;i<cur_inode->num;i++){
    //     j = i / INODE_PERBLK;
    //     k = i % INODE_PERBLK;
    //     if(k==0){
    //         sdread((char *)dbuf, cur_inode -> direct[j],BLK_SZ);
    //     }
    //     if(!strcmp((char *)dbuf[k].name,head_dir) && cur_inode->i_mode==1 && inode[dbuf[k].inode_id].i_mode==1 ){
    //         cur_inode = &inode[dbuf[k].inode_id];
    //         if(tail_dir[0]=='\0'){
    //             find=1;
	// 	        return 1;
    //         }
    //         do_cd(tail_dir);
    //     }
    // }
    // if(find==0){
    //     do_print("[ERROR] PATH NOT FOUND!\n");
    //     cur_inode = &inode[inode_id_temp];
    //     //screen_reflush();
    //     return 0;
    // }
    // else
    //     return 1;

}

int do_ls(char *dir){
    int inode_id_temp=cur_inode->inum;
    if(superblock->magic_num != MAGICNUM){
        do_print("[ERROR] No File System!\n");
        return -1;
    }
    if(dir[0]!='\0'){
        if(find_path(dir)==0 || cur_inode->i_mode != IMODE_DENTRY){
            do_print("[ERROR] PATH NOT FOUND!\n");
            cur_inode = &inode[inode_id_temp];
            return 0;
        }
    }
    int i,j,k;

    for(i=0;i<cur_inode->num;i++){
        j = i / INODE_PERBLK;
        k = i % INODE_PERBLK;
        if(k==0){
            sdread((char *)dbuf, cur_inode -> direct[j],BLK_SZ);
        }
    }
    for(i=0;i<cur_inode->num;i++){
        if(dbuf[i].name[0]!='\0')
            do_print("%s\n",dbuf[i].name);
    }
    //screen_reflush();
    cur_inode = &inode[inode_id_temp];
    return 1;
}

int do_touch(char *sname){
    if(superblock->magic_num != MAGICNUM){
        do_print("[ERROR] No File System!\n");
        return -1;
    }
    int i,j,k;
    for(i=0;i<cur_inode->num;i++){
        j = i / INODE_PERBLK;
        k = i % INODE_PERBLK;
        if(k==0){
            sdread((char *)dbuf, cur_inode -> direct[j],BLK_SZ);
        }
        if(dbuf[k].type==2&&!strcmp((char *)dbuf[k].name,sname)){
            return 0;
        }
    }
    j = i / INODE_PERBLK;
    k = i % INODE_PERBLK;
    if(k==0){
            sdread((char *)dbuf, cur_inode -> direct[j],BLK_SZ);
    }
    strcpy((char *)dbuf[k].name,sname);
    dbuf[k].type=1;
    i=dbuf[k].inode_id=alloc_inode();
    sdwrite((char *)dbuf,cur_inode->direct[j],BLK_SZ);
    cur_inode->ref++; 
    cur_inode->num++;
    cur_inode->used_sz += 4;
    write_block_inode(cur_inode->inum);
    inode[i].inum=i;
    inode[i].i_mode=0;
    inode[i].mode=O_RDWR;
    inode[i].used_sz = 8;
    inode[i].ctime=get_timer();
    inode[i].mtime=get_timer();
    inode[i].num=2;
    for(j = 0; j < MAX_DIR_BLK; j++)
        inode[i].direct[j] = 0;
    inode[i].direct[0]=alloc_datablock();
    write_block_inode(i);
    bzero(dbuf, sizeof(dbuf));
    sdwrite((char *)dbuf, inode[i].direct[0], BLK_SZ);
    return 1;    
}

int do_cat(char *sname){
    inode_t * inode_cur_tmp = cur_inode;
    if(find_path(sname)==0 || cur_inode->i_mode != IMODE_FILE){
        do_print("[ERROR] PATH NOT FOUND!\n");
        cur_inode = inode_cur_tmp;
        return 0;
    }
    int i,j,k;
    for(i=0;i<cur_inode->num;i++){
        j = i / INODE_PERBLK;
        k = i % INODE_PERBLK;
        if(k == 0){
            bzero((char *)dbuf,BLK_SZ);
            sdread((char *)dbuf, cur_inode -> direct[j], BLK_SZ);
        }
    }
    for(i = 0;i < cur_inode->used_sz;i++){
        do_print("%c",dbuf[i]);
    }
    cur_inode = inode_cur_tmp;
    return 1;
}

int alloc_fd(){
    int i;
    for(i=0;i<MAX_FD_NUM;i++){
        if(current_running->opfile[i].inum==0)
        return i;
    }
    return -1;
}
int do_fopen(char *sname, int access){
    inode_t * inode_cur_tmp = cur_inode;
    if(!do_cd(sname))
        return -1;
        int i,j,k;
    for(i=0;i<cur_inode->num;i++){
        j = i / INODE_PERBLK;
        k = i % INODE_PERBLK;
        if(k==0){
            bzero((char *)dbuf,BLK_SZ);
            sdread((char *)dbuf, cur_inode -> direct[j], BLK_SZ);
        }
    }
    if(cur_inode->i_mode == 0 && cur_inode -> mode != access)
        return -1;
    int fd_id= alloc_fd();
    current_running->opfile[fd_id].inum = cur_inode ->inum;
    current_running->opfile[fd_id].access = access;
    current_running->opfile[fd_id].r_cur_pos = 0;
    current_running->opfile[fd_id].w_cur_pos = 0;
    current_running->opfile[fd_id].start_cur_pos = 0;
    cur_inode = inode_cur_tmp;
    return fd_id;
}

int do_fread(int fd, char *buff, int size){
    inode_t * inode_cur_tmp = &inode[current_running->opfile[fd].inum];
    if(current_running->opfile[fd].access != O_RD && current_running->opfile[fd].access != O_RDWR)
        return -1;
    
    
    
    

}

int do_fwrite(int fd, char *buff, int size){
    inode_t * inode_cur_tmp = &inode[current_running->opfile[fd].inum];
    if(current_running->opfile[fd].access != O_WR && current_running->opfile[fd].access != O_RDWR)
        return -1;

}

void do_close(int fd){
    int k = current_running->opfile[fd].inum;
    inode[k].mtime = get_timer();
    write_block_inode(inode[k].inum);
    bzero((char *)&current_running->opfile[fd], sizeof(current_running->opfile[fd]));
}