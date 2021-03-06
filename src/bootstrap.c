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
#include "modules/heavyhitter/sharedmap.h"

#include "modules/superspreader/cuckoo_bucket.h"
#include "modules/superspreader/cuckoo_local.h"
#include "modules/superspreader/hashmap.h"
#include "modules/superspreader/hashmap_linear.h"

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
    REGISTER_MODULE("HeavyHitter::Sharedmap",               heavyhitter_sharedmap);

    REGISTER_MODULE("SuperSpreader::Hashmap",               superspreader_hashmap);
    REGISTER_MODULE("SuperSpreader::HashmapLinear",         superspreader_hashmap_linear);
    REGISTER_MODULE("SuperSpreader::CuckooLocal",           superspreader_cuckoo_local);
    REGISTER_MODULE("SuperSpreader::CuckooBucket",          superspreader_cuckoo_bucket);

    REGISTER_MODULE("Generic::RandMod",                     randmod);
}
