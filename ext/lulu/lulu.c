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
    MARKER_LIST_FOR_VALUE_DECL(dst)

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
        MARKER *marker = self->markers[i];
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
    rb_define_alloc_func(klass, rb_new_marker_list);

    for (int i = 0; i < STATIC_ARRAY_SIZE(function_table); i++) {
        struct ft_entry *e = function_table + i;
        rb_define_method(klass, e->name, e->func, e->argc);
    }
    for (int i = 0; i < STATIC_ARRAY_SIZE(int_const_table); i++) {
        struct ict_entry *e = int_const_table + i;
        rb_define_const(klass, e->name, INT2FIX(e->val));
    }
    for (int i = 0; i < STATIC_ARRAY_SIZE(string_const_table); i++) {
        struct sct_entry *e = string_const_table + i;
        rb_define_const(klass, e->name, rb_str_new2(e->val));
    }
}

#if 0
#include "internal.h"

#define INIT_STRING_FROM_VALUE(String, Value) \
    STRING String[1] = {{ RSTRING_PTR(Value), RSTRING_LEN(Value) }}

static VALUE rb_api_endecrypt(VALUE self, VALUE bridge_as_string)
#define ARGC_endecrypt 1
{
    Check_Type(bridge_as_string, T_STRING);
    INIT_STRING_FROM_VALUE(bridge_internal_string, bridge_as_string);
    endecrypt(bridge_internal_string);
    return bridge_as_string;
}

/*
struct analysis_result_t {
    TVersion version;                           // WPBDC version of parsed bridge.
    char scenario_id[SCENARIO_ID_SIZE];         // WPBDC scenario ID.
    char scenario_number[SCENARIO_NUMBER_SIZE]; // WPBDC scenario number.
    TTestStatus test_status;                    // Load test flag from uploaded file.
    int status;                                 // Summary status
    TBridgeError error;                         // Bridge error condition.
    double cost;                                // Cost in dollars.
    char hash[HASH_SIZE];                       // Bridge hash value.
};
*/

static VALUE symbol(char *name)
{
    return ID2SYM(rb_intern(name));
}

static void add_analysis_to_hash(VALUE hash, struct analysis_result_t *result)
{
    char hex_hash[2 * HASH_SIZE + 1];

    rb_hash_aset(hash, symbol("version"), INT2FIX(result->version));
    rb_hash_aset(hash, symbol("scenario"), rb_str_new(result->scenario_id, SCENARIO_ID_SIZE));
    rb_hash_aset(hash, symbol("scenario_number"), rb_str_new(result->scenario_number, SCENARIO_NUMBER_SIZE));
    rb_hash_aset(hash, symbol("test_status"), INT2FIX(result->test_status));
    rb_hash_aset(hash, symbol("status"), INT2FIX(result->status));
    rb_hash_aset(hash, symbol("error"), INT2FIX(result->error));
    rb_hash_aset(hash, symbol("score"), rb_float_new(result->cost));
    if (result->status == BRIDGE_OK || result->status == BRIDGE_FAILEDTEST) {
        rb_hash_aset(hash, symbol("hash"),
            rb_str_new(hex_str(result->hash, HASH_SIZE, hex_hash), 2 * HASH_SIZE));
    }
}

static VALUE rb_api_analyze(VALUE self, VALUE bridge_as_string)
#define ARGC_analyze 1
{
    Check_Type(bridge_as_string, T_STRING);
    INIT_STRING_FROM_VALUE(bridge_internal_string, bridge_as_string);
    struct analysis_result_t result[1];
    VALUE hash = rb_hash_new();
    analyze(bridge_internal_string, result);
    add_analysis_to_hash(hash, result);
    return hash;
}

static VALUE rb_api_are_same(VALUE self, VALUE bridge_as_string_a, VALUE bridge_as_string_b)
#define ARGC_are_same 2
{
    Check_Type(bridge_as_string_a, T_STRING);
    Check_Type(bridge_as_string_b, T_STRING);
    INIT_STRING_FROM_VALUE(bridge_internal_string_a, bridge_as_string_a);
    INIT_STRING_FROM_VALUE(bridge_internal_string_b, bridge_as_string_b);
    int cmp = compare(bridge_internal_string_a, bridge_internal_string_b);
    return (cmp < 0) ? Qnil : (cmp > 0) ? Qtrue : Qfalse;
}

static VALUE c_str_to_value(char *s)
{
    if (s) {
        VALUE value = rb_str_new2(s);
        Safefree(s);
        return value;
    }
    return Qnil;
}

