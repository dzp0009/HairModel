#ifndef _ISOCHART_HEAP_H_INCLUDED
#define _ISOCHART_HEAP_H_INCLUDED

#include "IsochartArray.h"

#define NOT_IN_HEAP -1

class IsochartHeapable
{
private:
    int token;

public:
    IsochartHeapable() { notInHeap(); }

	int vert[2];
	float cost;
	//double newposition[3];

	//int nIndex;

    inline int isInHeap() { return token!=NOT_IN_HEAP; }
    inline void notInHeap() { token = NOT_IN_HEAP; }
    inline int getHeapPos() { return token; }
    inline void setHeapPos(int t) { token=t; }
};


class isochart_heap_node {
public:
    float import;
    IsochartHeapable *obj;

    isochart_heap_node() { obj=NULL; import=0.0; }
    isochart_heap_node(IsochartHeapable *t, float i=0.0) { obj=t; import=i; }
    isochart_heap_node(const isochart_heap_node& h) { import=h.import; obj=h.obj; }
};



class IsochartHeap : public isochart_array<isochart_heap_node> {

    //
    // The actual size of the IsochartHeap.  isochart_array::length()
    // simply returns the amount of allocated space
    int size;

    void swap(int i, int j);

    int parent(int i) { return (i-1)/2; }
    int left(int i) { return 2*i+1; }
    int right(int i) { return 2*i+2; }

    void upheap(int i);
    void downheap(int i);

public:

    IsochartHeap() { size=0; }
    IsochartHeap(int s) : isochart_array<isochart_heap_node>(s) { size=0; }


    void insert(IsochartHeapable *, float);
    void update(IsochartHeapable *, float);

    isochart_heap_node *extract();
    isochart_heap_node *top() { return size<1 ? (isochart_heap_node *)NULL : &ref(0); }
    isochart_heap_node *kill(int i);

	inline int getsize() { return size; }
};



// HEAP_H_INCLUDED
#endif
