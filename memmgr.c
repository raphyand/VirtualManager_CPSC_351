//
//  memmgr.c
//  memmgr
//
//  Created by William McCarthy on 17/11/20.
//  Copyright © 2020 William McCarthy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256
#define boolean_TRUE 1
#define boolean_FALSE 0

//-------------------------------------------------------------------
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

unsigned foundPageInTable(unsigned int *table, unsigned query) {
     for (int i = 0; i < FRAME_SIZE; i++) {
          if (table[i] == query)
               return boolean_TRUE;
     }
     return boolean_FALSE;
}



int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  char buf[BUFLEN];
  unsigned   page, offset, physical_add, frame = 0, linesRead = 0, fault = 0, TLBHitResult = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt
  float TLBHitPercentage = 0, pageFaultPercentage;

  printf("ONLY READ FIRST 20 entries -- TODO: change to read all entries\n\n");

  // not quite correct -- should search page table before creating a new entry
      //   e.g., address # 25 from addresses.txt will fail the assertion
      // TODO:  add page table code


  //Page table should, if no page found, trigger a page_fault, then add the page and frame to the table
  //Page table should output a frame
  unsigned int pageTable[FRAME_SIZE];
  unsigned physicalMemory[FRAME_SIZE];
  //Initialize page table to empty, or NULL
  for (int i = 0; i < FRAME_SIZE; i++)
       pageTable[i] = 300;
  //Initialize physicalMemory to empty, or NULL
  // These will be the frames, and each frame will contain a physical address
  for (int i = 0; i < FRAME_SIZE; i++)
       physicalMemory[i] = NULL;

  //Initialize TLB
  //Likely to make a FIFO Strategy
  int TLB[16][2];
  for (int i = 0; i < 16; i++) {
       for (int j = 0; j < 2; j++) {
            TLB[i][j] = 300;
       }
  }
  //Once FIFO == 17/FIFO > 16, then set FIFO == 0
  int TLB_FULL = boolean_FALSE;
  int FIFO = 0;
 


  while (linesRead < 1000) { // frames < 1000

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt

    page   = getpage(  logic_add);
    offset = getoffset(logic_add);
    //physical_add = frame++ * FRAME_SIZE + offset;

    int TLBFlag = boolean_FALSE;
    int TLBLocation = 0;

    // Check TLB for Page ------------------------------------------------------------------------------
    for (int i = 0; i < 16; i++) {
         if (TLB[i][0] == page) {
              TLBFlag = boolean_TRUE; 
              TLBLocation = i;
         }
    }
    // If page is found in TLB, then grab frame number and add offset
    // Should NOT add to frames, just like checking the page table
    if (TLBFlag == boolean_TRUE) {
         //physical_add = frame++ * FRAME_SIZE + offset;
         printf("FOUND IN TLB.\n");
         physical_add = TLB[TLBLocation][1] * FRAME_SIZE + offset;
         linesRead++;
         TLBHitResult++;
    }
    // Check Page Table ------------------------------------------------------------------------
    else if (pageTable[page] != 300) {
         printf("DUPLICATE FOUND.\n");
         physical_add = pageTable[page] * FRAME_SIZE + offset;

         printf("Value: %d\n", physicalMemory[pageTable[page]]);
         linesRead++;
         //frame++;
    }
    else { //Page Fault Handling; Will add to PageTable---------------------------------------
         printf("PAGE_FAULT: Adding to PAGE_TABLE\n");
         
         // todo: read BINARY_STORE and confirm value matches read value from correct.txt
         FILE* fBin = fopen("BACKING_STORE.bin", "rb");
         fseek(fBin, logic_add, SEEK_SET);
         char sByte;
         fread(&sByte, sizeof(char), 1, fBin);
         // -----------------------------------------------------------------------------------

         // Update Page Table -----------------------------------------------------------------
         pageTable[page] = frame;
         physical_add = frame * FRAME_SIZE + offset; //frame++
         linesRead++;
         int val = (int)(sByte);
         physicalMemory[frame] = val;
         //------------------------------------------------------------------------------------

         // Add Items to TLB - REMINDER, this is for initial stuff because of PAGE FAULTS------
         // Opportunities to add to TLB later, to be implemented above, before Page Table Check
         if (FIFO > 15)
              TLB_FULL = boolean_TRUE;
         if (TLB_FULL == boolean_FALSE) {
              
              TLB[FIFO][0] = page;
              TLB[FIFO][1] = frame;
              printf("Page: %d ----- Frame: %d\n", TLB[FIFO][0], TLB[FIFO][1]);
              FIFO++;
         }
         //------------------------------------------------------------------------------------
         fault++;
         frame++;
         pageFaultPercentage = (float)fault / linesRead;
         TLBHitPercentage = (float)TLBHitResult / linesRead;

         printf("Value: %u\n", val);

         fclose(fBin);
         assert(val == value);

    }


    if (!(physical_add == phys_add))
         printf("ERROR!\n");
    assert(physical_add == phys_add);
    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed  -->value: %5u\n", logic_add, page, offset, physical_add, value);
    if (frame % 5 == 0) { printf("\n"); }
  }

  printf("\nTotal Frames: %d\n", frame);
  printf("Page Fault Percentage: %f\n", pageFaultPercentage);
  printf("TLB Hit Results: %d\n", TLBHitResult);
  printf("TLB Hit Percentage: %f\n", TLBHitPercentage);


  fclose(fcorr);
  fclose(fadd);
  
  printf("ALL logical ---> physical assertions PASSED!\n");
  printf("!!! This doesn't work passed entry 24 in correct.txt, because of a duplicate page table entry\n");
  printf("--- you have to implement the PTE and TLB part of this code\n");

//  printf("NOT CORRECT -- ONLY READ FIRST 20 ENTRIES... TODO: MAKE IT READ ALL ENTRIES\n");

  printf("\n\t\t...done.\n");
  return 0;
}
