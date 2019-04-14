#include "f4se/NiNodes.h"

NiNode * NiNode::Create(UInt16 children)
{
	NiNode * node = (NiNode*)Heap_Allocate(sizeof(NiNode));
	CALL_MEMBER_FN(node, ctor)(children);
	return node;
}
