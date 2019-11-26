#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "tiny_sha3/sha3.h"

#define NODE_SIZE 32

struct Graph {
    FILE* db;
    int64_t log2;
    int64_t pow2;
    uint8_t* pk;
    int64_t size;
    int64_t index;
    char* fileName;
};

void printBytes(uint8_t* bytes, int len) {
    for(int i = 0; i < len; i++) {
        printf("%02x", bytes[i]);
    }
}

int64_t Log2(int64_t x) {
    int64_t r = 0;
    for(; x > 1; x >>= 1) {
        r++;
    }
    
    return r;
}

int64_t subtree(struct Graph* g, const int64_t node) {
    int64_t level = (g->log2 + 1) - Log2(node);
    return (int64_t)((1 << (uint64_t)level) - 1);
}

int64_t bfsToPost(struct Graph* g, const int64_t node) {
    if(node == 0) {
        return 0;
    }
    
    int64_t cur = node;
    int64_t res = 0;
    
    while(cur != 1) {
        if(cur % 2 == 0) {
            res -= subtree(g, cur) + 1;
        } else {
            res--;
        }
        cur /= 2;
    }
    res += 2*g->pow2 - 1;
    
    return res;
}

int64_t numXi(int64_t index) {
	return (1 << ((uint64_t)index)) * (index + 1) * index;
}

void WriteId(struct Graph* g, uint8_t* Node, const int64_t id) {
    //printf("WriteId(%ld,", id);
    //printBytes(Node, NODE_SIZE);
    //printf(")\n");
    fseek(g->db, id*NODE_SIZE, SEEK_SET);
    fwrite(Node, 1, NODE_SIZE, g->db);
}

void WriteNode(struct Graph* g, uint8_t* Node, const int64_t id) {
    const int64_t idx = bfsToPost(g, id);
    WriteId(g, Node, idx);
}

void NewNode(struct Graph* g, const int64_t id, uint8_t* hash) {
    WriteNode(g, hash, id);
}

uint8_t* GetId(struct Graph* g, const int64_t id) {
    fseek(g->db, id*NODE_SIZE, SEEK_SET);
    uint8_t* node = malloc(NODE_SIZE);
    fread(node, 1, NODE_SIZE, g->db);
    //printf("GetId(%ld) = ", id);
    //printBytes(node, NODE_SIZE);
    //printf("\n");
    return node;
}

uint8_t* GetNode(struct Graph* g, const int64_t id) {
    //printf("GetNode(%ld)\n", id);
    const int64_t idx = bfsToPost(g, id);
    return GetId(g, idx);
}

uint32_t WriteVarInt(uint8_t* buffer, int64_t val) {
    uint64_t uval = ((uint64_t)(val)) << 1;
	if (val < 0) {
		uval = ~uval;
	}
	uint32_t i = 0;
    while (uval >= 0x80) {
        buffer[i] = (uint8_t)uval | 0x80;
        uval >>= 7;
        i++;
    }
    buffer[i] = (uint8_t)uval;
    return i;
}

void ButterflyGraph(struct Graph* g, int64_t index, int64_t* count) {
    //printf("ButterflyGraph (index=%ld, count=%ld)\n", index, *count);
    if(index == 0) {
        index = 1;
    }
    
    int64_t numLevel = 2*index;
    int64_t perLevel = (int64_t)(1 << (uint64_t)index);
    int64_t begin = *count - perLevel;
    int64_t level, i;
  
    for(level = 1; level < numLevel; level++) {
        for(i = 0; i < perLevel; i++) {
            //printf("ButterflyGraph (level=%ld/%ld, index=%ld/%ld)\n", level, numLevel, i, perLevel);
    
            int64_t prev;
            int64_t shift = index - level;
            if(level > numLevel/2) {
                shift = level - numLevel/2;
            }
            if(((i>>(uint64_t)shift)&1) == 0) {
                prev = i + (1 << (uint64_t)shift);
            } else {
                prev = i - (1 << (uint64_t)shift);
            }

            uint8_t* parent0 = GetNode(g, begin+(level-1)*perLevel + prev);
            uint8_t* parent1 = GetNode(g, *count - perLevel);
            uint8_t* buf = malloc(NODE_SIZE);
            WriteVarInt(buf, *count);
            uint8_t* hashInput = malloc(NODE_SIZE * 4);
            memcpy(hashInput, g->pk, NODE_SIZE);
            memcpy(hashInput+NODE_SIZE, buf, NODE_SIZE);
            memcpy(hashInput+(NODE_SIZE*2), parent0, NODE_SIZE);
            memcpy(hashInput+(NODE_SIZE*3), parent1, NODE_SIZE);
            
            //printf("Hash input: ");
            //printBytes(hashInput, NODE_SIZE * 4);
            //printf("\n");

            uint8_t* hashOutput = malloc(NODE_SIZE);
            sha3(hashInput, NODE_SIZE*4, hashOutput, NODE_SIZE);

            NewNode(g, *count, hashOutput);
            (*count)++;

            //printf("Count is now %ld\n", *count);
        }
    }
}

