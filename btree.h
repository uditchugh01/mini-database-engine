#ifndef BTREE_H
#define BTREE_H

#include <vector>
using namespace std;

// Minimum degree of the tree. Every node holds at most 2*T-1 keys and (except the root)
// at least T-1 keys. A bigger T means fewer levels, so fewer steps to find a key.
const int BTREE_T = 16;

struct BTreeNode {
    bool leaf;
    int numKeys;
    long long keys[2 * BTREE_T - 1];
    vector<int> rowIds[2 * BTREE_T - 1];   // rows holding each key (one key can appear in many rows)
    BTreeNode* children[2 * BTREE_T];

    BTreeNode(bool isLeaf);
};

// A B-tree that maps an INT key to the list of row positions having that key.
class BTree {
public:
    BTree();
    BTree(const BTree& other);              // deep copy (the tree owns raw pointers)
    BTree& operator=(const BTree& other);
    ~BTree();

    void insert(long long key, int rowId);

    // Returns the row list of the key, or nullptr if the key is not in the tree
    vector<int>* find(long long key);

    // Appends the row positions of every key in [lo, hi] to 'result'
    void rangeSearch(long long lo, long long hi, vector<int>& result);

    void clear();      // removes everything
    int height();      // number of levels (1 means the root is a leaf)

private:
    BTreeNode* root;

    void splitChild(BTreeNode* parent, int i);
    void insertNonFull(BTreeNode* node, long long key, int rowId);
    void rangeSearchNode(BTreeNode* node, long long lo, long long hi, vector<int>& result);
    BTreeNode* cloneNode(BTreeNode* node);
    void destroyNode(BTreeNode* node);
};

#endif
