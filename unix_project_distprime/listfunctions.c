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

#include "listfunctions.h"

void listTestAll()
{
	//this will be the data
	listPtr list = NULL;
	//listElemPtr elem1 = NULL, elem2 = NULL;

	list = listCreate();
	//selectionSort(&head);
	//reverseList(&head);
	listElemInsertEnd(list, (data_type)1);
	listElemInsertEnd(list, (data_type)3);
	listElemInsertEnd(list, (data_type)5);
	listElemInsertEnd(list, (data_type)7);
	listElemInsertEnd(list, (data_type)9);
	listElemInsertFront(list, (data_type)0);
	listElemInsertAfter(list, 1, (data_type)2);
	listElemInsertBefore(list, 4, (data_type)4);
	listElemInsertAfter(list, 5, (data_type)6);
	listElemInsertBefore(list, 8, (data_type)8);

	//elem1 = listElemGetFirst(head);
	//elem2 = listElemIn;

	listPrint(list, (FILE*)1);

	//listConnect(&elem1,&elem2,2);
	//head=elem1;
	//elem1=head;
	//elem2=elem1->next;
	//listConnect(&elem1,&elem2,1);
	//elem2=elem1->next;
	//listConnect(&elem1,&elem2,0);
	//printLnListAndLen(&head);

	// free() for every element of the list
	listFree(list);

	//system("pause");
	//return 0;
}

//creation or deletion of elements

listPtr listCreate()
{
	listPtr list = (listPtr)malloc(sizeof(struct _list));
	if(list == 0)
		ERR("malloc");
	list->first = NULL;
	list->last = NULL;
	list->len = 0;
	return list;
}

//listPtr listCreate(data_type data)
//{
//	listPtr list = listCreate();
//	listElemInsertEnd(list, data);
//	return list;
//}

void listFree(listPtr list)
{
	if(list == NULL)
		return;

	if(list->first)
	{
		listElemPtr head, temp;
		for(head = list->first; head;)
		{
			temp = head;
			head = head->next;
			free(temp->val);
			free(temp);
			if(head == list->first)
				return;
		}
	}
	free(list);
}

// creates new element of the list
listElemPtr listElemCreate(data_type data)
{
	listElemPtr newelem=(listElemPtr)malloc(sizeof(struct _listElem));
	if(newelem == 0)
		ERR("malloc");
	newelem->val = data;
	newelem->next = NULL;
	newelem->prev = NULL;
	return newelem;
}

void listElemFree(listElemPtr elem)
{
	free(elem->val);
	free(elem);
}

//inserts new element at the front
void listElemInsertFront(listPtr list, data_type data)
{
	if(list == NULL)
		return;

	listElemPtr newElem = listElemCreate(data);

	switch(list->len)
	{
	case 0:
		list->last = newElem;
		break;
	case 1:
	default:
		newElem->next = list->first;
		list->first->prev = newElem;
		break;
	}
	list->len += 1;
	// changing the beginning of the list
	list->first = newElem;
}

void listElemInsertBefore(listPtr list, size_t index, data_type data)
{
	if(list == NULL)
		return;

	if(list->len == 0 || index == 0)
		listElemInsertFront(list, data);
	else if(index >= list->len)
		listElemInsertEnd(list, data);
	else
		listElemInsertAfter(list, index - 1, data);
}

// inserts new element after element with given index
void listElemInsertAfter(listPtr list, size_t index, data_type data)
{
	if(list == NULL)
		return;

	if(list->len == 0)
		listElemInsertFront(list, data);
	else if(index >= list->len - 1)
		listElemInsertEnd(list, data);
	else
	{
		listElemPtr elem = listElemGet(list, index);
		listElemPtr newElem = listElemCreate(data);
		newElem->next = elem->next;
		newElem->prev = elem;
		elem->next = newElem;
		newElem->next->prev = newElem;
		list->len += 1;
	}
}