void XiGraphIter(struct Graph* g, int64_t index) {
    //printf("XiGraphIter(%ld)\n", index);
    int64_t count = g->pow2;

    int8_t stackSize = 5;
    int64_t *stack = malloc(sizeof(int64_t) * stackSize);
    for(int i = 0; i < 5; i++) stack[i] = index;

    int8_t graphStackSize = 5;
    int32_t *graphStack = malloc(sizeof(int32_t) * graphStackSize);
    for(int i = 0; i < 5; i++) graphStack[i] = graphStackSize-i-1;

    int64_t i = 0;
    int64_t graph = 0;
    int64_t pow2index = 1 << ((uint64_t)index);

    //printf("stack size: %d - graph stack size: %d - pow2index : %ld\n", stackSize, graphStackSize, pow2index);

    for(i = 0; i < pow2index; i++) {
        uint8_t* buf = malloc(NODE_SIZE);
        //printf("Count: %ld\n", count);
        WriteVarInt(buf, count);
        uint8_t* hashInput = malloc(NODE_SIZE * 2);
        memcpy(hashInput, g->pk, NODE_SIZE);
        memcpy(hashInput+NODE_SIZE, buf, NODE_SIZE);
        uint8_t* hashOutput = malloc(NODE_SIZE);
        //printf("Hash input: ");
        //printBytes(hashInput, NODE_SIZE * 2);
        //printf("\n");

        sha3(hashInput, NODE_SIZE*2, hashOutput, NODE_SIZE);
        NewNode(g, count, hashOutput);
        count++;
    }

    if(index == 1) {
		ButterflyGraph(g, index, &count);
		return;
	}

    while(stackSize != 0 && graphStackSize != 0) {
        
        index = stack[stackSize-1];
        graph = graphStack[graphStackSize-1];

        stackSize--;
        if (stackSize > 0) {
            int64_t* tempStack = malloc(sizeof(int64_t) * (stackSize));
            memcpy(tempStack, stack, sizeof(int64_t) * (stackSize));
            free(stack);
            stack = tempStack;
        }

        graphStackSize--;
        if (graphStackSize > 0) {
            int32_t* tempGraphStack = malloc(sizeof(int32_t) * (graphStackSize));
            memcpy(tempGraphStack, graphStack, sizeof(int32_t) * (stackSize));
            free(graphStack);
            graphStack = tempGraphStack;
        }

        //printf("Index: %ld, Graph: %ld - stack size: %d - graph stack size: %d\n", index, graph, stackSize, graphStackSize);


        int8_t indicesSize = 5;
        int64_t *indices = malloc(sizeof(int64_t) * indicesSize);
        for(int i = 0; i < indicesSize; i++) indices[i] = index-1;

        int8_t graphsSize = 5;
        int32_t *graphs = malloc(sizeof(int32_t) * graphsSize);
        for(int i = 0; i < graphsSize; i++) graphs[i] = graphsSize-i-1;

        int64_t pow2indexInner = 1 << ((uint64_t)index);
		int64_t pow2indexInner_1 = 1 << ((uint64_t)index-1);
        
        //printf("pow2indexInner: %ld - pow2indexInner_1: %ld\n", pow2indexInner, pow2indexInner_1);

        if(graph == 0) {
            uint64_t sources = count - pow2indexInner;
            for(i = 0; i < pow2indexInner_1; i++) {
                uint8_t* parent0 = GetNode(g, sources + i);
                uint8_t* parent1 = GetNode(g, sources + i + pow2indexInner_1);
                uint8_t* buf = malloc(NODE_SIZE);
                WriteVarInt(buf, count);
                uint8_t* hashInput = malloc(NODE_SIZE * 4);
                memcpy(hashInput, g->pk, NODE_SIZE);
                memcpy(hashInput+NODE_SIZE, buf, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*2), parent0, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*3), parent1, NODE_SIZE);
                //printf("Hash input: ");
                //printBytes(hashInput, NODE_SIZE * 2);
                //printf("\n");

                uint8_t* hashOutput = malloc(NODE_SIZE);
                sha3(hashInput, NODE_SIZE*4, hashOutput, NODE_SIZE);

                NewNode(g, count, hashOutput);
                count++;
            }
        } else if (graph == 1) {
            uint64_t firstXi = count;
            for(i = 0; i < pow2indexInner_1; i++) {
                uint64_t nodeId = firstXi + i;
                uint8_t* parent = GetNode(g, firstXi - pow2indexInner_1 + i);
                uint8_t* buf = malloc(NODE_SIZE);
                WriteVarInt(buf, nodeId);
                uint8_t* hashInput = malloc(NODE_SIZE * 3);
                memcpy(hashInput, g->pk, NODE_SIZE);
                memcpy(hashInput+NODE_SIZE, buf, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*2), parent, NODE_SIZE);
                //printf("Hash input: ");
                //printBytes(hashInput, NODE_SIZE * 2);
                //printf("\n");

                uint8_t* hashOutput = malloc(NODE_SIZE);
                sha3(hashInput, NODE_SIZE*3, hashOutput, NODE_SIZE);

                NewNode(g, count, hashOutput);
                count++;
            }
        } else if (graph == 2) {
            uint64_t secondXi = count;
            for(i = 0; i < pow2indexInner_1; i++) {
                uint64_t nodeId = secondXi + i;
                uint8_t* parent = GetNode(g, secondXi - pow2indexInner_1 + i);
                uint8_t* buf = malloc(NODE_SIZE);
                WriteVarInt(buf, nodeId);
                uint8_t* hashInput = malloc(NODE_SIZE * 3);
                memcpy(hashInput, g->pk, NODE_SIZE);
                memcpy(hashInput+NODE_SIZE, buf, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*2), parent, NODE_SIZE);
                //printf("Hash input: ");
                //printBytes(hashInput, NODE_SIZE * 2);
                //printf("\n");

                uint8_t* hashOutput = malloc(NODE_SIZE);
                sha3(hashInput, NODE_SIZE*3, hashOutput, NODE_SIZE);

                NewNode(g, count, hashOutput);
                count++;
            }
        } else if (graph == 3) {
            uint64_t secondButter = count;
            for(i = 0; i < pow2indexInner_1; i++) {
                uint64_t nodeId = secondButter + i;
                uint8_t* parent = GetNode(g, secondButter - pow2indexInner_1 + i);
                uint8_t* buf = malloc(NODE_SIZE);
                WriteVarInt(buf, nodeId);
                uint8_t* hashInput = malloc(NODE_SIZE * 3);
                memcpy(hashInput, g->pk, NODE_SIZE);
                memcpy(hashInput+NODE_SIZE, buf, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*2), parent, NODE_SIZE);
                //printf("Hash input: ");
                //printBytes(hashInput, NODE_SIZE * 2);
                //printf("\n");

                uint8_t* hashOutput = malloc(NODE_SIZE);
                sha3(hashInput, NODE_SIZE*3, hashOutput, NODE_SIZE);

                NewNode(g, count, hashOutput);
                count++;
            }
        } else {
            uint64_t sinks = count;
            uint64_t sources = sinks + pow2indexInner - numXi(index);
            for(i = 0; i < pow2indexInner_1; i++) {
                uint64_t nodeId0 = sinks + i;
				uint64_t nodeId1 = sinks + i + pow2indexInner_1;
				uint8_t* parent0 = GetNode(g, sinks - pow2indexInner_1 + i);
				uint8_t* parent1_0 = GetNode(g, sources + i);
				uint8_t* parent1_1 = GetNode(g, sources + i + pow2indexInner_1);

                uint8_t* buf = malloc(NODE_SIZE);
                WriteVarInt(buf, nodeId0);
                uint8_t* hashInput = malloc(NODE_SIZE * 4);
                memcpy(hashInput, g->pk, NODE_SIZE);
                memcpy(hashInput+NODE_SIZE, buf, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*2), parent0, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*3), parent1_0, NODE_SIZE);
                //printf("Hash input 0: ");
                //printBytes(hashInput, NODE_SIZE * 2);
                //printf("\n");

                uint8_t* hashOutput0 = malloc(NODE_SIZE);
                sha3(hashInput, NODE_SIZE*4, hashOutput0, NODE_SIZE);

                WriteVarInt(buf, nodeId1);
                memcpy(hashInput, g->pk, NODE_SIZE);
                memcpy(hashInput+NODE_SIZE, buf, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*2), parent0, NODE_SIZE);
                memcpy(hashInput+(NODE_SIZE*3), parent1_1, NODE_SIZE);
                //printf("Hash input 1: ");
                //printBytes(hashInput, NODE_SIZE * 2);
                //printf("\n");

                uint8_t* hashOutput1 = malloc(NODE_SIZE);
                sha3(hashInput, NODE_SIZE*4, hashOutput1, NODE_SIZE);

                NewNode(g, nodeId0, hashOutput0);
                NewNode(g, nodeId1, hashOutput1);
				count += 2;
            }
        }

        if ((graph == 0 || graph == 3) ||
			((graph == 1 || graph == 2) && index == 2)) {
			ButterflyGraph(g, index-1, &count);
		} else if (graph == 1 || graph == 2) {
            
            int64_t* tempStack = malloc(sizeof(int64_t) * (stackSize + indicesSize));
            memcpy(tempStack, stack, stackSize * sizeof(int64_t));
            memcpy(tempStack+stackSize, indices, indicesSize * sizeof(int64_t));
            stackSize += indicesSize;
            free(stack);
            stack = tempStack;

            int32_t* tempGraphStack = malloc(sizeof(int32_t) * (graphStackSize + graphsSize));
            memcpy(tempGraphStack, graphStack, graphStackSize * sizeof(int32_t));
            memcpy(tempGraphStack+graphStackSize, graphs, graphsSize * sizeof(int32_t));
            graphStackSize += graphsSize;
            free(graphStack);
            graphStack = tempGraphStack;
		}
    }
}


