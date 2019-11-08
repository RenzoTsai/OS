#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void write_bootblock(FILE *image, FILE *bbfile, Elf32_Phdr *Phdr);
Elf32_Phdr *read_exec_file(FILE *opfile);
uint8_t count_kernel_sectors(Elf32_Phdr *Phdr);
void extent_opt(Elf32_Phdr *Phdr_bb, Elf32_Phdr *Phdr_k, int kernelsz);

Elf32_Phdr *read_exec_file(FILE *opfile)
{
	Elf32_Phdr *phdr_ptr;
	Elf32_Ehdr head;
	phdr_ptr=(Elf32_Phdr*)malloc(sizeof(Elf32_Phdr));

	fread(&head,sizeof(Elf32_Ehdr),1,opfile);
	fseek(opfile,head.e_phoff,SEEK_SET);
	fread(phdr_ptr,sizeof(Elf32_Phdr)*head.e_phnum,1,opfile);
	//printf("e_phnum:%d\n",head.e_phnum);
	// printf("addr:%x\n",phdr_ptr);
	return phdr_ptr;
}

uint8_t count_kernel_sectors(Elf32_Phdr *Phdr)
{
	uint8_t num;
	num=(Phdr->p_memsz-1)/512+1;						//结果上取整
	// printf("p_memsz:%d\n",Phdr->p_memsz);
	//printf("num:%d\n",num);
	return num;

}

void write_bootblock(FILE *image, FILE *file, Elf32_Phdr *phdr)
{
	char *temp;
	temp=(char *)malloc(sizeof(char)*512);
	char zero[512]="";
	uint16_t end=0x55aa;
	
    fseek(file, phdr->p_offset, SEEK_SET); 			//get program from bootblock
    fread(temp,phdr->p_filesz,1,file);

    fwrite(temp,phdr->p_filesz,1,image);			//write bootblock to image

    if(phdr->p_filesz%512){								
    	fwrite(zero,1,512-2-phdr->p_filesz%512,image);	//add zero
    	fwrite(&end,2,1,image);							//add 0x55aa in the end
    }

    free(temp);
}

void write_kernel(FILE *image, FILE *knfile, Elf32_Phdr *Phdr, int kernelsz)
{
	Elf32_Ehdr kn_head;							//get Ehdr of kernel
	fseek(knfile,0,SEEK_SET);
	fread(&kn_head,sizeof(Elf32_Ehdr),1,knfile);
	fseek(knfile,0,SEEK_SET);

	char *temp;
	char zero[512]="";
	int i=0;

	Elf32_Phdr total_phdr [kn_head.e_phnum];	//get all Phdr(s) of kernel
	fseek(knfile, kn_head.e_phoff, SEEK_SET);
    fread(total_phdr, sizeof(Elf32_Phdr), kn_head.e_phnum, knfile);

    temp = (char*)malloc(sizeof(char) * kernelsz);

    for(i = 0; i < kn_head.e_phnum; i++){		//get program from kernel
		fseek(knfile, total_phdr[i].p_offset, SEEK_SET);
		fread(temp,total_phdr[i].p_filesz, 1, knfile);
    }
    
    fwrite(temp, kernelsz, 1, image);			//write kernel to image

     if(kernelsz % 512){						//add zero in the end
		 fwrite(zero, 1, 512-kernelsz%512, image);
     }
    free(temp);

}

void record_kernel_sectors(FILE *image, uint8_t kernelsz)
{
	fseek(image,509,SEEK_SET);
	fwrite(&kernelsz,1,1,image); //kernelsz is kernel's sector_num
}

void extent_opt(Elf32_Phdr *Phdr_bb, Elf32_Phdr *Phdr_k, int kernelsz)
{
	printf("kernelsz : 0x%x\n",kernelsz );
    printf("virtual addredd: 0x%x\n", Phdr_bb->p_vaddr);
    printf("segment size in file: 0x%x\n", Phdr_bb->p_filesz+Phdr_k->p_filesz);
    printf("segment size in memory: 0x%x\n", Phdr_bb->p_memsz+Phdr_k->p_memsz);
    printf("sector_num:%d\n",(Phdr_k->p_memsz+511)/512 );

}

int get_kernelsz( FILE *knfile, Elf32_Phdr *Phdr){
	Elf32_Ehdr kn_head;
	int i=0,total_size=0;

	fseek(knfile,0,SEEK_SET);						//get Ehdr of kernel
	fread(&kn_head,sizeof(Elf32_Ehdr),1,knfile);
	fseek(knfile,0,SEEK_SET);
	
	
	Elf32_Phdr total_phdr [kn_head.e_phnum];		//get all Phdr(s) of kernel
	printf("kernel ph_num:%d\n",kn_head.e_phnum);
	fseek(knfile, kn_head.e_phoff, SEEK_SET);
    fread(total_phdr, sizeof(Elf32_Phdr), kn_head.e_phnum, knfile);


	for(i = 0; i < kn_head.e_phnum; i++){			//get total size of kernel
		fseek(knfile, total_phdr[i].p_offset, SEEK_SET);
		total_size += total_phdr[i].p_memsz;
    }
    fseek(knfile, 0, SEEK_SET);
    return total_size;

}

int main()
{
	uint8_t sector_num;int kernelsz;
	FILE* bootblock_file,*kernel_file;
	Elf32_Phdr *bootblock_phdr,*kernel_phdr; 

	bootblock_file=fopen("bootblock","rb");
	kernel_file=fopen("main","rb");

	bootblock_phdr=read_exec_file(bootblock_file);
	kernel_phdr=read_exec_file(kernel_file);
	sector_num=count_kernel_sectors(kernel_phdr);
	FILE* image_file=fopen("image","wb");

	write_bootblock(image_file,bootblock_file,bootblock_phdr);
	kernelsz=get_kernelsz(kernel_file,kernel_phdr);
	sector_num=count_kernel_sectors(kernel_phdr);
	write_kernel(image_file,kernel_file,kernel_phdr,kernelsz);

	record_kernel_sectors(image_file,sector_num);
	extent_opt(bootblock_phdr,kernel_phdr,kernelsz);

	fclose(bootblock_file);
	fclose(kernel_file);
	fclose(image_file);
	return 0;
	

}