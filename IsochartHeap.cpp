#include "IsochartHeap.h"

using namespace std;

void IsochartHeap::swap(int i,int j)
{
    isochart_heap_node tmp = ref(i);

    ref(i) = ref(j);
    ref(j) = tmp;

    ref(i).obj->setHeapPos(i);
    ref(j).obj->setHeapPos(j);
}

void IsochartHeap::upheap(int i)
{
    if( i==0 ) return;

    if( ref(i).import > ref(parent(i)).import ) {
        swap(i,parent(i));
        upheap(parent(i));
    }
}

void IsochartHeap::downheap(int i)
{
    if (i>=size) return;        // perhaps just extracted the last

    int largest = i,
        l = left(i),
        r = right(i);

    if( l<size && ref(l).import > ref(largest).import ) largest = l;
    if( r<size && ref(r).import > ref(largest).import ) largest = r;

    if( largest != i ) {
        swap(i,largest);
        downheap(largest);
    }
}

void IsochartHeap::insert(IsochartHeapable *t,float v)
{
    if( size == maxLength() )
    {
	resize(2*size);
    }

    int i = size++;

    ref(i).obj = t;
    ref(i).import = v;

    ref(i).obj->setHeapPos(i);

    upheap(i);
}

void IsochartHeap::update(IsochartHeapable *t,float v)
{
    int i = t->getHeapPos();

    if( i >= size )
    {
	return;
    }
    else if( i == NOT_IN_HEAP )
    {
	return;
    }

    double old=ref(i).import;
    ref(i).import = v;

    if( v<old )
        downheap(i);
    else
        upheap(i);
}

isochart_heap_node *IsochartHeap::extract()
{
    if( size<1 ) return 0;

    swap(0,size-1);
    size--;

    downheap(0);

    ref(size).obj->notInHeap();

    return &ref(size);
}

isochart_heap_node *IsochartHeap::kill(int i)
{
	if (i<0)
		return NULL;

    swap(i, size-1);
    size--;
    ref(size).obj->notInHeap();

    if( ref(i).import < ref(size).import )
	downheap(i);
    else
	upheap(i);


    return &ref(size);
}