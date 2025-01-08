#include "../parser/fparse.h"

bool typeCheck(ASTNode* program) {

}

void traverseProgram(ASTNode* node) {

    if (node->type)

    // Traverse Children
    for (size_t i = 0; i < node->childCount; i++) {
        traverseProgram(node->children[i]);
    }
   
    // Then traverse next
    if (node->next) {
        traverseProgram(node->next);
    }
}