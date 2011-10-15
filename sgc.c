
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sgc.h"

static bool_t gc_enabled = 1;
static gc_visitor_t visitor_types[256];
static gc_finalizer_t finalizer_types[256];

// 8K pointers, 64KByte of stack.
static const size_t STACK_SIZE = 1024 * 8;

// @see seval:new_frame() to see the frame layout.
static obj_t **stack = NULL;
static obj_t **sp;

#define MIN_HEAP_SIZE (1024 * 1024 * 4)
static obj_t *gc_head = NULL;
static size_t next_collect = MIN_HEAP_SIZE;
static const double expand_factor = 1.5;
static size_t heap_size = 0;

void
sgc_init()
{
    static bool_t initialized = 0;
    if (initialized)
        return;

    initialized = 1;
    stack = (obj_t **)malloc(STACK_SIZE * sizeof(obj_t *));
    memset(stack, 0, STACK_SIZE * sizeof(obj_t *));
    sp = stack + STACK_SIZE;
}

void
gc_set_enabled(bool_t p)
{
    gc_enabled = p;
}

bool_t
gc_is_enabled()
{
    return gc_enabled;
}

obj_t *
gc_malloc(size_t size, type_t ob_type)
{
    obj_t *res;

    if (ob_type > TP_MAX) {
        // Invalid type
        fatal_error("gc_malloc: unknown type of object");
    }

    size += sizeof(header_obj_t);
    if (heap_size + size > next_collect && gc_enabled) {
        res = NULL;
    }
    else {
        res = malloc(size);
        if (res) {
            res->gc_next = gc_head;
            res->gc_marked = 0;
            res->ob_type = ob_type;
            res->ob_size = size;

            gc_head = res;
            heap_size += size;
        }
    }
    return res;
}

void
gc_register_type(type_t tp_index, gc_visitor_t v, gc_finalizer_t fini)
{
    visitor_types[tp_index] = v;
    finalizer_types[tp_index] = fini;
}

size_t
gc_mark(obj_t *self)
{
    size_t mark_count = 0;
    while (self && !self->gc_marked) {
        // Debug
        //fprintf(stderr, "(MARK! %p) [%ld]\n", self, mark_count);

        ++mark_count;
        self->gc_marked = 1;
        self = visitor_types[get_type(self)](self);
    }
    return mark_count;
}

size_t
gc_collect(obj_t **frame_ptr)
{
    obj_t **iter;
    obj_t *self, *victim;
    size_t size_pre_gc = heap_size;
    size_t bytes_freed;

    if (!gc_enabled) {
        // Debug
        //fprintf(stderr, "; heap (%ld/%ld), gc supressed...\n",
        //        heap_size, next_collect);
        return 0;
    }

    // Debug
    //fprintf(stderr, "; heap (%ld/%ld), gc collecting...\n",
    //        heap_size, next_collect);

    // Firstly mark the stack
    for (iter = frame_ptr; iter < stack + STACK_SIZE; ++iter) {
        // XXX: be aware of the frame pointer:
        // If *iter is in the stack, then just ignore it.
        // @see seval:new_frame()
        if ((obj_t **)*iter > iter && (obj_t **)*iter < stack + STACK_SIZE)
            continue;
        gc_mark(*iter);
    }

    // Then sweep. Don't look at the head now.
    if (gc_head) {
        self = gc_head;
        // XXX: Be careful when writing singly-linked-list...
        while (self->gc_next) {
            victim = self->gc_next;
            if (!victim->gc_marked) {
                self->gc_next = victim->gc_next;
                heap_size -= victim->ob_size;
                finalizer_types[get_type(victim)](victim);
            }
            else {
                self = self->gc_next;
            }
        }

        // Sweep the head.
        victim = gc_head;
        if (!victim->gc_marked) {
            gc_head = victim->gc_next;
            heap_size -= victim->ob_size;
            finalizer_types[get_type(victim)](victim);
        }
    }

    // Clean marks
    for (self = gc_head; self; self = self->gc_next) {
        // Must be all marked
        self->gc_marked = 0;
    }

    bytes_freed = size_pre_gc - heap_size;

    // Record the current size to determine the heap threshold for next gc.
    next_collect = heap_size * expand_factor;
    if (next_collect < MIN_HEAP_SIZE) {
        next_collect = MIN_HEAP_SIZE;
    }

    // Debug
    //fprintf(stderr, "; collection result => (%ld/%ld), %lu bytes freed\n",
    //        heap_size, next_collect, bytes_freed);

    return bytes_freed;
}

bool_t
gc_want_collect()
{
    return heap_size > next_collect && gc_enabled;
}

obj_t **
gc_get_stack_base()
{
    return sp;
}

void
gc_set_stack_base(obj_t **new_sp)
{
    sp = new_sp;
}

void
gc_incr_stack_base(long nb_incr)
{
    sp += nb_incr;
}

void
gc_print_heap()
{
    obj_t *iter;
    long i = 0;
    fprintf(stderr, "\nGC-PRINT-HEAP\n=============\n\n");
    for (iter = gc_head; iter; iter = iter->gc_next) {
        fprintf(stderr, "#%3ld  %p  ", i++, iter);
        print_repr(iter, stderr);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "\n*************\nGC-PRINT-HEAP\n\n");
}

void
gc_print_stack(obj_t **frame)
{
    obj_t **iter;
    long i = 0;
    fprintf(stderr, "\nGC-PRINT-STACK\n==============\n\n");
    for (iter = frame; iter < stack + STACK_SIZE; ++iter) {
        // If *iter is in the stack, then just ignore it.
        if ((obj_t **)*iter > iter && (obj_t **)*iter < stack + STACK_SIZE) {
            fprintf(stderr, "#%3ld  %p  [frame-pointer-dump]\n", i++, *iter);
        }
        else if (!*iter) {
            fprintf(stderr, "#%3ld  %p\n", i++, *iter);
        }
        else {
            fprintf(stderr, "#%3ld  %p  ", i++, *iter);
            print_repr(*iter, stderr);
            fprintf(stderr, "\n");
        }
    }
    fprintf(stderr, "\n**************\nGC-PRINT-STACK\n\n");
}

