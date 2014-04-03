/*
 ============================================================================
 Name        : lulu.c
 Author      : Eugene K. Ressler
 Version     : 0.1
 Copyright   : GPLv3
 Description : The Lulu gem for merging map markers.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ruby.h"
#include "utility.h"
#include "marker.h"
#include "merger.h"
#include "pq.h"
#include "qt.h"

static char EXT_VERSION[] = "0.1.2";

// -------- C marker list to be wrapped in a Ruby object -----------------------

typedef struct marker_list_s {
    MARKER_INFO info[1];
    MARKER *markers;
    int size, max_size;
} MARKER_LIST;

#define MARKER_LIST_DECL(Name)  MARKER_LIST Name[1]; init_marker_list(Name)
#define ml_set_marker_list_info(L, Kind, Scale)  mr_info_set((L)->info, (Kind), (Scale))

void init_marker_list(MARKER_LIST *list) {
    mr_info_init(list->info);
    list->markers = NULL;
    list->size = list->max_size = 0;
}

MARKER_LIST *new_marker_list(void) {
    NewDecl(MARKER_LIST, list);
    init_marker_list(list);
    return list;
}

void clear_marker_list(MARKER_LIST *list) {
    Free(list->markers);
    init_marker_list(list);
}

void free_marker_list(MARKER_LIST *list) {
    clear_marker_list(list);
    Free(list);
}

void add_marker(MARKER_LIST *list, COORD x, COORD y, SIZE size) {
    if (list->size >= list->max_size) {
        list->max_size = 4 + 2 * list->max_size;
        RenewArray(list->markers, list->max_size);
    }
    MARKER *marker = list->markers + list->size++;
    mr_set(list->info, marker, x, y, size);
}

void ensure_headroom(MARKER_LIST *list) {
    int needed_size = 2 * list->size - 1;
    if (list->max_size < needed_size) {
        list->max_size = needed_size;
        RenewArray(list->markers, list->max_size);
    }
}

void compress(MARKER_LIST *list) {
    int dst = 0;
    for (int src = 0; src < list->size; src++)
        if (!mr_deleted_p(list->markers + src)) {
            if (src != dst)
                list->markers[dst] = list->markers[src];
            mr_reset_parts(list->markers + dst);
            dst++;
        }
    list->size = dst;
}

// -------- Ruby API implementation --------------------------------------------

static void rb_api_free_marker_list(void *list) {
    free_marker_list(list);
}

static VALUE rb_api_new_marker_list(VALUE klass) {
    MARKER_LIST *list = new_marker_list();
    return Data_Wrap_Struct(klass, 0, rb_api_free_marker_list, list);
}

#define MARKER_LIST_FOR_VALUE_DECL(Var) MARKER_LIST *Var; Data_Get_Struct(Var ## _value, MARKER_LIST, Var)

static VALUE rb_api_initialize_copy(VALUE dst_value, VALUE src_value)
#define ARGC_initialize_copy 1
{
    if (dst_value == src_value)
        return src_value;

    if (TYPE(src_value) != T_DATA || RDATA(src_value)->dfree != (RUBY_DATA_FUNC)rb_api_free_marker_list)
        rb_raise(rb_eTypeError, "type mismatch (copy_marker_list)");

    MARKER_LIST_FOR_VALUE_DECL(src);
    MARKER_LIST_FOR_VALUE_DECL(dst);

    // Shallow copy contents, then deep copy array of markers.
    *dst = *src;
    NewArray(dst->markers, dst->max_size);
    CopyArray(dst->markers, src->markers, dst->max_size);

    return dst_value;
}

static VALUE rb_api_clear(VALUE self_value)
#define ARGC_clear 0
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    clear_marker_list(self);
    return self_value;
}

static VALUE rb_api_set_info(VALUE self_value, VALUE kind_value, VALUE scale_value)
#define ARGC_set_info 2
{
    MARKER_LIST_FOR_VALUE_DECL(self);

    // Get the valid symbol values.
    VALUE square_sym = ID2SYM(rb_intern("square"));
    VALUE circle_sym = ID2SYM(rb_intern("circle"));

    // Try to convert the kind into a symbol.  This could cause an exception.
    VALUE kind_as_sym = rb_funcall(kind_value, rb_intern("to_sym"), 0);

    // Also cause an exception if it's an incorrect symbol value.
    if (kind_as_sym != square_sym && kind_as_sym != circle_sym)
        rb_raise(rb_eTypeError, "invalid symbol for marker kind (set_info)");

    mr_info_set(self->info, kind_as_sym == square_sym ? SQUARE : CIRCLE, rb_num2dbl(scale_value));

    return self_value;
}

static VALUE rb_api_add(VALUE self_value, VALUE x_value, VALUE y_value, VALUE size_value)
#define ARGC_add 3
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    add_marker(self, rb_num2dbl(x_value), rb_num2dbl(y_value), rb_num2dbl(size_value));
    return INT2FIX(self->size);
}

static VALUE rb_api_length(VALUE self_value)
#define ARGC_length 0
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    return INT2FIX(self->size);
}

static VALUE rb_api_marker(VALUE self_value, VALUE index)
#define ARGC_marker 1
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    int i = NUM2INT(index);
    if (0 <= i && i < self->size) {
        MARKER *marker = self->markers + i;
        VALUE triple = rb_ary_new2(3);
        rb_ary_store(triple, 0, rb_float_new(mr_x(marker)));
        rb_ary_store(triple, 1, rb_float_new(mr_y(marker)));
        rb_ary_store(triple, 2, rb_float_new(marker->size));
        return triple;
    }
    return Qnil;
}

static VALUE rb_api_parts(VALUE self_value, VALUE index)
#define ARGC_parts 1
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    int i = NUM2INT(index);
    if (0 <= i && i < self->size) {
        MARKER *marker = self->markers + i;
        VALUE rtn;
        if (mr_merged(marker)) {
            rtn = rb_ary_new2(3);
            rb_ary_store(rtn, 0, ID2SYM(rb_intern(mr_deleted_p(marker) ? "merge" : "root")));
            rb_ary_store(rtn, 1, INT2FIX(marker->part_a));
            rb_ary_store(rtn, 2, INT2FIX(marker->part_b));
        } else {
            rtn = rb_ary_new2(1);
            rb_ary_store(rtn, 0, ID2SYM(rb_intern(mr_deleted_p(marker) ? "leaf" : "single")));
        }
        return rtn;
    }
    return Qnil;
}

static VALUE rb_api_deleted(VALUE self_value, VALUE index)
#define ARGC_deleted 1
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    int i = NUM2INT(index);
    if (0 <= i && i < self->size)
        return mr_deleted_p(self->markers + i) ? Qtrue : Qfalse;
    return Qnil;
}

static VALUE rb_api_compress(VALUE self_value)
#define ARGC_compress 0
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    compress(self);
    return INT2FIX(self->size);
}

static VALUE rb_api_merge(VALUE self_value)
#define ARGC_merge 0
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    ensure_headroom(self);
    compress(self);
    self->size = merge_markers_fast(self->info, self->markers, self->size);
    return INT2FIX(self->size);
}

#define FUNCTION_TABLE_ENTRY(Name) { #Name, RUBY_METHOD_FUNC(rb_api_ ## Name), ARGC_ ## Name }

static struct ft_entry {
    const char *name;
    VALUE (*func)(ANYARGS);
    int argc;
} function_table[] = {
    FUNCTION_TABLE_ENTRY(add),
    FUNCTION_TABLE_ENTRY(compress),
    FUNCTION_TABLE_ENTRY(clear),
    FUNCTION_TABLE_ENTRY(deleted),
    FUNCTION_TABLE_ENTRY(initialize_copy),
    FUNCTION_TABLE_ENTRY(length),
    FUNCTION_TABLE_ENTRY(marker),
    FUNCTION_TABLE_ENTRY(merge),
    FUNCTION_TABLE_ENTRY(parts),
    FUNCTION_TABLE_ENTRY(set_info),
};

#define STRING_CONST_TABLE_ENTRY(Name) { #Name, Name }

static struct sct_entry {
    const char *name;
    const char *val;
} string_const_table[] = {
    STRING_CONST_TABLE_ENTRY(EXT_VERSION)
};

void Init_lulu(void)
{
    VALUE module = rb_define_module("Lulu");
    VALUE klass = rb_define_class_under(module, "MarkerList", rb_cObject);
    rb_define_alloc_func(klass, rb_api_new_marker_list);

    for (int i = 0; i < STATIC_ARRAY_SIZE(function_table); i++) {
        struct ft_entry *e = function_table + i;
        rb_define_method(klass, e->name, e->func, e->argc);
    }

    for (int i = 0; i < STATIC_ARRAY_SIZE(string_const_table); i++) {
        struct sct_entry *e = string_const_table + i;
        rb_define_const(module, e->name, rb_str_new2(e->val));
    }
}
