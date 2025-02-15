/* DannyNiu/NJF, 2023-07-28. Public Domain. */

#define INTERCEPT_MEM_CALLS 1

#include "mem-intercept.c"
#include "s2obj.h"
#include "s2data.h"
#include "s2dict.h"
#include "s2list.h"

#define EnableDebugging false

int main()
{
    s2list_t *root;
    s2dict_t *frame;
    s2dict_t *view;
    s2data_t *toc;

    s2data_t *k_toc, *k_content, *k_parent, *k_root;

    int ret = EXIT_SUCCESS;
    int i;

    s2gc_set_threading(false);

    root = s2list_create();
#if EnableDebugging
    ((s2obj_t *)root)->dbg_c = 'L';
    ((s2obj_t *)root)->dbg_i = 'R';
#endif /* EnableDebugging */
        
    toc = s2data_create(4);
    *(int32_t *)s2data_map(toc, 0, 4) = 12;
    s2data_unmap(toc);
#if EnableDebugging
    ((s2obj_t *)toc)->dbg_c = 't';
    ((s2obj_t *)toc)->dbg_i = 65535;
#endif /* EnableDebugging */

    k_toc = s2data_create(3);
    k_content = s2data_create(3);
    k_parent = s2data_create(3);
    k_root = s2data_create(3);

#if EnableDebugging
    ((s2obj_t *)k_toc)->dbg_c = 'k';
    ((s2obj_t *)k_content)->dbg_c = 'k';
    ((s2obj_t *)k_parent)->dbg_c = 'k';
    ((s2obj_t *)k_root)->dbg_c = 'k';
#endif /* EnableDebugging */

    memcpy(s2data_map(k_toc, 0, 0), "toc", 3);
    memcpy(s2data_map(k_content, 0, 0), "con", 3);
    memcpy(s2data_map(k_parent, 0, 0), "par", 3);
    memcpy(s2data_map(k_root, 0, 0), "roo", 3);

    s2data_unmap(k_toc);
    s2data_unmap(k_content);
    s2data_unmap(k_parent);
    s2data_unmap(k_root);

    for(i=0; i<8; i++)
    {
        frame = s2dict_create();
        view = s2dict_create();
#if EnableDebugging
        ((s2obj_t *)frame)->dbg_c = 'f';
        ((s2obj_t *)frame)->dbg_i = i;
        ((s2obj_t *)view)->dbg_c = 'v';
        ((s2obj_t *)view)->dbg_i = i;
#endif /* EnableDebugging */

        // See note 2024-08-01.a in "s2dict-test.c"
        //- s2dict_set(view,  k_parent,  (s2obj_t *)frame, s2_setter_kept);
        //- s2dict_set(view,  k_root,    (s2obj_t *)root,  s2_setter_kept);
        //- s2dict_set(frame, k_toc,     (s2obj_t *)toc,   s2_setter_kept);
        //- s2dict_set(frame, k_content, (s2obj_t *)view,  s2_setter_gave);
        ;;  s2dict_set(view,  k_parent,  &frame->base, s2_setter_kept);
        ;;  s2dict_set(view,  k_root,    &root->base,  s2_setter_kept);
        ;;  s2dict_set(frame, k_toc,     &toc->base,   s2_setter_kept);
        ;;  s2dict_set(frame, k_content, &view->base,  s2_setter_gave);

        s2list_push(root, &frame->base, s2_setter_gave);
    }
    printf("toc: %p, root: %p.\n", toc, root);

    s2obj_release(&toc->base);
    s2list_push(root, &root->base, s2_setter_gave);

    s2obj_release(&k_toc->base);
    s2obj_release(&k_content->base);
    s2obj_release(&k_parent->base);
    s2obj_release(&k_root->base);

    s2gc_collect();

    for(i=0; i<4; i++)
    {
        if( mh[i] ) ret = EXIT_FAILURE;
        printf("%08lx%c", (long)mh[i], i==3 ? '\n' : ' ');
    }

    printf("mem-acquire: %ld, mem-release: %ld.\n", allocs, frees);

    return ret;
}
