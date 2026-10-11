typedef struct HostLinkNode HostLinkNode;
struct HostLinkNode { HostLinkNode *next; };

int __cdecl host_callback_a140(void)
{
    return 0x22;
}

void __cdecl host_callback_ac50(HostLinkNode **head, HostLinkNode *node)
{
    node->next = *head;
    *head = node;
}
