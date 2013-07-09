#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include <caml/config.h>
#include <caml/memory.h>
#include <caml/alloc.h>
#include <caml/mlvalues.h>
#include <caml/fail.h>
#include <caml/signals.h>
#include <caml/callback.h>
#include <caml/unixsupport.h>
#include <caml/custom.h>

#include <rrd.h>

#undef DEBUG

//#define DEBUG
#ifndef DEBUG
#define debug(_f, ...)    ((void)0)
#else
#define debug(_f, _a...)  (printf(_f, ##_a))
#endif

#define Castle_val(v) (*(castle_connection **) Data_custom_val(v))
char *copy_caml_string(value string) 
{
    char *result = malloc(caml_string_length(string) + 1);
    if (!result) return 0;
    memcpy(result, String_val(string), caml_string_length(string)+1);
    return result;
}

CAMLprim void caml_rrd_create(value filename_v, value pdp_step_v, value last_up_v, value args_v)
{
    CAMLparam4(filename_v, pdp_step_v, last_up_v, args_v);

    int i, ret = 0, err = 0, argc = Wosize_val(args_v);
    char *filename;
    char **args;
    unsigned long pdp_step = Int64_val(pdp_step_v);
    time_t last_up = Int_val(last_up_v);
    
    filename = copy_caml_string(filename_v);
    if (!filename)
    {
        err = ENOMEM;
        goto err0;
    }
    
    args = malloc(sizeof(char *) * argc);
    if (!args)
    {
        err = ENOMEM;
        goto err1;
    }

    for (i = 0; i < argc; i++)
        args[i] = NULL;
        
    for (i = 0; i < argc; i++)
    {
        args[i] = copy_caml_string(Field(args_v, i));
        if (!args[i])
        {
            err = ENOMEM;
            goto err2;
        }
    }
    
    caml_enter_blocking_section();
    rrd_get_context();
    rrd_clear_error();
    ret = rrd_create_r(filename, pdp_step, last_up, argc, (const char **)args);
    caml_leave_blocking_section();
    
err2: for (i = 0; i < argc; i++) 
          if (args[i])
              free(args[i]);
      free(args);
err1: free(filename);
err0:
    
    if (err)
        unix_error(err, "caml_rrd_create", Nothing);
    
    if (ret)
        caml_failwith(rrd_get_error());

    CAMLreturn0;
}

CAMLprim void caml_rrd_update_r(value filename_v, value template_v, value args_v)
{
    CAMLparam3(filename_v, template_v, args_v);
    
    int i, ret = 0, err = 0, argc = Wosize_val(args_v);
    char *filename, *template;
    char **args;
    
    filename = copy_caml_string(filename_v);
    if (!filename)
    {
        ret = ENOMEM;
        goto err0;
    }
    
    template  = copy_caml_string(template_v);
    if (!template)
    {
        ret = ENOMEM;
        goto err1;
    }
    
    args = malloc(sizeof(char *) * argc);
    if (!args)
    {
        ret = ENOMEM;
        goto err2;
    }

    for (i = 0; i < argc; i++)
        args[i] = NULL;

    for (i = 0; i < argc; i++)
    {
        args[i] = copy_caml_string(Field(args_v, i));
        if (!args[i])
        {
            ret = ENOMEM;
            goto err3;
        }
    }

    caml_enter_blocking_section();
    rrd_clear_error();
    ret = rrd_update_r(filename, template, argc, (const char **)args);
    caml_leave_blocking_section();

err3: for (i = 0; i < argc; i++)
          if (args[i])
              free(args[i]);
      free(args);
err2: free(template);    
err1: free(filename);
err0:

    if (err)
        unix_error(err, "caml_rrd_update", Nothing);

    if (ret)
        caml_failwith(rrd_get_error());

    CAMLreturn0;
}

