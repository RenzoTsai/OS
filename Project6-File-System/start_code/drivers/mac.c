#include "mac.h"
#include "irq.h"
#include "type.h"
#include "sched.h"

#define NUM_DMA_DESC 48
queue_t recv_block_queue;
uint32_t recv_flag[PNUM] = {0};
uint32_t ch_flag;
uint32_t mac_cnt = 0;
uint32_t reg_read_32(uint32_t addr)
{
    return *((uint32_t *)addr);
}
uint32_t read_register(uint32_t base, uint32_t offset)
{
    uint32_t addr = base + offset;
    uint32_t data;

    data = *(volatile uint32_t *)addr;
    return data;
}

void reg_write_32(uint32_t addr, uint32_t data)
{
    *((uint32_t *)addr) = data;
}

static void gmac_get_mac_addr(uint8_t *mac_addr)
{
    uint32_t addr;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0Low);
    mac_addr[0] = (addr >> 0) & 0x000000FF;
    mac_addr[1] = (addr >> 8) & 0x000000FF;
    mac_addr[2] = (addr >> 16) & 0x000000FF;
    mac_addr[3] = (addr >> 24) & 0x000000FF;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0High);
    mac_addr[4] = (addr >> 0) & 0x000000FF;
    mac_addr[5] = (addr >> 8) & 0x000000FF;
}

void print_tx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("send buffer mac->saddr=0x%x ", mac->saddr);
    printf("mac->saddr_phy=0x%x ", mac->saddr_phy);
    printf("send discrb mac->td_phy=0x%x\n", mac->td_phy);
#if 0
    desc_t *send=mac->td;
    for(i=0;i<mac->pnum;i++)
    {
        printf("send[%d].tdes0=0x%x ",i,send[i].tdes0);
        printf("send[%d].tdes1=0x%x ",i,send[i].tdes1);
        printf("send[%d].tdes2=0x%x ",i,send[i].tdes2);
        printf("send[%d].tdes3=0x%x ",i,send[i].tdes3);
    }
#endif
}

void print_rx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("recieve buffer add mac->daddr=0x%x ", mac->daddr);
    printf("mac->daddr_phy=0x%x ", mac->daddr_phy);
    printf("recieve discrb add mac->rd_phy=0x%x\n", mac->rd_phy);
    desc_t *recieve = (desc_t *)mac->rd;
#if 0
    for(i=0;i<mac->pnum;i++)
    {
        printf("recieve[%d].tdes0=0x%x ",i,recieve[i].tdes0);
        printf("recieve[%d].tdes1=0x%x ",i,recieve[i].tdes1);
        printf("recieve[%d].tdes2=0x%x ",i,recieve[i].tdes2);
        printf("recieve[%d].tdes3=0x%x\n",i,recieve[i].tdes3);
    }
#endif
}

static uint32_t printk_recv_buffer(uint32_t recv_buffer)
{
}

static uint32_t printf_recv_buffer(uint32_t recv_buffer)
{
    int i, print_location = 5;
    sys_move_cursor(1, print_location + 1);
    uint32_t *data = (uint32_t *) recv_buffer;
    for(i = 0; i < 256; i++)
        printf("%x ", data[i]);
    printf("\n");
    return 1;
}

void mac_irq_handle(void)
{
    // check whether new recv packet is arriving
    if(!queue_is_empty(&recv_block_queue))
        do_unblock_one(&recv_block_queue);
    clear_interrupt();
}

void irq_enable(int IRQn)
{
    reg_write_32(0xbfd0105c, 0x8);
}


/**
 * Clears all the pending interrupts.
 * If the Dma status register is read then all the interrupts gets cleared
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 * you will use it in task3
 */
void clear_interrupt()
{
    uint32_t data;
    data = reg_read_32(0xbfe11000 + DmaStatus);
    reg_write_32(0xbfe11000 + DmaStatus, data);
}