static VALUE rb_api_variant(VALUE self, VALUE bridge_as_string, VALUE seed)
#define ARGC_variant 2
{
    Check_Type(bridge_as_string, T_STRING);
    Check_Type(seed, T_FIXNUM);
    INIT_STRING_FROM_VALUE(bridge_internal_string, bridge_as_string);
    char *result = variant(bridge_internal_string, FIX2INT(seed));
    return c_str_to_value(result);
}

static VALUE rb_api_failed_variant(VALUE self, VALUE bridge_as_string, VALUE seed)
#define ARGC_failed_variant 2
{
    Check_Type(bridge_as_string, T_STRING);
    Check_Type(seed, T_FIXNUM);
    INIT_STRING_FROM_VALUE(bridge_internal_string, bridge_as_string);
    char *result = failed_variant(bridge_internal_string, FIX2INT(seed));
    return c_str_to_value(result);
}

static VALUE rb_api_perturbation(VALUE self, VALUE bridge_as_string, VALUE seed, VALUE n_joints, VALUE n_members)
#define ARGC_perturbation 4
{
    Check_Type(bridge_as_string, T_STRING);
    Check_Type(seed, T_FIXNUM);
    Check_Type(n_joints, T_FIXNUM);
    Check_Type(n_members, T_FIXNUM);
    INIT_STRING_FROM_VALUE(bridge_internal_string, bridge_as_string);
    char *result = perturbation(bridge_internal_string, FIX2INT(seed), FIX2INT(n_joints), FIX2INT(n_members));
    return c_str_to_value(result);
}

static VALUE rb_api_sketch(VALUE self, VALUE bridge_as_string, VALUE width, VALUE height)
#define ARGC_sketch 3
{
    Check_Type(bridge_as_string, T_STRING);
    Check_Type(width, T_FIXNUM);
    Check_Type(height, T_FIXNUM);
    INIT_STRING_FROM_VALUE(bridge_internal_string, bridge_as_string);
	COMPRESSED_IMAGE compressed_image[1];
	struct analysis_result_t result[1];
    VALUE hash = rb_hash_new();
	init_compressed_image(compressed_image);
	sketch(bridge_internal_string, FIX2INT(width), FIX2INT(height), compressed_image, result);

    /*
    struct analysis_result_t {
        TVersion version;                           // WPBDC version of parsed bridge.
        char scenario_id[SCENARIO_ID_SIZE];         // WPBDC scenario ID.
        char scenario_number[SCENARIO_NUMBER_SIZE]; // WPBDC scenario number.
        TTestStatus test_status;                    // Load test flag from uploaded file.
        int status;                                 // Summary status
        TBridgeError error;                         // Bridge error condition.
        double cost;                                // Cost in dollars.
        char hash[HASH_SIZE];                       // Bridge hash value.
    };
    */
    add_analysis_to_hash(hash, result);
    if (result->status == BRIDGE_OK) {
        rb_hash_aset(hash, symbol("image"), rb_str_new(compressed_image->data, compressed_image->filled));
        clear_compressed_image(compressed_image);
    }
    return hash;
}

static VALUE rb_api_analysis_table(VALUE self, VALUE bridge_as_string)
#define ARGC_analysis_table 1
{
    Check_Type(bridge_as_string, T_STRING);
    INIT_STRING_FROM_VALUE(bridge_internal_string, bridge_as_string);
    char *result = analysis_table(bridge_internal_string);
    return c_str_to_value(result);
}

static VALUE rb_api_analysis_log(VALUE self, VALUE bridge_as_string)
#define ARGC_analysis_log 1
{
    Check_Type(bridge_as_string, T_STRING);
    INIT_STRING_FROM_VALUE(bridge_internal_string, bridge_as_string);
    char *result = analysis_log(bridge_internal_string);
    return c_str_to_value(result);
}

static VALUE rb_api_local_contest_number_to_id(VALUE self, VALUE number_as_string)
#define ARGC_local_contest_number_to_id 1
{
    Check_Type(number_as_string, T_STRING);
    INIT_STRING_FROM_VALUE(number_internal_string, number_as_string);
    char *result = get_local_contest_number(number_internal_string);
    // Don't use c_str_to_value here because result is static.
    return result ? rb_str_new2(result) : Qnil;
}


#define FUNCTION_TABLE_ENTRY(Name) { #Name, rb_api_ ## Name, ARGC_ ## Name }

