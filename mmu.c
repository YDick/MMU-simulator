#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#define PAGE_SIZE           256 //bytes
#define TLB_SIZE            16
#define FRAME_SIZE          256 //bytes
#define NUM_FRAMES          256

// parse virtual/logical address into offset and page
#define V_PAGE_OFFSET_MASK    0x000000ff
#define V_PAGE_OFFSET_SHIFT   0
#define V_PAGE_NUM_MASK       0x0000ff00
#define V_PAGE_NUM_SHIFT      8

// frame number is last byte in page table entry
#define PT_FRAME_NUM_MASK   0x0000000ff
#define PT_FRAME_SHIFT      0
#define PT_VALID_MASK       0x800000000 //4 bytes
#define PT_COUNTER_MASK     0x00fffff00
#define PT_COUNTER_SHIFT    2

#define TLB_FRAME_MASK      0x000000ff
#define TLB_FRAME_SHIFT     0
#define TLB_PAGE_MASK       0x0000ff00
#define TLB_PAGE_SHIFT      8
#define TLB_VALID_MASK      0x80000000

// default memsize
static int MEM_SIZE = 256;
int PAGE_TABLE_SIZE = 256; //shud be the same as MEM_SIZE

// stats
double page_fault_count;
double TLB_hit_count;
int addr_count;

// outputs
int page;
int offset;
int frame;

int clock;


long int page_table[256] = {0}; //should be == MEM_SIZE
int TLB[TLB_SIZE] = {0};
char phys_mem[PAGE_SIZE * 256] = {0};

int TLB_index;

FILE *bs;

int getLeastRecentlyUsedFrame() {
    // counter or stack...?
    int oldestTime = INT_MAX;
    int time;
    int oldestFrame;

    for(int i = 0; i < MEM_SIZE; i++){
        time = (page_table[i] & PT_COUNTER_MASK) >> PT_COUNTER_SHIFT;
        if(time < oldestTime && page_table[i] & PT_VALID_MASK) {
            oldestTime = time;
            oldestFrame = i;
        }
    }

    return oldestFrame; 
}

int getFrameToFill() {
    static int frame_i = -1;
    static bool mem_full = false;

    if(frame_i >= MEM_SIZE ){
        mem_full = true;
    }

    if (!mem_full) {
        frame_i++;
    }
    else { //more memory than physical memory, need to swap out a frame to make room
        frame_i = getLeastRecentlyUsedFrame();
        printf("frame %d\n", frame_i);
    }
    return frame_i;
}



int getFrameFromPageTable(int page){

    int frame = (page_table[page] & PT_FRAME_NUM_MASK) >> PT_FRAME_SHIFT;

    // if page hasnt been filled yet from backingstore
    if(!(page_table[page] & PT_VALID_MASK) || page > MEM_SIZE) {
        // pageFault(page);
        frame = getFrameToFill();

        FILE *fp = bs;
        fseek(fp, page * PAGE_SIZE, SEEK_SET);
        fread(&phys_mem[frame * PAGE_SIZE], FRAME_SIZE, 1, fp);

        // put frame no in PT
        // page_table[page] = page_table[page] & !(0x00000000 | PT_COUNTER_MASK) ;
        page_table[page] = PT_VALID_MASK | ((frame & PT_FRAME_NUM_MASK) << PT_FRAME_SHIFT) | ((clock & PT_COUNTER_MASK) << PT_COUNTER_SHIFT);
        ++clock;
        page_fault_count +=1;
    }

    // put frame into TLB
    TLB[TLB_index % TLB_SIZE] = TLB_VALID_MASK | ((page << TLB_PAGE_SHIFT) & TLB_PAGE_MASK) | ((frame << TLB_FRAME_SHIFT) & TLB_FRAME_MASK);
    TLB_index = (TLB_index + 1) % TLB_SIZE;

    // clear timestamp

    // // update timestamp on PT
    // page_table[page] = page_table[page] | ((clock << PT_COUNTER_SHIFT) & PT_COUNTER_MASK);


    return page_table[page];
}



long int getFrameFromTLB(int page) {
    bool flag = false;
    
    for (int i=0; i < TLB_SIZE; i++){
        if(((TLB[i] & TLB_PAGE_MASK) >> TLB_PAGE_SHIFT) == page && TLB[i] & TLB_VALID_MASK){
            frame = TLB[i] & TLB_FRAME_MASK;
            TLB_hit_count+=1;
            // printf("frame:%d\n", frame);
            flag = true;
            break;
        }
    }
    if(!flag) //tbl miss
        frame = getFrameFromPageTable(page);

    return frame;
}


int main(int argc, char *argv[]) {
    MEM_SIZE = atoi(argv[1]);
    char *backing_store = argv[2];
    char *INPUT_ADDRESSES_FILE = argv[3];

    PAGE_TABLE_SIZE = MEM_SIZE;

    // open the address file to read
    FILE *addr_file;
    addr_file = fopen(INPUT_ADDRESSES_FILE, "r");
    char v_addr_str[20];

    // open and set backing_store
    bs = fopen(backing_store, "rb");

    // create output csv file
    FILE *outputFile;
    int size = strlen("output") + strlen(argv[1]) + strlen(".csv") +1;
    char *outputFileName = malloc(size);
    strcpy(outputFileName, "output");
    strcat(outputFileName, argv[1]);
    strcat(outputFileName, ".csv");
    outputFile = fopen(outputFileName, "w");


    while(fgets(v_addr_str, sizeof(v_addr_str), addr_file)){

    // Parse virtual address
    int v_addr = atoi(v_addr_str);
    offset = v_addr & V_PAGE_OFFSET_MASK;
    page = (v_addr & V_PAGE_NUM_MASK) >> V_PAGE_NUM_SHIFT;

    // printf("v_addr: %d, pg_num %d, offset: %d\n", v_addr, pg_num, offset);

    //first search TLB
    frame = getFrameFromTLB(page);
    // frame = getFrameFromPageTable(page);

    int phys_addr = ((frame & PT_FRAME_NUM_MASK) << 8) | offset;
    int val = phys_mem[phys_addr];
    // fprintf(outputFile, "%d,%d,%d,%04x,%04x,%04x\n", v_addr, phys_addr, val, frame,offset,phys_addr );
    fprintf(outputFile, "%d,%d,%d\n", v_addr, phys_addr, val);
    addr_count++;
    }

    // print stats
    fprintf(outputFile, "Page Faults Rate, %.2lf%%,\n", page_fault_count/addr_count * 100);
    fprintf(outputFile, "TLB Hits Rate, %.2lf%%,", TLB_hit_count/addr_count * 100);

    // for (int i=0; i < TLB_SIZE; i++){
    //     printf("%04x\n", TLB[i]);
    // }

    return 0;
}