void mac_recv_handle(mac_t *test_mac)
{
    desc_t *recv_desc = (desc_t *)test_mac->rd;
    int i, j, print_location = 4;
    int ignore =0;
    while(ch_flag < 64+ignore){
        uint32_t *data = (uint32_t *)(test_mac->daddr + 0x400 * (ch_flag%64));
        uint8_t *head =(uint8_t *)(test_mac->daddr + 0x400 * (ch_flag%64));
        if(!(head[0]==0x00&&head[1]==0x55&&head[2]==0x7b&&head[3]==0xb5&&head[4]==0x7d&&head[5]==0xf7) &&!(recv_desc[ch_flag%64].tdes0 & 0x80000000)){
           recv_desc[ch_flag%64].tdes0 = 0x80000000;  //在起始位置重置无效包数目的描述符和寄存器
           reg_write_32(DMA_BASE_ADDR + 0x8, 1);
           memset((void *)data , '\0' ,BUFFSIZE);
           ignore++;
           ch_flag++;
           sys_move_cursor(1, print_location-2);
           printf("[MAC RECV TASK]Received %d ignored package!\n",ignore);
        }
        else if(!(recv_desc[ch_flag%64].tdes0 & 0x80000000)){
            recv_flag[ch_flag] = 1;
            //sys_move_cursor(1, print_location);
            //printf("[MAC RECV TASK] %dth recv_buff: \n", ch_flag);
            // if(!(head[0]==0x00&&head[1]==0x55&&head[2]==0x7b&&head[3]==0xb5&&head[4]==0x7d&&head[5]==0xf7)){
            //    sys_move_cursor(1, 3);
            //    printf("[MAC RECV TASK]Received %dth ignored package! ch_flag=%d %x\n",ignore,ch_flag,recv_desc[ch_flag].tdes0);
            //    recv_desc[ch_flag].tdes0 = 0x80000000;
            //    reg_write_32(DMA_BASE_ADDR + 0x8, 1);
            //    ignore++;
            //    recv_flag[ch_flag]=0;
            // }
            // else 
            // if(recv_desc[ch_flag%64].tdes0 & 0xf8cf){
            //     invalid_num++;
            //     sys_move_cursor(1, print_location);
            //     printf("[MAC RECV TASK]Received invalid package. \n");
            // }
            // for(j = 0; j < 256; j++)
            //     printf("%x ", data[j]);
            // printf("\n");
            ch_flag++;
        }
        else{
            recv_flag[ch_flag%64] = (uint32_t)&recv_desc[ch_flag%64].tdes0;
            sys_move_cursor(1, print_location-1);
            printf("[MAC RECV TASK]Waiting for receiving %dth package.(ignore=%d) \n", ch_flag,ignore);
            sys_wait_recv_package();
            }
    }
    int valid_num=0,invalid_num = 0;
    for (i = 0; i < PNUM; i++)
    {
        uint32_t * Recv_desc = (uint32_t *)(test_mac->rd + i * 16);
        desc_t * recv = (desc_t *)Recv_desc;

        sys_move_cursor(1, print_location-1);
        printf("[MAC RECV TASK]%d recv buffer,r_desc( 0x%x) =0x%x:          \n", i, Recv_desc, *(Recv_desc));
        if(recv_desc[i].tdes0 & 0xf8cf){
            invalid_num++;
            sys_move_cursor(1, print_location);
            printf("[MAC RECV TASK]Received invalid package. \n");
        }

        uint32_t recv_buffer = recv->tdes2;
        //valid_num += printf_recv_buffer((recv->tdes2 | 0xa0000000));
        //sys_sleep(1);
        printf("\n");
    }

    sys_move_cursor(1, print_location);
    printf("[MAC RECV TASK]Receive %d valid packages and %d invalid packages!  \n", 64-invalid_num, invalid_num);

}

