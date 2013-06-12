/**
    Copyright 2009-2013 Mateusz Bysiek
    mb@mbdev.pl

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
 */

#ifndef MBDEV_LISTFUNCTIONS_H
#define MBDEV_LISTFUNCTIONS_H

#include <stdlib.h>
#include <malloc.h>
#include <time.h>

// type of data, can be safely changed to any integral, numerical data type
typedef void* data_type;
#define DATA_TYPE_FORMAT "%d"

// bidirectional list element
struct _listElem
{
    data_type val; // value
    struct _listElem* next; // next element
	 struct _listElem* prev; // previous element
};
typedef struct _listElem* listElemPtr;

struct _list
{
	struct _listElem* first;
	struct _listElem* last;
	size_t len;
};
typedef struct _list* listPtr;

// functions that performs a test run
void listTestAll();

//creation or deletion of a whole list
listPtr listCreate();
//listPtr listCreate(data_type data);
//listPtr listCreateRandom(size_t len, data_type min, data_type max);
void listFree(listPtr list);

//creation or deletion of elements
listElemPtr listElemCreate(data_type data);
void listElemFree(listElemPtr elem);

// adding elements to the list
void listElemInsertFront(listPtr list, data_type data);
void listElemInsertBefore(listPtr list, size_t index, data_type data);
void listElemInsertAfter(listPtr list, size_t index, data_type data);
void listElemInsertEnd(listPtr list, data_type data);
void listMerge(listPtr list1, listPtr list2);

// removing elements
void listElemRemoveFirst(listPtr list);
void listElemRemove(listPtr list, size_t index);
void listElemRemoveLast(listPtr list);
void listElemDetach(listPtr list, listElemPtr elem);
void listClear(listPtr list);

//properties of lists
//void listConnect(struct list**elem1,struct list**elem2,data_type n);
//struct list* findlast(struct list**head);
//struct list* findprev(struct list**head,struct list*elem);
//struct list* smallest(struct list**head);

listElemPtr listElemGetFirst(listPtr list);
listElemPtr listElemGet(listPtr list, size_t index);
listElemPtr listElemGetLast(listPtr list);
size_t listLength(listPtr list);

//rearanging of elements
void listElemMove(listElemPtr elem, listPtr from, listPtr to);
//void reverseList(struct list**head);
//void swap(struct list**head,struct list*elem1,struct list*elem2);
//void selectionSort(struct list**head);

void listPrint(listPtr list, FILE* f);

//printing elements on screen
//void printList(struct list**head);
//void printLnList(struct list**head);
//void printLen(struct list**head);
//void printLnLen(struct list**head);
//void printListAndLen(struct list**head);
//void printLnListAndLen(struct list**head);

#endif // MBDEV_LISTFUNCTIONS_H