static struct ft_entry {
  char *name;
  VALUE (*func)();
  int argc;
} function_table[] = {
  FUNCTION_TABLE_ENTRY(endecrypt),
  FUNCTION_TABLE_ENTRY(analyze),
  FUNCTION_TABLE_ENTRY(are_same),
  FUNCTION_TABLE_ENTRY(variant),
  FUNCTION_TABLE_ENTRY(failed_variant),
  FUNCTION_TABLE_ENTRY(perturbation),
  FUNCTION_TABLE_ENTRY(sketch),
  FUNCTION_TABLE_ENTRY(analysis_table),
  FUNCTION_TABLE_ENTRY(analysis_log),
  FUNCTION_TABLE_ENTRY(local_contest_number_to_id),
};

#define INT_CONST_TABLE_ENTRY(Name) { #Name, Name }

static struct ict_entry {
  char *name;
  int val;
} int_const_table[] = {
  INT_CONST_TABLE_ENTRY(CONTEST_YEAR),
  INT_CONST_TABLE_ENTRY(BRIDGE_OK),
  INT_CONST_TABLE_ENTRY(BRIDGE_MALFORMED),
  INT_CONST_TABLE_ENTRY(BRIDGE_WRONGVERSION),
  INT_CONST_TABLE_ENTRY(BRIDGE_FAILEDTEST),

  INT_CONST_TABLE_ENTRY(MAX_JOINTS),
  INT_CONST_TABLE_ENTRY(MAX_MEMBERS),

  // Size of hash value in bytes.
  INT_CONST_TABLE_ENTRY(HASH_SIZE),

  // Possible errors during bridge parsing.
  INT_CONST_TABLE_ENTRY(BridgeNoError),
  INT_CONST_TABLE_ENTRY(BridgeMissingData),
  INT_CONST_TABLE_ENTRY(BridgeSyntax),
  INT_CONST_TABLE_ENTRY(BridgeMissingHeader),
  INT_CONST_TABLE_ENTRY(BridgeBadHeader),
  INT_CONST_TABLE_ENTRY(BridgeMissingTestStatus),
  INT_CONST_TABLE_ENTRY(BridgeBadTestStatus),
  INT_CONST_TABLE_ENTRY(BridgeBadScenario),
  INT_CONST_TABLE_ENTRY(BridgeBadNDesignIterations),
  INT_CONST_TABLE_ENTRY(BridgeTooManyElements),
  INT_CONST_TABLE_ENTRY(BridgeBadJointBanner),
  INT_CONST_TABLE_ENTRY(BridgeTooFewJoints),
  INT_CONST_TABLE_ENTRY(BridgeWrongPrescribedJoints),
  INT_CONST_TABLE_ENTRY(BridgeBadMemberBanner),
  INT_CONST_TABLE_ENTRY(BridgeTooFewMembers),
  INT_CONST_TABLE_ENTRY(BridgeBadLabelPos),
  INT_CONST_TABLE_ENTRY(BridgeExtraJunk),
  INT_CONST_TABLE_ENTRY(BridgeDupJoints),
  INT_CONST_TABLE_ENTRY(BridgeDupMembers),
  INT_CONST_TABLE_ENTRY(BridgeJointOnMember),
  INT_CONST_TABLE_ENTRY(BridgeBadLoadScenario),
  INT_CONST_TABLE_ENTRY(BridgeBadChar),

  // Test status flag values read from bridge file.
  INT_CONST_TABLE_ENTRY(NullTestStatus),
  INT_CONST_TABLE_ENTRY(Unrecorded),
  INT_CONST_TABLE_ENTRY(Untested),
  INT_CONST_TABLE_ENTRY(Failed),
  INT_CONST_TABLE_ENTRY(Passed),

  INT_CONST_TABLE_ENTRY(SCENARIO_ID_SIZE),
  INT_CONST_TABLE_ENTRY(SCENARIO_NUMBER_SIZE),
};

#define STRING_CONST_TABLE_ENTRY(Name) { #Name, Name }

static struct sct_entry {
  char *name;
  char *val;
} string_const_table[] = {
    STRING_CONST_TABLE_ENTRY(SEMIFINAL_SCENARIO_ID)
};

void Init_lulu(void)
{
  int i;
  VALUE module;

  module = rb_define_module("Lulu");

  for (i = 0; i < STATIC_ARRAY_SIZE(function_table); i++) {
    struct ft_entry *e = function_table + i;
    rb_define_module_function(module, e->name, e->func, e->argc);
  }
  for (i = 0; i < STATIC_ARRAY_SIZE(int_const_table); i++) {
    struct ict_entry *e = int_const_table + i;
    rb_define_const(module, e->name, INT2FIX(e->val));
  }
  for (i = 0; i < STATIC_ARRAY_SIZE(string_const_table); i++) {
    struct sct_entry *e = string_const_table + i;
    rb_define_const(module, e->name, rb_str_new2(e->val));
  }
}

#endif

#endif
