#include "BuddyAllocator.h"
#include <iostream>
#include <math.h>
using namespace std;

BuddyAllocator::BuddyAllocator (int _basic_block_size, int _total_memory_length){
  // this is used to make sure we scale up memory in case 127 is given we scale up to 128
  total_memory_size = ceil(pow(2,ceil(log2(_total_memory_length))));
  basic_block_size = ceil(pow(2,ceil(log2(_basic_block_size))));

  start = new char[total_memory_size];
  int l = log2(total_memory_size / basic_block_size);

  for(int i = 0; i < l;i++)
  {
    FreeList.push_back(LinkedList());
  }

  FreeList.push_back(LinkedList((BlockHeader*)start));
  BlockHeader* h = new(start) BlockHeader(total_memory_size);

}


BuddyAllocator::~BuddyAllocator ()
{
	delete[] start;
}

BlockHeader* BuddyAllocator::split(BlockHeader* b){
  int bs = b->block_size;
  b->block_size /= 2; //half
  b->next = nullptr;
  
  BlockHeader* sh =(BlockHeader*) ((char*)b + b->block_size);
  sh->next = nullptr;

  sh->block_size = b->block_size;

  return sh;
}


char* BuddyAllocator::alloc(int _length) 
{
  /* This preliminary implementation simply hands the call over the 
     the C standard library! 
     Of course this needs to be replaced by your implementation.
  */
  int x = _length + sizeof(BlockHeader); //find a block of the size
  int index = ceil( log2(ceil(((double)x / basic_block_size))));//compute the index into that free list
  int blockSizeReturn = (1 << index) *  basic_block_size;

  if(index >= FreeList.size()) 
  {
    return nullptr;
  }

  if(FreeList[index].head != nullptr)
  { //you found the block you are looking for
      BlockHeader* b = FreeList[index].remove();
      b->isFree = 0;
      return (char*) (b + 1); // cast char and increase

  }

  int indexCorrect = index;
  for(;index < FreeList.size(); index++){
      if(FreeList[index].head){
        break;
      }

  }

  if(index >= FreeList.size()){ //last index, no bigger block found
      return nullptr;
  }

  while(index > indexCorrect){
      BlockHeader* b = FreeList[index].remove(); 
      BlockHeader* shb  = split(b); // bigger block found
      --index;
      FreeList[index].insert(b);

      FreeList[index].insert(shb);

  }

  BlockHeader* block = FreeList[index].remove();

  block->isFree = 0;

  return (char *) (block+1);

}

BlockHeader* BuddyAllocator::getbuddy(BlockHeader* b)
{
  return (BlockHeader*)((int)(((char*)b - start) ^ b->block_size) + start);
}

BlockHeader* BuddyAllocator::merge(BlockHeader* sa, BlockHeader* ba)
{
  if(sa > ba){
    swap(sa,ba);
  }
  sa->block_size *=2;
  return sa;

}

int BuddyAllocator::free(char* _a) {


  BlockHeader* b = (BlockHeader*)(_a - sizeof(BlockHeader));
  while(true)
  {
    int size = b->block_size; 
    b->isFree = 1;
    int index = getIndex(size);
    if(index == FreeList.size() -1){
      FreeList[index].insert(b);
      b->next = nullptr;

      break;
    }

    BlockHeader* buddy = getbuddy(b);
    
    if(buddy->isFree){
      FreeList[index].remove();
      b = merge(b, buddy);
    }
    else
    {
      FreeList[index].insert(b);
      b->next=nullptr;

      break;
    }
  }

  return 0;
}


void BuddyAllocator::printlist (){
  cout << "Printing the Freelist in the format \"[index] (block size) : # of blocks\"" << endl;
  for (int i=0; i<FreeList.size(); i++){
    cout << "[" << i <<"] (" << ((1<<i) * basic_block_size) << ") : ";  // 2^i * bbs
    int count = 0;
    BlockHeader* b = FreeList [i].head;

    while (b){
      count ++;

      if (b->block_size != (1<<i) * basic_block_size){
        cout << b->block_size;
        cerr << "ERROR:: Block is in a wrong list" << endl;
        cout << count << endl;

        exit (-1);
      }
      b = b->next;
    }
    cout << count << endl;
  }
}

