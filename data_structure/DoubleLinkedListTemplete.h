#pragma once

/* Usage :
DoublyLinkedList<int> dll;
dll.addNode(1);
dll.addNode(2);
dll.addNode(3);
dll.addNode(4);
dll.addNode(5);

cout << dll << endl;
DoublyLinkedList<int> dll2(dll);
cout << dll2 << endl;

dll.deleteNode(3);
dll.deleteNode(1);
dll.deleteNode(5);
*/


template <class T>
struct Node
{
	Node(T data) : data(data), next(NULL), prev(NULL) {}
	T data;
	Node * next;
	Node * prev;
};

template <class T>
class CDoublyLinkedList
{
public:
	CDoublyLinkedList() : head(NULL), tail(NULL), count(0) {}
	CDoublyLinkedList(const CDoublyLinkedList<T> & DLL);
	~CDoublyLinkedList();

	void		releaseAll();

	void		addNode(T data);

	void		deleteNode(T data);	//delete specific data node
	Node<T>*	deleteNode(Node<T> *cur, bool bForward);	//delete current node

	Node<T>*	begin() { return head; }
	Node<T>*	end()	{ return tail; }

	int			getCount() { return count; }

	//template <class U>
	//friend std::ostream & operator<<(std::ostream & os, const CDoublyLinkedList<U> & dll);
private:
	Node<T>		*head;
	Node<T>		*tail;
	int			count;
};

/*
template <class U>
std::ostream & operator<<(std::ostream & os, const CDoublyLinkedList<U> & dll)
{
	Node<U> * tmp = dll.head;
	while (tmp)
	{
		os << tmp->data << " ";
		tmp = tmp->next;
	}

	return os;
}
*/
template <class T>
CDoublyLinkedList<T>::~CDoublyLinkedList()
{
	releaseAll();
}

template <class T>
void CDoublyLinkedList<T>::releaseAll()
{
	Node<T> * tmp = NULL;
	while (head)
	{ 
		tmp = head;
		head = head->next;
		delete tmp;
		count--;
	}
	head = tail = NULL;
}

template <class T>
void CDoublyLinkedList<T>::addNode(T data)
{
	Node<T> * t = new Node<T>(data);

	Node<T> * tmp = head;
	if (tmp == NULL)
	{
		head = tail = t;
	}
	else
	{
		while (tmp->next != NULL)
		{
			tmp = tmp->next;
		}

		tmp->next = t;
		t->prev = tail;

		tail = t;
	}
	count++;
}

template <class T>
void CDoublyLinkedList<T>::deleteNode(T data)
{
	Node<T> * tmp = head;
	while (tmp && tmp->data != data)
	{
		tmp = tmp->next;
	}

	if (tmp)
	{
		if (tmp->prev && tmp->next) // no change to head or tail
		{
			tmp->prev->next = tmp->next;
			tmp->next->prev = tmp->prev;
		}
		else if (tmp->prev) // change to tail
		{
			tmp->prev->next = tmp->next;
			tail = tmp->prev;
		}
		else if (tmp->next) // change to head
		{
			tmp->next->prev = tmp->prev;
			head = tmp->next;
		}
		else //when only one node exist...
		{
			//delete cur;	
			head = tail = NULL;
		}

		delete tmp;
		count--;
	}
}

template <class T>
Node<T> * CDoublyLinkedList<T>::deleteNode( Node<T> *cur, bool bForward )
{
	if (cur->prev && cur->next) // no change to head or tail
	{
		cur->prev->next = cur->next;
		cur->next->prev = cur->prev;
	}
	else if (cur->prev) // change to tail
	{
		cur->prev->next = cur->next;
		tail = cur->prev;
	}
	else if (cur->next) // change to head
	{
		cur->next->prev = cur->prev;
		head = cur->next;
	}
	else //when only one node exist...
	{
		delete cur;
		count--;
		head = tail = NULL;
		return NULL;
	}

	Node<T> *new_cur = cur->next;
	delete cur;
	count--;

	return new_cur;
}

//create a new instance with a existing DLL
template <class T>
CDoublyLinkedList<T>::CDoublyLinkedList(const CDoublyLinkedList<T> & DLL)
{
	head = tail = NULL;	//added by scpark. head and tail pointers of new instance DLL must be NULL.
	Node<T> * tmp = DLL.head;

	while (tmp)
	{
		addNode(tmp->data);
		tmp = tmp->next;
	}
}
