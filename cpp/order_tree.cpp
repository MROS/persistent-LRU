#include "order_tree.hpp"
#include <stdio.h>
#include <vector>
using namespace std;

Node *_createFullTree(int height, int index, vector<int> &values) {
    if (height < 0) {
        throw "樹高度不得爲負";
    } else if (height == 0) {
        int value = values.size() > index ? values[index] : -1;
        return new Node(index, value);
    } else {
        auto root = new Node();
        root->children[0] = _createFullTree(height - 1, index * 2, values);
        root->children[1] = _createFullTree(height - 1, index * 2 + 1, values);
        return root;
    }
}

Node *createFullTree(int height, vector<int> &values) {
    return _createFullTree(height, 0, values);
}

OrderTree::OrderTree(int height) {
    this->height = height;
    this->cursor = 0;
    this->root = new Node();
}

OrderTree::OrderTree(int height, vector<int> &values) {
    this->height = height;
    this->cursor = values.size();
    this->root = createFullTree(height, values);
}

void _show(Node *node, int height) {
    if (node == nullptr) {
        for (int i = 0; i < (1 << height); i++) {
            printf("(空, 空) ");
        }
    } else if (height > 0) {
        // 爲內部節點
        _show(node->children[0], height - 1);
        _show(node->children[1], height - 1);
    } else {
        // 爲葉子節點
        printf("(%d, %d) ", node->key, node->value);
    }
}

void OrderTree::show() {
    _show(this->root, this->height);
    printf("\n");
}

std::pair<OrderTree*, Node*> OrderTree::add(int value) {
    Node *old_pointer = this->root;     // 原樹的指標
    Node *new_pointer = new Node();     // 正在創建的新樹的指標
    Node *new_root = new_pointer;
    for (int h = this->height - 1; h >= 0; h--) {
        int br = this->cursor & (1 << h) ? 1 : 0;
        if (old_pointer != nullptr) {
            new_pointer->children[!br] = old_pointer->children[!br];
            old_pointer = old_pointer->children[br];
        }
        new_pointer->children[br] = new Node();
        new_pointer = new_pointer->children[br];
    }
    new_pointer->index = this->cursor;
    new_pointer->value = value;
    OrderTree *new_tree = this->new_tree();
    new_tree->root = new_root;
    new_tree->cursor++;
    std::pair<OrderTree*, Node*> ret = {new_tree, new_pointer};
    return ret;
}

std::pair<OrderTree*, Node*> OrderTree::change_value(Node *node, int value) {
    Node *old_pointer = this->root;     // 原樹的指標
    Node *new_pointer = new Node();     // 正在創建的新樹的指標
    Node *new_root = new_pointer;
    for (int h = this->height - 1; h >= 0; h--) {
        int br = node->index & (1 << h) ? 1 : 0;
        new_pointer->children[!br] = old_pointer->children[!br];
        old_pointer = old_pointer->children[br];
        new_pointer->children[br] = new Node();
        new_pointer = new_pointer->children[br];
    }
    new_pointer->value = value;
    OrderTree *new_tree = this->new_tree();
    new_tree->root = new_root;
    std::pair<OrderTree*, Node*> ret = {new_tree, new_pointer};
    return ret;
}

// to_head(A) 做兩件事情
// 1. 先將 A 所在位置設爲空
// 2. 在 cursor 位置填入 A
//
// 也就是說，我們必須複製通往 node->index 的路徑，以及通往 this->cursor 的路徑
std::pair<OrderTree*, Node*> OrderTree::to_head(Node *node) {
    Node *old_pointer = this->root;     // 原樹的指標
    Node *pointer = new Node();     // 正在創建的新樹的指標
    Node *new_root = pointer;
    // 一直到 node->index 跟 this->cursor 的最低共同祖先爲止
    // 目標都是一樣的，都是複製這條路徑
    int common_h;
    for (common_h = this->height - 1;
         common_h >= 0 && (node->index & (1 << common_h)) == (this->cursor & (1 << common_h));
         common_h--)
    {
        int br = this->cursor & (1 << common_h) ? 1 : 0;
        pointer->children[!br] = old_pointer->children[!br];
        old_pointer = old_pointer->children[br];
        pointer->children[br] = new Node();
        pointer = pointer->children[br];
    }
    printf("node->index: %d, this->cursor: %d\n", node->index, this->cursor);
    printf("this->height: %d\n", this->height);
    printf("common_h: %d\n", common_h);

    // 處理 node->index
    // 先觀察何處開始爲孤枝
    int lone_h = 0; // 在 lone_h 高度時沒有分叉，且一路向下也沒有任何分叉
    Node *p = old_pointer->children[0]; // 因爲 cursor 較新， node 必定在 cursor 之左，走 children[0] 分支
    for (int h = common_h - 1; h >= 0; h--) {
        int br = node->index & (1 << h) ? 1 : 0;
        if (p->children[!br] == nullptr) {
            lone_h = lone_h < 0 ? h + 1 : lone_h;
            printf("孤枝高度可能爲 %d\n", lone_h);
        } else {
            lone_h = 0;
        }
        p = p->children[br];
    }
    printf("孤枝高度爲 %d\n", lone_h);

    // 僅複製到孤枝分叉處

    // 若最低共同祖先一往左就是孤枝，整條設爲 nullptr
    Node *index_pointer = pointer;
    p = old_pointer;
    int h;
    for (h = common_h; h > lone_h; h--) {
        int br = node->index & (1 << h) ? 1 : 0;
        index_pointer->children[!br] = p->children[!br];
        index_pointer->children[br] = new Node();
        index_pointer = index_pointer->children[br];
        p = p->children[br];
    }
    // 孤枝處設爲 nullptr
    int br = node->index & (1 << h) ? 1 : 0;
    index_pointer->children[!br] = p->children[!br];
    index_pointer->children[br] = nullptr;

    // 處理 this->cursor
    pointer->children[1] = new Node();
    Node *cursor_pointer = pointer->children[1];
    p = old_pointer->children[1];
    for (h = common_h - 1; h >= 0 && p != nullptr; h--) {
        printf("cursor height: %d\n", h);
        int br = this->cursor & (1 << h) ? 1 : 0;
        cursor_pointer->children[!br] = p->children[!br];
        cursor_pointer->children[br] = new Node();
        cursor_pointer = cursor_pointer->children[br];
        p = p->children[br];
    }
    // 原樹中已經沒有同路徑的分支了
    for (; h >= 0; h--) {
        printf("走自己的路 height: %d\n", h);
        int br = this->cursor & (1 << h) ? 1 : 0;
        cursor_pointer->children[br] = new Node();
        cursor_pointer = cursor_pointer->children[br];
    }
    cursor_pointer->index = this->cursor;
    cursor_pointer->value = node->value;
    cursor_pointer->key = node->key;

    // 創建新的樹並回傳
    OrderTree *new_tree = this->new_tree();
    new_tree->root = new_root;
    new_tree->cursor++;
    std::pair<OrderTree*, Node*> ret = {new_tree, cursor_pointer};
    return ret;
}

OrderTree *OrderTree::update(Node *node, int value) {
    return nullptr;
}