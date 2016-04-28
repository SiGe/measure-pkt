#include "module.h"
#include "bootstrap.h"

#include "modules/count_array_cuckoo_local.h"
#include "modules/count_array_cuckoo.h"
#include "modules/count_array_hashmap.h"
#include "modules/count_array_hashmap_linear.h"
#include "modules/count_array_pqueue.h"

void boostrap_register_modules(void) {
    REGISTER_MODULE("HeavyHitter::Cuckoo", count_array_cuckoo);
    REGISTER_MODULE("HeavyHitter::CuckooLocal", count_array_cuckoo_local);
    REGISTER_MODULE("HeavyHitter::Hashmap", count_array_hashmap);
    REGISTER_MODULE("HeavyHitter::HashmapLinear", count_array_hashmap_linear);
    REGISTER_MODULE("HeavyHitter::PQueue", count_array_pqueue);
}
