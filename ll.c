#include "traceReplay.h"

struct Node* ll_insert(void **head, void *data, int n)
{
	struct Node *new_node = malloc(struct Node);
	struct Node *next, *temp, *prev;
	int cnt = 0;

	if (n < -1)
		return NULL;

	if (new_node == NULL)
		return NULL;

	new_node->data = data;
	new_node->next = NULL;

	if (*head == NULL) {
		*head = new_node;
		return new_node;
	}

	if (n == -1) 
	{
		temp = (*head);
		while (temp->next != NULL) {
			temp = temp->next;
		}
		temp->next = new_node;
		return new_node;
	}
	else if (n == 0)
	{
		temp = (*head);
		(*head) = new_node;
		new_node->next = temp;
		return new_node;
	}
	return new_node;
/*
	temp = (*head);
	prev = NULL;
	{
		prev = temp;
		temp = temp->next;
		cnt++;
	} while ((cnt != n) && (temp->next != NULL));

	prev->next = new_node;
	new_node->next = temp;

	return new_node;
	*/
}

struct Node* ll_insert_priority(void **head, void *data, int (fn)(void* data1, void* data2))
{
	struct Node *new_node = malloc(struct Node);
	struct Node *next, *temp, *prev;
	int cnt = 0;

	if (new_node == NULL)
		return NULL;

	new_node->data = data;
	new_node->next = NULL;

	if (*head == NULL) {
		*head = new_node;
		return new_node;
	}
	
	prev = NULL;
	temp = *head;
	do {
		int result = fn(data, temp->data);
		if (result < 0)
			break;
		prev = temp;
		temp = temp->next;
	} while(temp != NULL);

	if (prev == NULL)
	{	
		*head = new_node;
		new_node->next = temp;
	} else {
		prev->next = new_node;
		new_node->next = temp;
	}
	return new_node;
}

void* ll_remove(void **head, int n)
{
	struct Node *prev, *temp;
	void* data;

	if (*head == NULL)
		return NULL;
	
	if (n < -1)
		return NULL;

	if ((*head)->next == NULL)
	{
		temp = (*head);
		(*head) = NULL;
		return temp;
	}

	if (n == -1) 
	{
		prev = NULL;
		temp = (*head);
		while (temp->next != NULL) {
			prev = temp;
			temp = temp->next;
		}
		if (prev == NULL)
			(*head) = temp->next;
		else {
			prev->next = temp->next;
		}
		return temp;
	}

	prev = NULL;
	temp = (*head);
	cnt = 0;
	do {
		if (cnt == n)
			break;
		prev = temp;
		temp = temp->next;
		cnt++;
	} while (temp->next != NULL);

	if (prev == NULL)
		(*head) = temp->next;
	else {
		prev->next = temp->next;
	}
	return temp;

out:
	data = temp;
	free(temp);
}


int ll_size(void *head)
{
	int ret = 0;
	struct Node * temp;
	temp = head;
	while (temp != NULL) {
		ret++;
		temp = temp->next;
	}
	return ret;
}


