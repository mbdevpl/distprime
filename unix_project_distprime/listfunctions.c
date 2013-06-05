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

void listFunctionsTest(){
    struct list*head=NULL,*elem1=NULL,*elem2; //this will be the data
    //srand(time(0));
    head=generateRandomList(20,0,9); //random list
    selectionSort(&head);
    reverseList(&head);
    elem2=head;
    listConnect(&elem1,&elem2,2);
    head=elem1;
    elem1=head;
    elem2=elem1->next;
    listConnect(&elem1,&elem2,1);
    elem2=elem1->next;
    listConnect(&elem1,&elem2,0);
    printLnListAndLen(&head);
    freeList(&head); //free() for every element of the list
    //system("pause");
    //return 0;
}

//creation or deletion of elements

struct list* createElem(data_type n){
    //creates new element of the list
    struct list*newelem=(struct list*)malloc(sizeof(struct list));
    newelem->val=n;
    newelem->next=NULL;
    return newelem;
}

struct list* generateRandomList(int len,data_type min,data_type max){
    struct list*head=NULL,*temp;
    int i;
    if((len>0)&&(max>=min)){
        head=createElem(rand()%(max-min+1)+min);
        temp=head;
        for(i=1;i<len;i++){
            //newelem=;
            temp->next=createElem(rand()%(max-min+1)+min);
            temp=temp->next;
        }
    }
    return head;
}

void insertFront(struct list**head,data_type n){
    //inserts new element at the front
    struct list*newelem=createElem(n);
    newelem->next=*head;
    *head=newelem; //changing the beginning of the list
}

void insertAfter(struct list**head,int place,data_type n){
    //inserts new element after element numbered 'place'
    //place = 4  ==>  after the fourth element
    //place = 1  ==>  after the first element
    //place = 0 or less  ==>  at the beginning
    struct list*newelem,*temp,*after=NULL;
    int counter=0;
    if((place>0)&&(temp=*head)){
        for(;temp->next;){
            if(counter==place-1)
                after=temp; //to remember after which elem. we insert new
            counter++;
            temp=temp->next;
        }
        if(after){ //inserting new elem. after 'after'
            newelem=createElem(n);
            temp=after->next;
            after->next=newelem;
            newelem->next=temp;
        }
        else insertEnd(head,n); //add the new elem. at the end
    }
    else insertFront(head,n); //add at the beginning (or create new list)
}

void listConnect(struct list**elem1,struct list**elem2,data_type n){
    struct list*newelem;
    newelem=createElem(n);
    if(*elem1){
        (*elem1)->next=newelem;
        newelem->next=(*elem2);
    }
    else{
        *elem1=newelem;
        (*elem1)->next=(*elem2);
    }
}

void insertEnd(struct list**head,data_type n){
    //adds new element at the end of the list
    struct list*newelem,*temp;
    if((temp=*head)){ //nonempty list
        newelem=createElem(n);
        for(;temp->next;) //until next is not null
            temp=temp->next; //go to the next elem.
        temp->next=newelem; //next of last becomes our new elem.
    }
    else insertFront(head,n); //list is empty
}

void removeFirst(struct list**head,data_type n){
    //removes the first occurence of value n from the list
    struct list*temp=*head;
    if(temp){
        if((temp->val)==n) head=NULL; //one elem, equal to n
        else if(temp->next){ //more then one elem
            for(;temp->next;){
                if((temp->next->val)==n){ //next elem. value equal to n
                    struct list *removed=temp->next; //remember next elem
                    temp->next=temp->next->next; //change pointer
                    free(removed); //remove former next elem
                    //and move to end, or more than one occur. will be remov.
                    for(;temp->next;) temp=temp->next;
                }else temp=temp->next;
            }
        } //else: nothing to remove, only one elem. and it is diffrent than n
    } //else: nothing to remove, because the list has no elements
}

