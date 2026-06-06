/* Backport additions from fix_oplus_bsp_midas.patch - minimal compatibility shims
 * This file will be copied to common/include/linux/cgroup-defs.h to provide
 * missing symbols that caused CI build errors (CGROUP_SUBSYS_EXT_SLOTS, css_set_* helpers).
 * NOTE: This is a focused replacement containing the necessary compatibility
 * definitions extracted from the upstream patch. If further symbols are needed,
 * we'll expand this file accordingly.
 */

#ifndef _LINUX_CGROUP_DEFS_BACKPORT_H
#define _LINUX_CGROUP_DEFS_BACKPORT_H

#include <linux/list.h>
#include <linux/refcount.h>

/* The following definitions preserve ABI compatibility for older css_set layout.
 * We keep an "ext" slot to store the last CGROUP_SUBSYS_EXT_SLOTS entries.
 */

#ifndef CGROUP_SUBSYS_COUNT
#define CGROUP_SUBSYS_COUNT 16
#endif

#define CGROUP_SUBSYS_EXT_SLOTS 4
#define CGROUP_SUBSYS_MAIN_COUNT (CGROUP_SUBSYS_COUNT - CGROUP_SUBSYS_EXT_SLOTS)

struct cgroup_subsys_state;
struct css_set;

struct css_set_ext {
    struct css_set *owner;
    struct cgroup_subsys_state *subsys[CGROUP_SUBSYS_EXT_SLOTS];
    struct list_head e_cset_node[CGROUP_SUBSYS_EXT_SLOTS];
};

struct css_set {
    struct cgroup_subsys_state *subsys_main[CGROUP_SUBSYS_MAIN_COUNT];
    refcount_t refcount;
    struct list_head e_cset_node_main[CGROUP_SUBSYS_MAIN_COUNT];
    struct list_head threaded_csets;
    struct rcu_head rcu_head;
    union {
        struct { ANDROID_BACKPORT_RESERVE(1); };
        struct { struct css_set_ext *ext; };
    };
};

static inline bool css_set_uses_ext_ssid(int ssid)
{
    return ssid >= CGROUP_SUBSYS_MAIN_COUNT;
}

static inline int css_set_ext_index(int ssid)
{
    return ssid - CGROUP_SUBSYS_MAIN_COUNT;
}

static inline struct cgroup_subsys_state *css_set_get_subsys(struct css_set *cset, int ssid)
{
    if (css_set_uses_ext_ssid(ssid))
        return cset->ext->subsys[css_set_ext_index(ssid)];
    return cset->subsys_main[ssid];
}

static inline void css_set_set_subsys(struct css_set *cset, int ssid, struct cgroup_subsys_state *css)
{
    if (css_set_uses_ext_ssid(ssid)) {
        cset->ext->subsys[css_set_ext_index(ssid)] = css;
        return;
    }
    cset->subsys_main[ssid] = css;
}

static inline struct list_head *css_set_e_cset_node(struct css_set *cset, int ssid)
{
    if (css_set_uses_ext_ssid(ssid))
        return &cset->ext->e_cset_node[css_set_ext_index(ssid)];
    return &cset->e_cset_node_main[ssid];
}

static inline struct css_set *css_set_from_e_cset_node(struct list_head *node, int ssid)
{
    if (css_set_uses_ext_ssid(ssid)) {
        return container_of(node, struct css_set_ext, e_cset_node[css_set_ext_index(ssid)])->owner;
    }
    return container_of(node, struct css_set, e_cset_node_main[ssid]);
}

/* cgroup extensions stored per-cgroup to hold ext arrays */
struct cgroup_ext {
    struct cgroup_subsys_state __rcu *subsys[CGROUP_SUBSYS_EXT_SLOTS];
    int nr_dying_subsys[CGROUP_SUBSYS_EXT_SLOTS];
    struct list_head e_csets[CGROUP_SUBSYS_EXT_SLOTS];
};

static inline struct cgroup_subsys_state __rcu **cgroup_subsys_ptr(struct cgroup *cgrp, int ssid)
{
    if (css_set_uses_ext_ssid(ssid))
        return &cgrp->ext->subsys[css_set_ext_index(ssid)];
    return &cgrp->subsys_main[ssid];
}

static inline int *cgroup_nr_dying_subsys_ptr(struct cgroup *cgrp, int ssid)
{
    if (css_set_uses_ext_ssid(ssid))
        return &cgrp->ext->nr_dying_subsys[css_set_ext_index(ssid)];
    return &cgrp->nr_dying_subsys_main[ssid];
}

static inline struct list_head *cgroup_e_csets_ptr(struct cgroup *cgrp, int ssid)
{
    if (css_set_uses_ext_ssid(ssid))
        return &cgrp->ext->e_csets[css_set_ext_index(ssid)];
    return &cgrp->e_csets_main[ssid];
}