struct Graph* NewGraph(int64_t index, int64_t size, int64_t pow2, int64_t log2, char* fileName, uint8_t* pk) {
    uint8_t exists = 0;
    FILE* db;
    if((db = fopen(fileName, "r")) != NULL) {
        fclose(db);
        exists = 1;
    }

    db = fopen(fileName, "wb+");
    struct Graph *g = malloc(sizeof(struct Graph));
    g->db = db;
    g->log2 = log2;
    g->pow2 = pow2;
    g->pk = pk;
    g->size = size;
    g->index = index;
    g->fileName = fileName;

    if(exists == 0) {
        XiGraphIter(g, index);
    }

    return g;
}

int main() { 

/*    // TEST Varint
    uint8_t* buf = malloc(NODE_SIZE);
    WriteVarInt(buf, 131072);
    return 0;
*/
    // TEST
    int64_t index = 16;
    int64_t size = 16777216;
    int64_t log2 = Log2(size) + 1;
    int64_t pow2 = 1 << ((uint64_t)log2);

    //printf("index: %ld - size: %ld, pow2: %ld, log2: %ld\n", index, size, pow2, log2);

    char* hashInput = "Vertcoin PoS PoC";
    uint8_t* pk = malloc(NODE_SIZE);
    sha3(hashInput, 16, pk, NODE_SIZE);

    //printf("pk:");
    //printBytes(pk, NODE_SIZE);
    //printf("\n");

    NewGraph(index, size, pow2, log2, "./verthash.dat", pk);
}
