/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

/* -*- C++ -*- */

#pragma once

#include <cassert>
#include <climits>
#include <cstdlib>

namespace iseg {

// A Cartesian tree.
// adapted from treap code by Bobby Blumofe

template<class KEY, class VALUE>
class Treap
{
public:
	class Node
	{ // A node in the treap.
		friend class Treap;
		unsigned int m_Priority; //   The priority.
		KEY m_Key;							 //   The key.
		VALUE m_Value;					 //   The value.
		Node* m_Parent;					 //   Pointer to parent.
		Node* m_Left;						 //   Pointer to left child.
		Node* m_Right;					 //   Pointer to right child.

	public:
		// Construct node.
		Node() : m_Left(nullptr), m_Right(nullptr) {}

		Node(unsigned int priority_, KEY key_, VALUE value_, Node* parent_)
				: m_Priority(priority_), m_Key(key_), m_Value(value_), m_Parent(parent_), m_Left(nullptr), m_Right(nullptr)
		{
		}
		KEY GetKey() const { return m_Key; }
		VALUE GetValue() const { return m_Value; }
		unsigned int GetPriority() const { return m_Priority; }
		//		void setPriority(unsigned int prior1) {priority=prior1; }
	};

	// Construct an empty treap.
	Treap();

	// Destructor.
	~Treap();

	// Return value of key or 0 if not found.
	// Return a matching node (or nullptr if not found).
	Node* Lookup(KEY key) const { return DoLookup(key); }

	// Return a matching node (or nullptr if not found).
	Node* LookupGreater(KEY key) const { return DoLookupGreater(key); }

	// Set the given key to have the given value.
	void Insert(Node* n, KEY key, VALUE value, unsigned int priority);
	void Insert(KEY key, VALUE value, unsigned int priority);
	void UpdatePriority(Node* n, unsigned int priority);
	void Insert(Node* n);
	Node* GetTop() { return m_Root; }

	// Remove entry with given key.
	// Remove entry.
	Node* Remove(Node* node)
	{
#if 0
			// Search for node with given key.
			Node* node = DoLookup(key);
#endif

		// If not found, then do nothing.
		if (!node)
			return nullptr;

		// While node is not a leaf...
		while (node->m_Left || node->m_Right)
		{
			// If left child only, rotate right.
			if (!node->m_Right)
				RotateRight(node);

			// If right child only, rotate left.
			else if (!node->m_Left)
				RotateLeft(node);

			// If both children, else
			{
				if (node->m_Left->m_Priority < node->m_Right->m_Priority)
					RotateRight(node);
				else
					RotateLeft(node);
			}
		}

		// Clip off node.
		Node* parent = node->m_Parent;
		if (!parent)
		{
			assert(m_Root == node);
			m_Root = nullptr;
		}
		else
		{
			if (parent->m_Left == node)
				parent->m_Left = nullptr;
			else
				parent->m_Right = nullptr;
		}

		// Check treap properties.
		// assert (heapProperty (root, INT_MIN));
		// assert (bstProperty (root, INT_MIN, INT_MAX));

#if 0
			delete node;
			return nullptr;
#else
		// Return the removed node.

		return node;
#endif
	}

	void Print() const
	{
		ReallyPrint(m_Root);
		cout << endl;
	}

	void ReallyPrint(Node* node) const
	{
		if (node == nullptr)
			return;
		ReallyPrint(node->left);
		cout << "[" << node->key << "] ";
		ReallyPrint(node->right);
	}

private:
	Node* m_Root; // Pointer to root node of treap.

	// Disable copy and assignment.
	Treap(const Treap& treap) {}
	Treap& operator=(const Treap& treap) { return *this; }

	// Check treap properties.
	int HeapProperty(Node* node, int lbound) const;
	int BstProperty(Node* node, int lbound, int ubound) const;

	// Delete treap rooted at given node.
	void DeleteTreap(Node* node);

	// Return node with given key or nullptr if not found.
	Node* DoLookup(KEY key) const;
	Node* DoLookupGreater(KEY key) const;
	Node* LookupGeq(KEY key, Node* root) const;

