#include "btree.h"

BTreeNode::BTreeNode(bool isLeaf) {
    leaf = isLeaf;
    numKeys = 0;
    for (int i = 0; i < 2 * BTREE_T; i++) children[i] = nullptr;
}

// ---------- creation / destruction / copying ----------

BTree::BTree() {
    root = new BTreeNode(true);
}

BTree::~BTree() {
    destroyNode(root);
}

BTree::BTree(const BTree& other) {
    root = cloneNode(other.root);
}

BTree& BTree::operator=(const BTree& other) {
    if (this != &other) {
        destroyNode(root);
        root = cloneNode(other.root);
    }
    return *this;
}

void BTree::destroyNode(BTreeNode* node) {
    if (node == nullptr) return;
    if (!node->leaf) {
        for (int i = 0; i <= node->numKeys; i++) destroyNode(node->children[i]);
    }
    delete node;
}

BTreeNode* BTree::cloneNode(BTreeNode* node) {
    BTreeNode* copy = new BTreeNode(node->leaf);
    copy->numKeys = node->numKeys;
    for (int i = 0; i < node->numKeys; i++) {
        copy->keys[i] = node->keys[i];
        copy->rowIds[i] = node->rowIds[i];
    }
    if (!node->leaf) {
        for (int i = 0; i <= node->numKeys; i++) {
            copy->children[i] = cloneNode(node->children[i]);
        }
    }
    return copy;
}

void BTree::clear() {
    destroyNode(root);
    root = new BTreeNode(true);
}

int BTree::height() {
    int h = 1;
    BTreeNode* node = root;
    while (!node->leaf) {
        node = node->children[0];
        h++;
    }
    return h;
}

// ---------- search ----------

vector<int>* BTree::find(long long key) {
    BTreeNode* node = root;
    while (node != nullptr) {
        int i = 0;
        while (i < node->numKeys && key > node->keys[i]) i++;

        if (i < node->numKeys && key == node->keys[i]) return &node->rowIds[i];
        if (node->leaf) return nullptr;
        node = node->children[i];
    }
    return nullptr;
}

// In-order walk that skips every subtree that cannot contain a key in [lo, hi]
void BTree::rangeSearchNode(BTreeNode* node, long long lo, long long hi, vector<int>& result) {
    for (int i = 0; i < node->numKeys; i++) {
        // children[i] holds keys between keys[i-1] and keys[i]
        if (!node->leaf) {
            bool childMayMatch = (lo < node->keys[i]) && (i == 0 || node->keys[i - 1] < hi);
            if (childMayMatch) rangeSearchNode(node->children[i], lo, hi, result);
        }
        if (node->keys[i] > hi) return;   // all remaining keys are even larger

        if (node->keys[i] >= lo) {
            for (int j = 0; j < node->rowIds[i].size(); j++) {
                result.push_back(node->rowIds[i][j]);
            }
        }
    }
    // the last child holds keys bigger than the last key of this node
    if (!node->leaf && node->keys[node->numKeys - 1] < hi) {
        rangeSearchNode(node->children[node->numKeys], lo, hi, result);
    }
}

void BTree::rangeSearch(long long lo, long long hi, vector<int>& result) {
    if (lo > hi) return;
    rangeSearchNode(root, lo, hi, result);
}

// ---------- insertion ----------

// Splits the full child parent->children[i] into two nodes.
// The middle key of the child moves up into the parent.
void BTree::splitChild(BTreeNode* parent, int i) {
    BTreeNode* child = parent->children[i];
    BTreeNode* right = new BTreeNode(child->leaf);

    // the upper T-1 keys of the child move to the new right node
    right->numKeys = BTREE_T - 1;
    for (int j = 0; j < BTREE_T - 1; j++) {
        right->keys[j] = child->keys[j + BTREE_T];
        right->rowIds[j] = child->rowIds[j + BTREE_T];
        child->rowIds[j + BTREE_T].clear();
    }
    if (!child->leaf) {
        for (int j = 0; j < BTREE_T; j++) {
            right->children[j] = child->children[j + BTREE_T];
            child->children[j + BTREE_T] = nullptr;
        }
    }
    child->numKeys = BTREE_T - 1;

    // make room in the parent for the new child and the middle key
    for (int j = parent->numKeys; j >= i + 1; j--) {
        parent->children[j + 1] = parent->children[j];
    }
    parent->children[i + 1] = right;

    for (int j = parent->numKeys - 1; j >= i; j--) {
        parent->keys[j + 1] = parent->keys[j];
        parent->rowIds[j + 1] = parent->rowIds[j];
    }
    parent->keys[i] = child->keys[BTREE_T - 1];
    parent->rowIds[i] = child->rowIds[BTREE_T - 1];
    child->rowIds[BTREE_T - 1].clear();
    parent->numKeys++;
}

// Inserts into a node that is guaranteed not to be full
void BTree::insertNonFull(BTreeNode* node, long long key, int rowId) {
    int i = node->numKeys - 1;

    if (node->leaf) {
        // shift bigger keys one step right, then drop the new key in
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->rowIds[i + 1] = node->rowIds[i];
            i--;
        }
        node->keys[i + 1] = key;
        node->rowIds[i + 1].clear();
        node->rowIds[i + 1].push_back(rowId);
        node->numKeys++;
    } else {
        while (i >= 0 && key < node->keys[i]) i--;
        i++;   // i is now the child where the key belongs

        if (node->children[i]->numKeys == 2 * BTREE_T - 1) {
            splitChild(node, i);
            if (key > node->keys[i]) i++;   // the middle key moved up; pick the correct half
        }
        insertNonFull(node->children[i], key, rowId);
    }
}

void BTree::insert(long long key, int rowId) {
    // key already present: just remember one more row for it
    vector<int>* existing = find(key);
    if (existing != nullptr) {
        existing->push_back(rowId);
        return;
    }

    // full root: the tree grows one level taller
    if (root->numKeys == 2 * BTREE_T - 1) {
        BTreeNode* newRoot = new BTreeNode(false);
        newRoot->children[0] = root;
        root = newRoot;
        splitChild(root, 0);
    }
    insertNonFull(root, key, rowId);
}