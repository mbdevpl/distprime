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
typedef int64_t data_type;
#define DATA_TYPE_FORMAT "%lli"

struct list{ //one directional list
    data_type val; //value
    struct list *next; //next element
};

//function that performs a test run
void listFunctionsTest();

//creation or deletion of elements
struct list* createElem(data_type n);
struct list* generateRandomList(int len,data_type min,data_type max);
void insertFront(struct list**head,data_type n);
void insertAfter(struct list**head,int place,data_type n);
void listConnect(struct list**elem1,struct list**elem2,data_type n);
void insertEnd(struct list**head,data_type n);
void removeFirst(struct list**head,data_type n);
void freeList(struct list**head);

//properties of lists
struct list* findlast(struct list**head);
struct list* findprev(struct list**head,struct list*elem);
struct list* smallest(struct list**head);

//rearanging of elements
void reverseList(struct list**head);
void swap(struct list**head,struct list*elem1,struct list*elem2);
void selectionSort(struct list**head);

//printing elements on screen
void printList(struct list**head);
void printLnList(struct list**head);
void printLen(struct list**head);
void printLnLen(struct list**head);
void printListAndLen(struct list**head);
void printLnListAndLen(struct list**head);

#endif // MBDEV_LISTFUNCTIONS_H
