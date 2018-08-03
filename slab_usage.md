## step  

| step | kapi | file | function |
|------|------|------|----------|
|Sreate cache |kmem_cache_create|mm/slab_common.c|create a kmem_cache structure|
|Allocate an object|kmem_cache_alloc|mm/slab.c|allocate a object from kmem_cache|
|Deallocate an object|kmem_cache_free|mm/slab.c|Deallocate an object|

kmem_cache_zalloc = kmem_cache_alloc + memset(0)
kmem_cache_alloc -> slab_alloc -> _do_cache_alloc -> _____cache_alloc -> cpu_cache_get && kmemleak_erase
kmem_cache_free -> __cache_free -> ___cache_free (PAX_MEMORY_SANITIZE)