void check_recv(mac_t *test_mac)
{
    // do_print("de:%x ch:%d de:%x\n",test_mac,ch_flag,test_mac2);
    desc_t *recv_desc = (desc_t *)test_mac->rd;
    while(ch_flag < 64){
        if(!(recv_desc[ch_flag].tdes0 & 0x80000000)){
            recv_flag[ch_flag] = 1;
            ch_flag++;
        }
    }
    if(!queue_is_empty(&recv_block_queue))
        do_unblock_one(&recv_block_queue);
}




void set_sram_ctr()
{
    *((volatile unsigned int *)0xbfd00420) = 0x8000; /* 使能GMAC0 */
}
static void s_reset(mac_t *mac) //reset mac regs
{
    uint32_t time = 1000000;
    reg_write_32(mac->dma_addr, 0x01);

    while ((reg_read_32(mac->dma_addr) & 0x01))
    {
        reg_write_32(mac->dma_addr, 0x01);
        while (time)
        {
            time--;
        }
    };
}
void disable_interrupt_all(mac_t *mac)
{
    reg_write_32(mac->dma_addr + DmaInterrupt, DmaIntDisable);
    return;
}

void set_mac_addr(mac_t *mac)
{
    uint32_t data;
    uint8_t MacAddr[6] = {0x00, 0x55, 0x7b, 0xb5, 0x7d, 0xf7};
    uint32_t MacHigh = 0x40, MacLow = 0x44;
    data = (MacAddr[5] << 8) | MacAddr[4];
    reg_write_32(mac->mac_addr + MacHigh, data);
    data = (MacAddr[3] << 24) | (MacAddr[2] << 16) | (MacAddr[1] << 8) | MacAddr[0];
    reg_write_32(mac->mac_addr + MacLow, data);
}

uint32_t do_net_recv(uint32_t rd, uint32_t rd_phy, uint32_t daddr)
{

    reg_write_32(DMA_BASE_ADDR + 0xc, rd_phy);
    reg_write_32(GMAC_BASE_ADDR, reg_read_32(GMAC_BASE_ADDR) | 0x4);
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02200002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));

    int i;
    desc_t *recv_des = (desc_t *)rd;
    for(i = 0; i < 64; i++)
        recv_des[i].tdes0 = 0x80000000;
    
    for(i = 0; i < 64; i++)
        reg_write_32(DMA_BASE_ADDR + 0x8, 1);

    // for(i = 0; i < 64; i++)
    // {
    //     if(!(recv_buf[i].tdes0 & 0x80000000))
    //     {
    //         ch_flag = i;
    //         recv_flag[i] = recv_buf[i].tdes0;
    //     }
    //     recv_buf[i].tdes0 = 0x80000000;
    // }

    return 0;
}

void do_net_send(uint32_t td, uint32_t td_phy)
{

    reg_write_32(DMA_BASE_ADDR + 0x10, td_phy);

    // MAC rx/tx enable

    reg_write_32(GMAC_BASE_ADDR, reg_read_32(GMAC_BASE_ADDR) | 0x8);                    // enable MAC-TX
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02202000); //0x02202002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));

    int i;
    desc_t *send_des = (desc_t *)td;
    for(i = 0; i < 64; i++)
        send_des[i].tdes0 = 0x80000000;
    
    for(i = 0; i < 64; i++)
        reg_write_32(DMA_BASE_ADDR + 0x4, 1);
}

void do_init_mac(void)
{
    mac_t test_mac;
    uint32_t i;

    test_mac.mac_addr = 0xbfe10000;
    test_mac.dma_addr = 0xbfe11000;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum

    set_sram_ctr(); /* 使能GMAC0 */
    s_reset(&test_mac);
    disable_interrupt_all(&test_mac);
    set_mac_addr(&test_mac);
    //DO IN TASK3
    reg_write_32(0xbfd01064, 0xffffffff);
    reg_write_32(0xbfd01068, 0xffffffff);
    reg_write_32(0xbfd0106c, 0);
}

void do_wait_recv_package(void)
{
    current_running->status = TASK_BLOCKED;
    current_running->which_queue = &recv_block_queue;
    queue_push(&recv_block_queue, current_running);
    do_scheduler();
}