void listElemInsertEnd(listPtr list, data_type data)
{
	if(list == NULL)
		return;

	listElemPtr newElem = listElemCreate(data);

	switch(list->len)
	{
	case 0:
		list->first = newElem;
		break;
	case 1:
	default:
		newElem->prev = list->last;
		list->last->next = newElem;
		break;
	}
	list->len += 1;
	// changing the end of the list
	list->last = newElem;
}

void listMerge(listPtr list1, listPtr list2)
{
#ifdef DEBUG_LISTFUNCTIONS
	printf("listMerge");
	printf("1"); listPrintErrors(list1, stdout);
	printf("2"); listPrintErrors(list2, stdout);
#endif
	if(listLength(list2) == 0)
	{
		// nothing
	}
	else if(listLength(list1) == 0)
	{
		list1->first = list2->first;
		list1->last = list2->last;
		list1->len = list2->len;
	}
	else
	{
		list1->last->next = list2->first;
		list2->first->prev = list1->last;
		list1->last = list2->last;
		list1->len += list2->len;
	}
	if(list2 != NULL)
	{
		list2->first = NULL;
		list2->last = NULL;
		list2->len = 0;
		free(list2);
	}
#ifdef DEBUG_LISTFUNCTIONS
	printf("1"); listPrintErrors(list1, stdout);
	printf("2"); listPrintErrors(list2, stdout);
	printf("\n");
#endif
}

void listElemRemoveLast(listPtr list)
{
	if(listLength(list) == 0)
		return;
	listElemPtr temp = list->last;
	if(listLength(list) == 1)
	{
		list->first = NULL;
		list->last = NULL;
	}
	else
	{
		list->last = list->last->prev;
		list->last->next = NULL;
	}
	list->len -= 1;
	free(temp->val);
	free(temp);
}

int listElemDetach(listPtr list, listElemPtr elem)
{
	if(listLength(list) == 0 || elem == NULL)
		return 0;
#ifdef DEBUG_LISTFUNCTIONS
	printf("listElemDetach");
	//listPrintStatistics(list, stdout);
	listPrintErrors(list, stdout);
#endif
	if(elem == list->last)
		list->last = list->last->prev;
	if(elem == list->first)
		list->first = list->first->next;
	if(elem->prev)
		elem->prev->next = elem->next;
	if(elem->next)
		elem->next->prev = elem->prev;
	list->len -= 1;
	elem->prev = NULL;
	elem->next = NULL;
#ifdef DEBUG_LISTFUNCTIONS
	//listPrintStatistics(list, stdout);
	listPrintErrors(list, stdout);
	printf("\n");
#endif
	return 1;
}

void listClear(listPtr list)
{
	if(list == NULL)
		return;

	if(list->first)
	{
		listElemPtr head, temp;
		for(head = list->first; head;)
		{
			temp = head;
			head = head->next;
			free(temp->val); // because data_type is void*
			free(temp);
			if(head == list->first)
				return;
		}
	}

	list->first = NULL;
	list->last = NULL;
	list->len = 0;
}

listElemPtr listElemGetFirst(listPtr list)
{
	if(list == NULL)
		return NULL;
	return list->first;
}

listElemPtr listElemGet(listPtr list, size_t index)
{
	printf("listElemGet()\n");
	if(list == NULL || index < 0 || index >= list->len)
		return NULL;
	if(index == 0)
		return list->first;
	if(index == list->len - 1)
		return list->last;

	size_t counter = 1;
	listElemPtr elem;
	for(elem = list->first->next; elem; elem=elem->next)
	{
		if(counter == index)
			break;
		++counter;
	}
	return elem;
}

listElemPtr listElemGetLast(listPtr list)
{
	if(list == NULL)
		return NULL;
	return list->last;
}

size_t listLength(listPtr list)
{
	if(list == NULL)
		return 0;
	return list->len;
}