void freeList(struct list**head){
    struct list*temp;
    for(temp=*head;temp;temp=temp->next){
        if(temp!=*head){
            free(*head);
            *head=temp;
        }
    }
    if(*head){
        free(*head);
        *head=NULL;
    }
}

//properties of lists

struct list* findlast(struct list**head){
    //finds last element of the list and returns a pointer to it
    struct list*last;
    if((last=*head))
        for(;last->next;last=last->next);
    return last;
}

struct list* findprev(struct list**head,struct list*elem){
    //returns a pointer to element pointing to a given element
    struct list*prev;
    if((prev=*head))
        for(;(prev->next)!=elem;prev=prev->next);
    return prev;
}
    
struct list* smallest(struct list**head){
    struct list*temp,*small;
    if((temp=*head)){
        small=temp;
        for(;temp;temp=temp->next)
            if((temp->val)<(small->val)) small=temp;
        return small;
    }
    return temp;
}

int listLength(struct list**head)
{
	if(head == NULL)
		return -1;
	struct list*temp;
	int counter=0;
	for(temp=*head;temp;temp=temp->next){
		counter++;
		if(temp->next==*head)break;
	}
	return counter;
}

//rearanging of elements

void reverseList(struct list**head){
    struct list*temp,*end,*preend;
    if(*head){
        for(end=*head;end->next;end=end->next); //where is the end
        if((*head)->next){
            for(preend=*head;preend->next->next;preend=preend->next);
            end->next=preend;
            for(;(*head)!=preend;){
                for(temp=*head;(temp->next)!=preend;temp=temp->next);
                preend->next=temp;
                preend=temp;
            }
            (*head)->next=NULL;
        }
        *head=end;
    }
}

void swap(struct list**head,struct list*elem1,struct list*elem2){
    struct list/**temp,*/*temp1,*temp2,*prev1=NULL,*prev2=NULL;
    if((*head)&&elem1&&elem2&&(elem1!=elem2)){
        for(temp1=*head;(temp1!=elem1)&&(temp1!=elem2);temp1=temp1->next);
        for(temp2=temp1->next;(temp2!=elem1)&&(temp2!=elem2);temp2=temp2->next);
        if((*head)!=temp1) prev1=findprev(head,temp1); //until next is temp1
        prev2=findprev(head,temp2); //until next is temp2
        if(prev1) prev1->next=temp2; //swapping
        else *head=temp2;
        prev2=(prev2->next=temp1)->next;
        temp1->next=temp2->next;
        temp2->next=prev2;
    }
}

void selectionSort(struct list**head){
    //sorts the list in ascending order, using selection sort algorithm
    struct list/**prevhead=NULL,*/*currhead,*min;
    for(currhead=*head;currhead;currhead=currhead->next){
        swap(head,currhead,(min=smallest(&currhead)));
        /*prevhead=(currhead=min);*/ //setting new currhead (after swap)
    }
}

//printing list on screen

void printList(struct list**head){
    //prints the list's contents
    struct list*temp;
    for(temp=*head;temp;){
        printf(DATA_TYPE_FORMAT,temp->val);
        if((temp=temp->next)&&(temp!=*head))printf(", ");
        if(temp==*head) return;
    }
}

void printLnList(struct list**head){
    //prints the list's contents and newline character
    printList(head);
    printf("\n");
}

void printLen(struct list**head){
    //prints the list's length
    struct list*temp;
    int counter=0;
    for(temp=*head;temp;temp=temp->next){
        counter++;
        if(temp->next==*head)break;
    }
    if(temp)if(temp->next==*head) printf("circle with ");
    printf("len=%i",counter);
}

void printLnLen(struct list**head){
    //prints the list's length and newline character
    printLen(head);
    printf("\n");
}
void printListAndLen(struct list**head){
    //prints the list's contents and its length
    printList(head);
    printf(", ");
    printLen(head);
}

void printLnListAndLen(struct list**head){
    //prints the list's contents, its length and newline character
    printListAndLen(head);
    printf("\n");
}
