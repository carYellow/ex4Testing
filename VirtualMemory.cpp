#include <clocale>
#include "VirtualMemory.h"
#include "PhysicalMemory.h"

#include <bitset> //TOD: Get rid of this
#include <iostream>

int getSeqOfBits(int value, int numOfBits, int startBit);

int dfs(int frame, int *maxFrame, int myPointer, int curDepth, uint64_t dontDestroyFrame);

bool isFrameAllZero(int frameIndex);

int extractBits(int number, int k, int p);

typedef struct {
    uint64_t papaOfFrameToEvict; // ROW in RAM, YOOOOO WERE SO COOL!!!
    uint64_t pageIndexToEvict;
    uint64_t frameIndexToEvict;


    double maxWeightOfPage;


} EvictedFrame;

int getFrame(uint64_t forbiddenFrame);

int dfsEvict(EvictedFrame *ev, uint64_t frame, int curDepth, double curWeight, int curPapa, uint64_t p_i);

double weightOfNode(int frame);

void clearTable(uint64_t frameIndex) {
    for (uint64_t i = 0; i < PAGE_SIZE; ++i) {
        PMwrite(frameIndex * PAGE_SIZE + i, 0);
    }
}

void VMinitialize() {
    clearTable(0);
}


int VMread(uint64_t virtualAddress, word_t *value) {
    if(virtualAddress >= VIRTUAL_MEMORY_SIZE){
        return 0;
    }
//    int depth = CEIL((log2(VIRTUAL_MEMORY_SIZE) - log2(PAGE_SIZE)) / log2(PAGE_SIZE));
    int size = VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH;
    uint64_t page = virtualAddress >> OFFSET_WIDTH;

    uint64_t offset = getSeqOfBits(virtualAddress, OFFSET_WIDTH, 0);
    uint64_t p_i;
    word_t frameIndex;
    uint64_t prev = 0;
    uint64_t baseAddr = 0;
    word_t nextAddr = 0;
    for (int i = 0; i < TABLES_DEPTH; i++) {
        //TODO: run this from tables_depth to 0
        //TODO make sure that p_i is correct
        //Note:p_i is wrong it always returns the same value witch is the same as virtualAddress
        //Seems to be due to the start bit always staying zero, perhaps it should jump by the number of bits each time
        //That would make some sence

        p_i = getSeqOfBits(virtualAddress, size / TABLES_DEPTH, (size - (i * size / TABLES_DEPTH)));
//        p_i = getSeqOfBits(virtualAddress, size / TABLES_DEPTH, (size - (i * size / TABLES_DEPTH) - (size / TABLES_DEPTH)));
        printf("p_i is: %lu\n", p_i);

        PMread((baseAddr * PAGE_SIZE) + p_i, &nextAddr);
        if (nextAddr == 0) {
            //Find an unused frame or evict a page from some frame. denote frame as f

            frameIndex = getFrame(baseAddr);


            //Restore the page we are looking for to frame f2 (only necessary in actual pages)
            //May
            if (i + 1 == TABLES_DEPTH) {
                PMrestore(frameIndex, page);

            } else {
                //Write 0 in all of its contents (only necessary in tables)
                clearTable(frameIndex);
            }
//                clearTable(frameIndex);


            PMwrite((baseAddr * PAGE_SIZE) + p_i, frameIndex);

            prev = baseAddr;
            baseAddr = frameIndex;
        } else {
            prev = baseAddr;
            baseAddr = nextAddr;
        }
    }
//    PMrestore(frameIndex, page);
    PMread((baseAddr * PAGE_SIZE) + offset, value);
    //TODO check if the address is valid, cause there is an option to return fail
    return 1;
}
//        p_i = extractBits(virtualAddress, numOfBits, i * numOfBits);

int VMwrite(uint64_t virtualAddress, word_t value) {
    if(virtualAddress >= VIRTUAL_MEMORY_SIZE){
        return 0;
    }
//    int depth = CEIL((log2(VIRTUAL_MEMORY_SIZE) - log2(PAGE_SIZE)) / log2(PAGE_SIZE));
    int size = VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH;
    uint64_t page = virtualAddress >> OFFSET_WIDTH;

    uint64_t offset = getSeqOfBits(virtualAddress, OFFSET_WIDTH, 0);
    uint64_t p_i;
    word_t frameIndex;
    uint64_t prev = 0;
    uint64_t baseAddr = 0;
    word_t nextAddr = 0;
    for (int i = 0; i < TABLES_DEPTH; i++) {
        //TODO: run this from tables_depth to 0
        //TODO make sure that p_i is correct
        //Note:p_i is wrong it always returns the same value witch is the same as virtualAddress
        //Seems to be due to the start bit always staying zero, perhaps it should jump by the number of bits each time
        //That would make some sence
        int numOfBits = (64 - OFFSET_WIDTH) / TABLES_DEPTH,
                p_i = getSeqOfBits(virtualAddress, size / TABLES_DEPTH, (size - (i * size / TABLES_DEPTH)));
//        p_i = getSeqOfBits(virtualAddress, size / TABLES_DEPTH, (size - (i * size / TABLES_DEPTH) - (size / TABLES_DEPTH)));
        printf("p_i is: %lu\n", p_i);

        PMread((baseAddr * PAGE_SIZE) + p_i, &nextAddr);
        if (nextAddr == 0) {
            //Find an unused frame or evict a page from some frame. denote frame as f

            frameIndex = getFrame(baseAddr);


            //Restore the page we are looking for to frame f2 (only necessary in actual pages)
            //May
            if (i + 1 == TABLES_DEPTH) {
                PMrestore(frameIndex, page);

            } else {
                //Write 0 in all of its contents (only necessary in tables)
                clearTable(frameIndex);
            }
//                clearTable(frameIndex);


            PMwrite((baseAddr * PAGE_SIZE) + p_i, frameIndex);

            prev = baseAddr;
            baseAddr = frameIndex;
        } else {
            prev = baseAddr;
            baseAddr = nextAddr;
        }
    }
//    PMrestore(frameIndex, page);
    PMwrite((baseAddr * PAGE_SIZE) + offset, value);
    //TODO check if the address is valid, cause there is an option to return fail

    return 1;
}

