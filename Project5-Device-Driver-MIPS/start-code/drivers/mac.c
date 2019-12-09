#include "mac.h"
#include "irq.h"

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
void mac_irq_handle(void)
{
    if(!queue_is_empty(&recv_block_queue))
        do_unblock_one(&recv_block_queue);
    clear_interrupt();
}

void irq_enable(int IRQn)
{
    reg_write_32(0xbfd0105c, 0x8);
}

static invalid_num = 0;

void mac_recv_handle(mac_t *test_mac)
{
    desc_t *recv_desc = (desc_t *)test_mac->rd;
    int i, j, print_location = 4;
    while(ch_flag < 64){
        if(!(recv_desc[ch_flag].tdes0 & 0x80000000)){
            recv_flag[ch_flag] = 1;
            printf_recv_buffer(test_mac->daddr,recv_desc);
            ch_flag++;
        }
        else{
            recv_flag[ch_flag] = (uint32_t)&recv_desc[ch_flag].tdes0;
            sys_move_cursor(1, print_location);
            printf("Waiting for receiving %dth package.   \n", i);
            sys_wait_recv_package();
        }
    }
    printf("Receive %d valid packages and %d invalid packages!  \n", 64-invalid_num, invalid_num);

}

static uint32_t printf_recv_buffer(uint32_t recv_buffer,uint32_t recv_desc)
{
    sys_move_cursor(1, print_location + 1);
    printf(" %dth recv_buff: \n", ch_flag);
    if(recv_desc[ch_flag].tdes0 & 0xf8cf){
        invalid_num++;
        printf("Received invalid package. \n");
    }
    uint32_t *data = (uint32_t *)(recv_buffer + 0x400 * ch_flag);
    for(j = 0; j < 256; j++)
        printf("%x ", data[j]);
    printf("\n");
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
    desc_t *recv_buf = (desc_t *)rd;
    for(i = 0; i < 64; i++)
        recv_buf[i].tdes0 = 0x80000000;
    
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
    desc_t *send_buf = (desc_t *)td;
    for(i = 0; i < 64; i++)
        send_buf[i].tdes0 = 0x80000000;
    
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
}

void do_wait_recv_package(void)
{
    current_running->status = TASK_BLOCKED;
    current_running->my_queue = &recv_block_queue;
    queue_push(&recv_block_queue, current_running);
    do_scheduler();
}