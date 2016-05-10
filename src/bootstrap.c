#include "module.h"
#include "bootstrap.h"

#include "modules/heavyhitter/countmin.h"
#include "modules/heavyhitter/cuckoo.h"
#include "modules/heavyhitter/cuckoo_bucket.h"
#include "modules/heavyhitter/cuckoo_local.h"
#include "modules/heavyhitter/cuckoo_local_ptr.h"
#include "modules/heavyhitter/hashmap.h"
#include "modules/heavyhitter/hashmap_linear.h"
#include "modules/heavyhitter/hashmap_linear_ptr.h"
#include "modules/heavyhitter/pqueue.h"

#include "modules/randmod.h"

void boostrap_register_modules(void) {
    REGISTER_MODULE("HeavyHitter::CountMin",                heavyhitter_countmin);
    REGISTER_MODULE("HeavyHitter::Cuckoo",                  heavyhitter_cuckoo);
    REGISTER_MODULE("HeavyHitter::CuckooBucket",            heavyhitter_cuckoo_bucket);
    REGISTER_MODULE("HeavyHitter::CuckooLocal",             heavyhitter_cuckoo_local);
    REGISTER_MODULE("HeavyHitter::CuckooLocalPointer",      heavyhitter_cuckoo_local_ptr);
    REGISTER_MODULE("HeavyHitter::Hashmap",                 heavyhitter_hashmap);
    REGISTER_MODULE("HeavyHitter::HashmapLinear",           heavyhitter_hashmap_linear);
    REGISTER_MODULE("HeavyHitter::HashmapLinearPointer",    heavyhitter_hashmap_linear_ptr);
    REGISTER_MODULE("HeavyHitter::PQueue",                  heavyhitter_pqueue);

    REGISTER_MODULE("Generic::RandMod",                     randmod);
}
