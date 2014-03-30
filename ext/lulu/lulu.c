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
#include "lulu.h"
#include "utility.h"
#include "marker.h"
#include "merger.h"
#include "pq.h"
#include "qt.h"

char EXT_VERSION[] = "0.1.0";

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

// ---- Ruby interface code ----------------------------------------------------

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
        rb_raise(rb_eTypeError, "bad argument type (copy_marker_list)");

    MARKER_LIST_FOR_VALUE_DECL(src);
    MARKER_LIST_FOR_VALUE_DECL(dst);

    // Shallow copy contents, then deep copy array of markers.
    *dst = *src;
    NewArray(dst->markers, dst->max_size);
    CopyArray(dst->markers, src->markers, dst->max_size);

    return dst_value;
}

static VALUE rb_api_set_info(VALUE self_value, VALUE kind_value, VALUE scale_value)
#define ARGC_set_info 2
{
   MARKER_LIST_FOR_VALUE_DECL(self);
   VALUE square_sym = ID2SYM(rb_intern("square"));
   VALUE kind_as_sym = rb_funcall(kind_value, rb_intern("to_sym"), 0);
   mr_info_set(self->info, kind_as_sym == square_sym ? SQUARE : CIRCLE, rb_num2dbl(scale_value));
   return self_value;
}

static VALUE rb_api_add(VALUE self_value, VALUE x_value, VALUE y_value, VALUE size_value)
#define ARGC_add 3
{
    MARKER_LIST_FOR_VALUE_DECL(self);

    add_marker(self, rb_num2dbl(x_value), rb_num2dbl(y_value), rb_num2dbl(size_value));
    return self_value;
}

static VALUE rb_api_length(VALUE self_value)
#define ARGC_length 0
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    return rb_int2inum(self->size);
}

static VALUE rb_api_marker(VALUE self_value, VALUE index)
#define ARGC_marker 1
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    int i = NUM2INT(index);
    if (0 <= i && i < self->size) {
        MARKER *marker = self->markers + i;
        if (mr_deleted_p(marker))
            return Qfalse;
        VALUE triple = rb_ary_new2(3);
        rb_ary_store(triple, 0, rb_float_new(mr_x(marker)));
        rb_ary_store(triple, 1, rb_float_new(mr_y(marker)));
        rb_ary_store(triple, 2, rb_float_new(marker->size));
        return triple;
    }
    return Qnil;
}

static VALUE rb_api_merge(VALUE self_value)
#define ARGC_merge 0
{
    MARKER_LIST_FOR_VALUE_DECL(self);
    merge_markers_fast(self->info, self->markers, self->size);
    return self_value;
}

#define FUNCTION_TABLE_ENTRY(Name) { #Name, rb_api_ ## Name, ARGC_ ## Name }

static struct ft_entry {
  char *name;
  VALUE (*func)();
  int argc;
} function_table[] = {
    FUNCTION_TABLE_ENTRY(initialize_copy),
    FUNCTION_TABLE_ENTRY(set_info),
    FUNCTION_TABLE_ENTRY(add),
    FUNCTION_TABLE_ENTRY(length),
    FUNCTION_TABLE_ENTRY(marker),
    FUNCTION_TABLE_ENTRY(merge),
};

#define INT_CONST_TABLE_ENTRY(Name) { #Name, Name }

static struct ict_entry {
  char *name;
  int val;
} int_const_table[] = {
};

#define STRING_CONST_TABLE_ENTRY(Name) { #Name, Name }

static struct sct_entry {
  char *name;
  char *val;
} string_const_table[] = {
    STRING_CONST_TABLE_ENTRY(EXT_VERSION)
};

void Init_Lulu(void)
{
    VALUE module = rb_define_module("Lulu");
    VALUE klass = rb_define_class_under(module, "MarkerList", rb_cObject);
    rb_define_alloc_func(klass, rb_api_new_marker_list);

    for (int i = 0; i < STATIC_ARRAY_SIZE(function_table); i++) {
        fprintf(stderr, "Adding!\n");
        struct ft_entry *e = function_table + i;
        rb_define_method(klass, e->name, e->func, e->argc);
    }
    for (int i = 0; i < STATIC_ARRAY_SIZE(int_const_table); i++) {
        struct ict_entry *e = int_const_table + i;
        rb_define_const(module, e->name, INT2FIX(e->val));
    }
    for (int i = 0; i < STATIC_ARRAY_SIZE(string_const_table); i++) {
        struct sct_entry *e = string_const_table + i;
        rb_define_const(module, e->name, rb_str_new2(e->val));
    }
}