	// Perform rotations.
	void RotateLeft(Node* node);
	void RotateRight(Node* node);
};

// Construct an empty treap.
template<class KEY, class VALUE>
Treap<KEY, VALUE>::Treap() : m_Root(nullptr) {}

// Destructor.
template<class KEY, class VALUE>
Treap<KEY, VALUE>::~Treap()
{
	DeleteTreap(m_Root);
}

// Delete treap rooted at given node.
template<class KEY, class VALUE>
void Treap<KEY, VALUE>::DeleteTreap(Node* node)
{
	// If empty, nothing to do.
	if (!node)
		return;

	// Delete left and right subtreaps.
	DeleteTreap(node->m_Left);
	DeleteTreap(node->m_Right);

	// Delete root node.
	delete node;
}

// Test heap property in subtreap rooted at node.
template<class KEY, class VALUE>
int Treap<KEY, VALUE>::HeapProperty(Node* node, int lbound) const
{
	// Empty treap satisfies.
	if (!node)
		return 1;

	// Check priority.
	if (node->priority < lbound)
		return 0;

	// Check left subtreap.
	if (!HeapProperty(node->left, node->priority))
		return 0;

	// Check right subtreap.
	if (!HeapProperty(node->right, node->priority))
		return 0;

	// All tests passed.
	return 1;
}

// Test bst property in subtreap rooted at node.
template<class KEY, class VALUE>
int Treap<KEY, VALUE>::BstProperty(Node* node, int lbound, int ubound) const
{
	// Empty treap satisfies.
	if (!node)
		return 1;

	// Check key in range.
	if (node->key < lbound || node->key > ubound)
		return 0;

	// Check left subtreap.
	if (!BstProperty(node->left, lbound, node->key))
		return 0;

	// Check right subtreap.
	if (!BstProperty(node->right, node->key, ubound))
		return 0;

	// All tests passed.
	return 1;
}

// Perform a left rotation.
template<class KEY, class VALUE>
void Treap<KEY, VALUE>::RotateLeft(Node* node)
{
	// Get right child.
	Node* right = node->m_Right;
	assert(right);

	// Give node right's left child.
	node->m_Right = right->m_Left;

	// Adjust parent pointers.
	if (right->m_Left)
		right->m_Left->m_Parent = node;
	right->m_Parent = node->m_Parent;

	// If node is root, change root.
	if (!node->m_Parent)
	{
		assert(m_Root == node);
		m_Root = right;
	}

	// Link node parent to right.
	else
	{
		if (node->m_Parent->m_Left == node)
			node->m_Parent->m_Left = right;
		else
			node->m_Parent->m_Right = right;
	}

	// Put node to left of right.
	right->m_Left = node;
	node->m_Parent = right;
}

// Perform a right rotation.
template<class KEY, class VALUE>
void Treap<KEY, VALUE>::RotateRight(Node* node)
{
	// Get left child.
	Node* left = node->m_Left;
	assert(left);

	// Give node left's right child.
	node->m_Left = left->m_Right;

	// Adjust parent pointers.
	if (left->m_Right)
		left->m_Right->m_Parent = node;
	left->m_Parent = node->m_Parent;

	// If node is root, change root.
	if (!node->m_Parent)
	{
		assert(m_Root == node);
		m_Root = left;
	}

	// Link node parent to left.
	else
	{
		if (node->m_Parent->m_Left == node)
			node->m_Parent->m_Left = left;
		else
			node->m_Parent->m_Right = left;
	}

	// Put node to right of left.
	left->m_Right = node;
	node->m_Parent = left;
}

// Return node with given key or 0 if not found.
template<class KEY, class VALUE>
typename Treap<KEY, VALUE>::Node* Treap<KEY, VALUE>::DoLookup(KEY key) const
{
	// Start at the root.
	Node* node = m_Root;

	// While subtreap rooted at node not empty...
	while (node)
	{
		// If found, then return value.
		if (key == node->m_Key)
			return node;

		// Otherwise, search left or right subtreap.
		else if (key < node->m_Key)
			node = node->m_Left;
		else
			node = node->m_Right;
	}

	// Return.
	return node;
}

template<class KEY, class VALUE>
typename Treap<KEY, VALUE>::Node*
		Treap<KEY, VALUE>::DoLookupGreater(KEY key) const
{
	return LookupGeq(key, m_Root);
}

// Return node with greater or equal key or 0 if not found.
template<class KEY, class VALUE>
typename Treap<KEY, VALUE>::Node* Treap<KEY, VALUE>::LookupGeq(KEY key, Node* rt) const
{
	Node* best_so_far = nullptr;

	// Start at the root.
	Node* node = rt;

	// While subtreap rooted at node not empty...
	while (node)
	{
		// If exact match found, then return value.
		if (key == node->key)
			return node;

		// Move right -- this node is too small.
		if (node->key < key)
			node = node->right;

		// Otherwise, this one's pretty good;
		// look for a better match.
		else
		{
			if ((best_so_far == nullptr) || (best_so_far->key > node->key))
				best_so_far = node;
			node = node->left;
		}
	}

	// Return.
	return best_so_far;
}

// Set the given key to have the given value.
template<class KEY, class VALUE>
void Treap<KEY, VALUE>::Insert(typename Treap<KEY, VALUE>::Node* n, KEY key, VALUE value, unsigned int priority)
{
	//  print();

	// 0 is not a valid value.
	assert(value != 0);

	// Start at the root.
	Node* parent = nullptr;
	Node* node = m_Root;

	// While subtreap rooted at node not empty...
	while (node)
	{
#if 0
			// If found, then update value and done.
			if (key == node->key) {
				node->value = value;
				return;
			}
#endif

		// Otherwise, search left or right subtreap.
		parent = node;

		if (key < node->m_Key)
			node = node->m_Left;
		else
			node = node->m_Right;
	}

	// Not found, so create new node.
	// EDB was
	// node = new Node (lrand48(), key, value, parent);
	node = new Node(priority, key, value, parent); //xxxa node = new (n) Node (priority, key, value, parent);
	n = node;
	//n->priority=priority;
	//n->key=key;
	//n->value=value;
	//n->parent=parent;
	//n->left=n->right=nullptr;
	// node = new Node (priority, key, value, parent);

	// If the treap was empty, then new node is root.
	if (!parent)
		m_Root = node;

	// Otherwise, add node as left or right child.
	else if (key < parent->m_Key)
		parent->m_Left = node;
	else
		parent->m_Right = node;

	// While heap property not satisfied...
	while (parent && parent->m_Priority > node->m_Priority)
	{
		// Perform rotation.
		if (parent->m_Left == node)
			RotateRight(parent);
		else
			RotateLeft(parent);

		// Move up.
		parent = node->m_Parent;
	}

	// print();

	// Check treap properties.
	// assert (heapProperty (root, INT_MIN));
	// assert (bstProperty (root, INT_MIN, INT_MAX));
}

// Set the given key to have the given value.
template<class KEY, class VALUE>
void Treap<KEY, VALUE>::Insert(typename Treap<KEY, VALUE>::Node* n)
{
	//  print();

	// 0 is not a valid value.
	assert(n->m_Value != 0);

	// Start at the root.
	Node* parent = nullptr;
	Node* node = m_Root;

	// While subtreap rooted at node not empty...
	while (node)
	{
		parent = node;

		if (n->m_Key < node->m_Key)
			node = node->m_Left;
		else
			node = node->m_Right;
	}

	// Not found, so create new node.
	// EDB was
	// node = new Node (lrand48(), key, value, parent);
	//node=new Node (n->priority, n->key, n->value, parent);//xxxa node = new (n) Node (n->priority, n->key, n->value, parent);
	//n=node;
	n->m_Parent = parent;
	n->m_Left = n->m_Right = nullptr;
	// node = new Node (priority, key, value, parent);

	// If the treap was empty, then new node is root.
	if (!parent)
		m_Root = n;

	// Otherwise, add node as left or right child.
	else if (n->m_Key < parent->m_Key)
		parent->m_Left = n;
	else
		parent->m_Right = n;

	// While heap property not satisfied...
	while (parent && parent->m_Priority > n->m_Priority)
	{
		// Perform rotation.
		if (parent->m_Left == n)
			RotateRight(parent);
		else
			RotateLeft(parent);

		// Move up.
		parent = n->m_Parent;
	}

	// print();

	// Check treap properties.
	// assert (heapProperty (root, INT_MIN));
	// assert (bstProperty (root, INT_MIN, INT_MAX));
}

template<class KEY, class VALUE>
void Treap<KEY, VALUE>::UpdatePriority(Node* n, unsigned int priority)
{ // this could certainly be sped up...
	Remove(n);
	n->m_Priority = priority;
	Insert(n);
}

} // namespace iseg