void listElemMove(listElemPtr elem, listPtr from, listPtr to)
{
#ifdef DEBUG_LISTS
	printf("listElemMove(elem, from, to)\n");
#endif
	if(elem == NULL || from == NULL || to == NULL)
		return;

	if(!listElemDetach(from, elem))
		return;

	if(to->len > 0)
	{
		to->last->next = elem;
		elem->prev = to->last;
		to->last = elem;
	}
	else
	{
		to->first = elem;
		to->last = elem;
	}
	to->len += 1;
}

//printing list on screen

//prints the list's contents
void listPrint(listPtr list, FILE* f)
{
	if(list == NULL || list->len < 1)
		return;
	listElemPtr temp;
	for(temp=list->first;temp;)
	{
		fprintf(f, "%d", (int)temp->val); // todo DATA_TYPE_FORMAT
		if((temp = temp->next) && (temp != list->first))
			fprintf(f, ", ");
		if(temp == list->first)
			return;
	}
}

void listPrintStatistics(listPtr list, FILE* f)
{
	fprintf(f, "[list %u", (size_t)list);
	if(list != NULL)
	{
		fprintf(f, " first=%u", (size_t)list->first);
		fprintf(f, " last=%u", (size_t)list->last);
		fprintf(f, " len=%u", list->len);
	}
	fprintf(f, "]");
}

void listPrintErrors(listPtr list, FILE* f)
{
	fprintf(f, "[");
	if(list != NULL)
	{
		if(list->len == 0)
		{
			if(list->first != NULL || list->last != NULL)
				fprintf(f, "len=0 but elements exist");
		}
		else
		{
			if(list->first == NULL || list->last == NULL)
				fprintf(f, "len=%u but elements missing", list->len);
			else
			{
				size_t i = 0;
				listElemPtr e;
				for(e=list->first;e;)
				{
					if(e == list->first)
					{
						if(e->prev > 0) fprintf(f, ";first elem has prev");
					}
					else if(e->prev == NULL) fprintf(f, ";%u prev is null", i);
					if(e == list->last)
					{
						if(e->next > 0) fprintf(f, ";%u last elem has next", i);
					}
					else if(e->next == NULL) fprintf(f, ";%u next is null", i);
					e = e->next;
					if(e == list->first)
						return;
					++i;
				}
			}
		}
	}
	else
		fprintf(f, "null");
	fprintf(f, "]");
}