CAMLprim value caml_rrd_fetch_r(value filename_v, value cf_v, value start_v, 
    value end_v, value step_v)
{
    CAMLparam5(filename_v, cf_v, start_v, end_v, step_v);
    CAMLlocal3(results, result, tmp);
    
    int ret = 0, err = 0;
    char *filename, *cf ;
    time_t start = Int_val(start_v);
    time_t end = Int_val(end_v);
    
    unsigned long i, j, ds_cnt, data_cnt;
    unsigned long step = Int_val(step_v);
    char **ds_names;
    rrd_value_t *data;
    
    filename = copy_caml_string(filename_v);
    if (!filename) 
    {
        err = ENOMEM;
        goto err0;
    }
    
    cf = copy_caml_string(cf_v);
    if (!filename) 
    {
        err = ENOMEM;
        goto err1;
    }
    
    result = Nothing;
    
    caml_enter_blocking_section();
    rrd_clear_error();
    ret = rrd_fetch_r(filename, cf, &start, &end,  &step,
        &ds_cnt, &ds_names, &data);
    caml_leave_blocking_section();
    
      free(cf);
err1: free(filename);
err0:

    if (ret)
        unix_error(ret, "caml_rrd_fetch_r", Nothing);

    if (err)
        caml_failwith(rrd_get_error());
    
    results = caml_alloc(ds_cnt, 0);
    data_cnt = (end - start) / step;
    
    for (i = 0; i < ds_cnt; i++)
    {
        tmp = caml_alloc(data_cnt, Double_array_tag);
        for (j = 0; j < data_cnt; j++)
        {
            Store_double_field(tmp, j, *(data + (i * data_cnt) + j));
        }

        result = caml_alloc(2, 0);
        Store_field(result, 0, caml_copy_string(ds_names[i]));
        Store_field(result, 1, tmp); 
        
        Store_field(results, i, result);      
    }
    
    free(ds_names);
    free(data);
        
    CAMLreturn(results);
}

CAMLprim value caml_rrd_fetch_ex_r(value filename_v, value cf_v, value start_v,
    value end_v, value step_v)
{
    CAMLparam5(filename_v, cf_v, start_v, end_v, step_v);
    CAMLlocal4(ret_value, results, result, tmp);

    int ret = 0, err = 0;
    char *filename, *cf ;
    time_t start = Int_val(start_v);
    time_t end = Int_val(end_v);

    unsigned long i, j, ds_cnt, data_cnt;
    unsigned long step = Int_val(step_v);
    char **ds_names;
    rrd_value_t *data;

    filename = copy_caml_string(filename_v);
    if (!filename)
    {
        err = ENOMEM;
        goto err0;
    }

    cf = copy_caml_string(cf_v);
    if (!filename)
    {
        err = ENOMEM;
        goto err1;
    }

    result = Nothing;

    caml_enter_blocking_section();
    rrd_clear_error();
    ret = rrd_fetch_r(filename, cf, &start, &end,  &step,
        &ds_cnt, &ds_names, &data);
    caml_leave_blocking_section();

      free(cf);
err1: free(filename);
err0:

    if (ret)
        unix_error(ret, "caml_rrd_fetch_r", Nothing);

    if (err)
        caml_failwith(rrd_get_error());

    ret_value = caml_alloc(4, 0);

    results = caml_alloc(ds_cnt, 0);
    data_cnt = (end - start) / step;

    for (i = 0; i < ds_cnt; i++)
    {
        tmp = caml_alloc(data_cnt, Double_array_tag);
        for (j = 0; j < data_cnt; j++)
        {
            Store_double_field(tmp, j, *(data + (i * data_cnt) + j));
        }

        result = caml_alloc(2, 0);
        Store_field(result, 0, caml_copy_string(ds_names[i]));
        Store_field(result, 1, tmp);

        Store_field(results, i, result);
    }

    free(ds_names);
    free(data);

    Store_field(ret_value, 0, results);
    Store_field(ret_value, 1, Val_int(start));
    Store_field(ret_value, 2, Val_int(end));
    Store_field(ret_value, 3, Val_int(step));

    CAMLreturn(ret_value);
}