int getFrame(uint64_t forbiddenFrame) {
    int maxFrame = 0;
    int frameIndex = dfs(0, &maxFrame, NULL, 0, forbiddenFrame);
    if (frameIndex != -1) {

        return frameIndex;
    } else if (maxFrame + 1 < NUM_FRAMES) {
        return maxFrame + 1;
    }
        // Evict frame
    else {
        auto ev = EvictedFrame();
        dfsEvict(&ev, 0, 0, 0, 0, 0);
        PMwrite(ev.papaOfFrameToEvict, 0);
        PMevict(ev.frameIndexToEvict, ev.pageIndexToEvict);
        return ev.frameIndexToEvict;

    }

}

int dfsEvict(EvictedFrame *ev, uint64_t frame, int curDepth, double curWeight, int curPapa, uint64_t p_i) {

    curWeight += weightOfNode(frame);
    if (curDepth >= TABLES_DEPTH) { //TODO: Check case theat the weights are equel
        //Check if the wieght is higher
        curWeight+=weightOfNode(p_i);
        if ((curWeight > ev->maxWeightOfPage) || ((curWeight == ev->maxWeightOfPage) && (p_i < ev->pageIndexToEvict))) {
            ev->maxWeightOfPage = curWeight;
            ev->papaOfFrameToEvict = curPapa;
            ev->frameIndexToEvict = frame;
            ev->pageIndexToEvict = p_i;
        }

        return 0;
    }
    for (uint64_t rowNum = 0; rowNum < PAGE_SIZE; ++rowNum) {
        int rowInRam = (frame * PAGE_SIZE) + rowNum;
        int nextFrame;
        PMread(rowInRam, &nextFrame);
        if (nextFrame == 0) {
            continue;
        }
        int shift = (VIRTUAL_ADDRESS_WIDTH - OFFSET_WIDTH)/TABLES_DEPTH; //TODO:
        p_i = (p_i << shift) + rowNum;
        dfsEvict(ev, nextFrame, curDepth+1, curWeight, rowInRam, p_i);
    }

}

double weightOfNode(int frame) {
    if (frame % 2 == 0) {
        return WEIGHT_EVEN;
    }
    return WEIGHT_ODD;
}


/**
 *
 * @return
 */
int dfs(int frame, int *maxFrame, int myPointer, int curDepth, uint64_t dontDestroyFrame) {
    if (frame > *maxFrame) {
        *maxFrame = frame;
    }
    if (curDepth >= TABLES_DEPTH) {
        return -1;
    }

    if (frame >= NUM_FRAMES) {
        return -1;
    }
    if (frame != dontDestroyFrame && isFrameAllZero(frame)) {
//    if (isFrameAllZero(frame)) {
        //papa of frame = 0
        PMwrite(myPointer, 0);
        return frame;
    }

    for (int rowNum = 0; rowNum < PAGE_SIZE; ++rowNum) {
        int rowInRam = (frame * PAGE_SIZE) + rowNum;
        int nextFrame;
        PMread(rowInRam, &nextFrame);
        if (nextFrame == 0) {
            continue;
        }

        int res = dfs(nextFrame, maxFrame, rowInRam, curDepth + 1, dontDestroyFrame);
        if (res != -1) {
            return res;
        }
    }
    return -1;


}

bool isFrameAllZero(int frameIndex) {
    if (frameIndex == 0) {
        return false;
    }
    for (int rowNum = 0; rowNum < PAGE_SIZE; ++rowNum) {
        int value;
        PMread((frameIndex * PAGE_SIZE) + rowNum, &value);
        if (value != 0) {
            return false;
        }
    }
    return true;
}

int getSeqOfBits(int value, int numOfBits, int startBit) {
    unsigned mask;
    mask = ((1 << numOfBits) - 1) << startBit;


    return (value & mask) >> startBit;
}

// Function to extract k bits from p position
// and returns the extracted value as integer
int extractBits(int number, int k, int p) {
    return (((1 << k) - 1) & (number >> (p - 1)));
}