/*
//void listConnect(struct list**elem1,struct list**elem2,data_type n){
//    struct list*newelem;
//    newelem=createElem(n);
//    if(*elem1){
//        (*elem1)->next=newelem;
//        newelem->next=(*elem2);
//    }
//    else{
//        *elem1=newelem;
//        (*elem1)->next=(*elem2);
//    }
//}

//void insertEnd(struct list**head,data_type n){
//    //adds new element at the end of the list
//    struct list*newelem,*temp;
//    if((temp=*head)){ //nonempty list
//        newelem=createElem(n);
//        for(;temp->next;) //until next is not null
//            temp=temp->next; //go to the next elem.
//        temp->next=newelem; //next of last becomes our new elem.
//    }
//    else insertFront(head,n); //list is empty
//}

//void removeFirst(struct list**head,data_type n){
//    //removes the first occurence of value n from the list
//    struct list*temp=*head;
//    if(temp){
//        if((temp->val)==n) head=NULL; //one elem, equal to n
//        else if(temp->next){ //more then one elem
//            for(;temp->next;){
//                if((temp->next->val)==n){ //next elem. value equal to n
//                    struct list *removed=temp->next; //remember next elem
//                    temp->next=temp->next->next; //change pointer
//                    free(removed); //remove former next elem
//                    //and move to end, or more than one occur. will be remov.
//                    for(;temp->next;) temp=temp->next;
//                }else temp=temp->next;
//            }
//        } //else: nothing to remove, only one elem. and it is diffrent than n
//    } //else: nothing to remove, because the list has no elements
//}

//properties of lists

//struct list* findlast(struct list**head){
//    //finds last element of the list and returns a pointer to it
//    struct list*last;
//    if((last=*head))
//        for(;last->next;last=last->next);
//    return last;
//}

//struct list* findprev(struct list**head,struct list*elem){
//    //returns a pointer to element pointing to a given element
//    struct list*prev;
//    if((prev=*head))
//        for(;(prev->next)!=elem;prev=prev->next);
//    return prev;
//}

//struct list* smallest(struct list**head){
//    struct list*temp,*small;
//    if((temp=*head)){
//        small=temp;
//        for(;temp;temp=temp->next)
//            if((temp->val)<(small->val)) small=temp;
//        return small;
//    }
//    return temp;
//}

//struct list* generateRandomList(int len,data_type min,data_type max){
//    struct list*head=NULL,*temp;
//    int i;
//    if((len>0)&&(max>=min)){
//        head=createElem(rand()%(max-min+1)+min);
//        temp=head;
//        for(i=1;i<len;i++){
//            //newelem=;
//            temp->next=createElem(rand()%(max-min+1)+min);
//            temp=temp->next;
//        }
//    }
//    return head;
//}

//rearanging of elements

//void reverseList(struct list**head){
//    struct list*temp,*end,*preend;
//    if(*head){
//        for(end=*head;end->next;end=end->next); //where is the end
//        if((*head)->next){
//            for(preend=*head;preend->next->next;preend=preend->next);
//            end->next=preend;
//            for(;(*head)!=preend;){
//                for(temp=*head;(temp->next)!=preend;temp=temp->next);
//                preend->next=temp;
//                preend=temp;
//            }
//            (*head)->next=NULL;
//        }
//        *head=end;
//    }
//}

//void swap(struct list**head,struct list*elem1,struct list*elem2){
//    struct list*temp1,*temp2,*prev1=NULL,*prev2=NULL;
//    if((*head)&&elem1&&elem2&&(elem1!=elem2)){
//        for(temp1=*head;(temp1!=elem1)&&(temp1!=elem2);temp1=temp1->next);
//        for(temp2=temp1->next;(temp2!=elem1)&&(temp2!=elem2);temp2=temp2->next);
//        if((*head)!=temp1) prev1=findprev(head,temp1); //until next is temp1
//        prev2=findprev(head,temp2); //until next is temp2
//        if(prev1) prev1->next=temp2; //swapping
//        else *head=temp2;
//        prev2=(prev2->next=temp1)->next;
//        temp1->next=temp2->next;
//        temp2->next=prev2;
//    }
//}

//void selectionSort(struct list**head){
//    //sorts the list in ascending order, using selection sort algorithm
//    struct list*currhead,*min;
//    for(currhead=*head;currhead;currhead=currhead->next){
//        swap(head,currhead,(min=smallest(&currhead)));
//        //prevhead=(currhead=min); //setting new currhead (after swap)
//    }
//}

//void printLnList(struct list**head){
//    //prints the list's contents and newline character
//    printList(head);
//    printf("\n");
//}

//void printLen(struct list**head){
//    //prints the list's length
//    struct list*temp;
//    int counter=0;
//    for(temp=*head;temp;temp=temp->next){
//        counter++;
//        if(temp->next==*head)break;
//    }
//    if(temp)if(temp->next==*head) printf("circle with ");
//    printf("len=%i",counter);
//}

//void printLnLen(struct list**head){
//    //prints the list's length and newline character
//    printLen(head);
//    printf("\n");
//}
//void printListAndLen(struct list**head){
//    //prints the list's contents and its length
//    printList(head);
//    printf(", ");
//    printLen(head);
//}

//void printLnListAndLen(struct list**head){
//    //prints the list's contents, its length and newline character
//    printListAndLen(head);
//    printf("\n");
//}
*/
