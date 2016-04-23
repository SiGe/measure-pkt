#include <string.h>
#include "module.h"

struct ModuleList _all_modules;

void _module_register(
        char const* name,
        ModulePtr (*init) (ModuleConfigPtr),
        void (*del)(ModulePtr),
        void (*execute)(ModulePtr, PortPtr, struct rte_mbuf **, uint32_t),
        void (*reset)(ModulePtr)) {
    struct ModuleList *lst = malloc(sizeof(struct ModuleList));
    lst->next = _all_modules.next;
    lst->init = init;
    lst->del  = del;
    lst->execute = execute;
    lst->reset = reset;
    strncpy(lst->name, name, 255);
    _all_modules.next = lst;
}

static struct ModuleList *module_get(char const *name) {
    struct ModuleList *ptr = &_all_modules;

    while (ptr) {
        if (strcasecmp(name, ptr->name) == 0)
            return ptr;
        ptr = ptr->next;
    }

    printf("Failed to find module: %s\n", name);
    exit(EXIT_FAILURE);
    return 0;
}

ModuleInit module_init_get(char const* name) {
    return module_get(name)->init;
}

ModuleDelete module_delete_get(char const* name) {
    return module_get(name)->del;
}

ModuleExecute module_execute_get(char const* name) {
    return module_get(name)->execute;
}

ModuleReset module_reset_get(char const* name) {
    return module_get(name)->reset;
}
